/* $Id$ */

/* ----------------------------------------------------------------------------
 * GPROLOG-POSTGRESQL is Copyright (C) 1999-2004 Salvador Abreu
 * 
 *    This program  is free software;  you can redistribute  it and/or
 *    modify  it under  the terms  of  the GNU  Lesser General  Public
 *    License  as published  by the  Free Software  Foundation; either
 *    version 2.1, or (at your option) any later version.
 * 
 *    This program is distributed in  the hope that it will be useful,
 *    but WITHOUT  ANY WARRANTY; without even the  implied warranty of
 *    MERCHANTABILITY or  FITNESS FOR  A PARTICULAR PURPOSE.   See the
 *    GNU Lesser General Public License for more details.
 * 
 *    You should have received a copy of the GNU Lesser General Public
 *    License  along with  this program;  if  not, write  to the  Free
 *    Software Foundation, Inc., 59  Temple Place - Suite 330, Boston,
 *    MA 02111-1307, USA.
 * 
 * On Debian  GNU/Linux systems, the  complete text of the  GNU Lesser
 * General Public License is found in /usr/share/common-licenses/LGPL.
 * ----------------------------------------------------------------------------
 */

/* ============================================================================
 *
 * GNU Prolog interface for PostgreSQL (pl-pq.c)
 *
 * Predicates:
 *   pq_open(+HOST, +PORT, +DBNAME, -CONNINDEX)
 *   pq_close(+CONNINDEX)
 *
 *   pq_exec(+CONNINDEX, +QUERY, -RESINDEX)
 *   pq_fetch(+RESINDEX)
 *
 *   pq_ntuples(+RESINDEX, -COUNT)
 *   pq_last_oid(+RESINDEX, -OID)
 *
 *   pq_get_data_*(+RESINDEX, +COLNO, -RESTERM)
 *
 *   pq_begin(+CONNINDEX)
 *   pq_end(+CONNINDEX, +ABORT)
 *
 *   pq_get_binary(+CONNINDEX, -BINARY)
 *   pq_set_binary(+CONNINDEX, +BINARY)
 *
 *   pq_set_timing(+CONNINDEX, +TIMING)
 *   pq_stats(+CONNINDEX, -NQUERIES, -NRESULTS, -TIMES)
 */

#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>

void swab(const void *from, void *to, ssize_t n);

#undef Min
#undef Max

#include <postgresql/libpq-fe.h>
#include <postgresql/sql3types.h>

#include <postgresql/server/postgres.h>
#include <postgresql/server/utils/timestamp.h>



#include <gprolog.h>


/* -- Handle xwd(UPPER, LOWER) for longer-than-tagged ints ----------------- */

#define XWD_BITS	27	/* 18 would be DEC-10 nostalgia ;-) */
#define XWD_MASK        ((1<<XWD_BITS)-1)
//#define XWD_needed(x)   (((x) & ~XWD_MASK) != 0)
#define XWD_needed(x)   (Rd_Integer_Check (Mk_Integer (x)) != (x))
#define XWD_UH(x)       ((x) >> XWD_BITS)
#define XWD_LH(x)       ((x) & XWD_MASK)
#define XWD(uh, lh)     ((uh) << XWD_BITS) | (lh)

static int xwd_functor = 0;
static PlTerm xwd_args[2];

static inline PlTerm xwd_int32 (unsigned long v) {
  if (XWD_needed (v)) {
    if (!xwd_functor) xwd_functor = Create_Atom ("xwd");
    xwd_args[0] = Mk_Integer (XWD_UH(v));
    xwd_args[1] = Mk_Integer (XWD_LH(v));
    return Mk_Compound (xwd_functor, 2, xwd_args);
  }
  else
    return Mk_Integer (v);
}

/* == Type declarations ==================================================== */

typedef struct {
  PGconn *conn;			/* connection */
  int     active;		/* number of active queries */
  int     last_oid;		/* OID for last query (insert only) */
  int     binary;		/* is this a binary connection? */
  int     depth;		/* number of active transactions */
  int     timing;		/* are we timing & counting? */
  int     nqueries;		/* total # of queries */
  int     nresults;		/* total # of results returned */
  int     times[3];		/* USER, SYSTEM, ELAPSED */
#define PQT_U 0
#define PQT_S 1
#define PQT_E 2
} PQ_conn;

typedef struct {
  int       connx;		/* connection used for this result */
  PGresult *res;		/* the result itself */
  int       row;		/* next row # */
  int       nrows;		/* number of rows */
  int      *chb;		/* related to corresponding choice-point */
} PQ_res;



/* == Global variable declarations ========================================= */

#define PQ_MAX_CONN 256
#define PQ_MAX_RES  16384

static PQ_conn connections[PQ_MAX_CONN] = { { 0, 0 }, };
static PQ_res  results[PQ_MAX_RES] = { { 0, 0, 0 }, };

int PQ_max_conn = 0;
int PQ_max_res = 0;


/* == Support macros ======================================================= */

#define CHECK_CONN(cxi, index_only, fname)				     \
  do {									     \
    int cx = (cxi);							     \
    if (0 <= cx && cx < PQ_MAX_CONN && (index_only || connections[cx].conn)) \
      break;								     \
    Pl_Err_System (Create_Atom (#fname ": illegal result index"));	     \
    return FALSE;							     \
  } while (0)

#define CHECK_RES(rxi, index_only, fname)				\
  do {									\
    int rx = (rxi);							\
    if (0 <= rx && rx < PQ_MAX_RES && (index_only || results[rx].res))	\
      break;								\
    Pl_Err_System (Create_Atom (#fname ": illegal result index"));	\
    return FALSE;							\
  } while (0)


#define INIT_TIMINGS()				\
  do {						\
    if (connections[connx].timing) {		\
      ts[PQT_U] = M_User_Time ();		\
      ts[PQT_S] = M_System_Time ();		\
      ts[PQT_E] = M_Real_Time ();		\
    }						\
  } while (0)

#define UPDATE_TIMINGS()						\
  do {									\
    if (connections[connx].timing) {					\
      connections[connx].times[PQT_U] += M_User_Time ()   - ts[PQT_U];	\
      connections[connx].times[PQT_S] += M_System_Time () - ts[PQT_S];	\
      connections[connx].times[PQT_E] += M_Real_Time ()   - ts[PQT_E];	\
    }									\
  } while (0)

/* == Prototypes =========================================================== */

Bool pq_open (char*, int, char*, int*);
Bool pq_close (int);
Bool pq_begin (int);		/* transaction group start */
Bool pq_end (int, int);		/* transaction group end */
Bool pq_get_binary (int, int*);
Bool pq_set_binary (int, int);
Bool pq_exec (int, char*, int*);
Bool pq_fetch (int);
Bool pq_ntuples (int, int*);
Bool pq_last_oid(int, PlTerm*);
//Bool pq_get_data(int, int, int, PlTerm*);
Bool pq_clear (int);


/* == Predicate functions ================================================== */

// :- foreign(pq_open(+string, +integer, +string, -integer))
// Returns the index of a PQ_conn

Bool pq_open(char *host, int port, char *db, int *connx)
{
  char port_str[64];
  int i;

  sprintf (port_str, "%d", port? port: 5432);

  for (i=0; i<PQ_MAX_CONN; ++i) {
    if (connections[i].conn == 0) {
      *connx = i;
      connections[i].conn = PQsetdb(host, port_str, NULL, NULL, db);
      connections[i].active = 0;


      if (PQstatus(connections[i].conn) == CONNECTION_BAD) {
	static char msg[256];
	sprintf(msg, "psql: %s", PQerrorMessage(connections[i].conn));
	PQfinish(connections[i].conn);
	connections[i].conn = 0;
	Pl_Err_System(Create_Allocate_Atom(msg));
	return FALSE;
      }

      connections[i].binary = PQprotocolVersion(connections[i].conn) >= 3;
      connections[i].timing = 0;
      connections[i].nqueries = 0;
      connections[i].nresults = 0;
      connections[i].times[PQT_U] = 0;
      connections[i].times[PQT_S] = 0;
      connections[i].times[PQT_E] = 0;

      return TRUE;
    }
  }
  Pl_Err_System (Create_Atom ("psql: too many open connections"));
  return FALSE;
}


// :- foreign(pq_close(+integer))

Bool pq_close (int connx)
{
  if (0 <= connx &&
      connx < PQ_MAX_CONN &&
      connections[connx].conn) {
    PQfinish (connections[connx].conn);
    connections[connx].conn = 0;
    return TRUE;
  }
  return FALSE;
}

// :- foreign(pq_begin(+integer))

Bool pq_begin (int connx)
{
  PGconn *conn = NULL;
  PGresult *res;

  CHECK_CONN (connx, 0, pq_begin/1);
  conn = connections[connx].conn;
  if (connections[connx].depth++ < 1) {	/* need to BEGIN */
    res = PQexec (conn, "BEGIN");
    PQclear (res);
  }
  return TRUE;
}

// :- foreign(pq_end(+integer, +integer)

Bool pq_end (int connx, int abort)
{
  PGconn *conn = NULL;
  PGresult *res;

  CHECK_CONN (connx, 0, pq_end/2);
  conn = connections[connx].conn;
  if (connections[connx].depth > 0) {
    if (--connections[connx].depth < 1) { /* need to END/ABORT */
      res = PQexec (conn, abort? "ROLLBACK": "COMMIT");
      PQclear (res);
    }
  }
  return TRUE;
}


// :- foreign(pq_get_binary(+integer, -integer))

Bool pq_get_binary (int connx, int *binary)
{
  CHECK_CONN (connx, 0, pq_get_binary/2);
  *binary = connections[connx].binary;
  return TRUE;
}


// :- foreign(pq_set_binary(+integer, +integer))

Bool pq_set_binary (int connx, int binary)
{
  CHECK_CONN (connx, 0, pq_set_binary/2);
  connections[connx].binary = binary;
  return TRUE;
}


// :- foreign(pq_exec(+integer, +string, -integer), [choice_size(1)])
// Returns the index of a PQ_res

Bool pq_exec(int connx, char *query, int *resx)
{
  static char msg[256];
  PGconn *conn = NULL;
  PGresult *res;
  int i;
  int nrows;
  int *chb = Get_Choice_Buffer (int *);
  int ts[3];

  CHECK_CONN (connx, 0, pq_exec/3);
  conn = connections[connx].conn;

  if (Get_Choice_Counter() != 0) { // Been here already: advance to next tuple
    if (connections[connx].timing) connections[connx].nresults++;

    i = *chb;
    *resx = i;
    if (++results[i].row < results[i].nrows)
      return TRUE;		// More results available

    No_More_Choice ();
    if (results[i].res) {	// watermark cleanup might do it!
      PQclear (results[i].res);
      results[i].res = 0;
    }
    return FALSE;
  }
  else {			// First time around: maybe create chp.
    INIT_TIMINGS ();
    *resx = -1;
    if (connections[connx].binary)
      res = PQexecParams (conn, query, 0, 0, 0, 0, 0, 1);
    else
      res = PQexec (conn, query);

    if (connections[connx].timing) connections[connx].nqueries++;

    switch (PQresultStatus (res)) {
    case PGRES_EMPTY_QUERY:	// The string sent to the backend was empty.
      No_More_Choice();
      PQclear (res);
      UPDATE_TIMINGS ();
      return FALSE;

    case PGRES_COMMAND_OK:	// Succ. compl. of a command returning no data
      No_More_Choice();
      connections[connx].last_oid = (int) PQoidValue (res);
      PQclear (res);
      UPDATE_TIMINGS ();
      return TRUE;

    case PGRES_TUPLES_OK:	// The query executed successfully
      nrows = PQntuples (res);
      if (nrows == 0) {
	PQclear (res);
	No_More_Choice ();
	UPDATE_TIMINGS ();
	return FALSE;
      }

      for (i=0; i<PQ_MAX_RES; ++i) {
	if (results[i].res == 0) {	   // Found an empty slot
	  if (connections[connx].timing) connections[connx].nresults++;

	  *chb = i;			   // save the index
	  results[i].chb   = chb;	   // remember where
	  results[i].res   = res;          // save the result,
	  results[i].connx = connx;        // the connection,
	  results[i].row   = 0;            // the row (initial value),
	  results[i].nrows = nrows;        // and the number of rows
	  connections[connx].active++;     // count them
	  *resx = i;

	  Create_Water_Mark ((void (*) ()) pq_clear,
			     (void *) i); // will undo this.
	  UPDATE_TIMINGS ();
	  return TRUE;
	}
      }
      PQclear (res);
      No_More_Choice ();
      Pl_Err_System (Create_Atom ("pq_exec/3: too many open results"));
      UPDATE_TIMINGS ();
      return FALSE;

    case PGRES_COPY_OUT:	// Copy Out (from server) data transfer started
    case PGRES_COPY_IN:		// copy In (to server) data transfer started
      No_More_Choice();
      PQclear (res);
      res = NULL;
      Pl_Err_System(Create_Atom("psql: back-end expecting copy in/out"));
      UPDATE_TIMINGS ();
      return FALSE;

    case PGRES_BAD_RESPONSE:	// The server's response was not understood
    case PGRES_NONFATAL_ERROR:
    case PGRES_FATAL_ERROR:
    default:
      No_More_Choice();
      sprintf(msg, "psql: %s", PQerrorMessage(conn));
      PQclear (res);
      Pl_Err_System(Create_Allocate_Atom(msg));
      UPDATE_TIMINGS ();
      return FALSE;
    }
  }
}

// :- foreign(pq_fetch(+integer))

Bool pq_fetch (int dummy)
{
  return TRUE;			/* this is a no-op for postgres */
}


// :- foreign(pq_ntuples(+integer, -integer))

Bool pq_ntuples (int resx, int* ntuples)
{
  CHECK_RES (resx, 0, pq_ntuples);
  *ntuples = results[resx].nrows;
  return TRUE;
}

// :- foreign(pq_last_oid(+integer, -term))

Bool pq_last_oid (int connx, PlTerm *oidT)
{
  unsigned int oid;
  CHECK_CONN (connx, 0, pq_ntuples);
  oid = connections[connx].last_oid;
  *oidT = xwd_int32 (oid);
  return TRUE;
}


// :- foreign(pq_get_data_int(+integer, +integer, -integer))

Bool pq_get_data_int (int resx, int colno, PlTerm *value)
{
  char *value_tmp;
  int connx;
  int ts[3];
  long ltemp;
  long long lltemp;

  CHECK_RES (resx, 0, pq_get_data_int);
  connx = results[resx].connx;

  if (!xwd_functor) xwd_functor = Create_Atom ("xwd");

  INIT_TIMINGS ();
  value_tmp = PQgetvalue (results[resx].res, results[resx].row, colno-1);
  if (connections[connx].binary) {
    int len = PQgetlength (results[resx].res, results[resx].row, colno-1);
    switch (len) {
    case 0:
      *value = 0;
      break;
    case 4:
      ((uint32_t *) &ltemp)[0] = ntohl (((uint32_t *)value_tmp)[0]);
      *value = xwd_int32 (ltemp);
      break;
    case 8:
      ((uint32_t *) &lltemp)[0] = ntohl (((uint32_t *)value_tmp)[1]);
      ((uint32_t *) &lltemp)[1] = ntohl (((uint32_t *)value_tmp)[0]);
      *value = xwd_int32 (lltemp); /* FIXME: overflows 64 -> 32 */
      break;
    default: {
      char msg[128];
      sprintf (msg, "illegal int result length: %d (col %d)", len, colno);
      Pl_Err_System (Create_Allocate_Atom (msg));
      UPDATE_TIMINGS ();
      return FALSE;
    }
    }
  }
  else
    sscanf (value_tmp, "%d", value);
  UPDATE_TIMINGS ();
  return TRUE;
}

// :- foreign(pq_get_data_float(+integer, +integer, -float))

Bool pq_get_data_float (int resx, int colno, double *value)
{
  char *value_tmp;
  float ftemp;
  double dtemp;
  int connx;
  int ts[3];

  CHECK_RES (resx, 0, pq_get_data_int);
  connx = results[resx].connx;

  INIT_TIMINGS ();
  value_tmp = PQgetvalue (results[resx].res, results[resx].row, colno-1);
  if (connections[connx].binary) {
    int len = PQgetlength (results[resx].res, results[resx].row, colno-1);
    switch (len) {
    case 0:
      *value = 0.0;
      break;
    case 4:
      *((uint32_t *) &ftemp) = ntohl (((uint32_t *)value_tmp)[0]);
      *value = ftemp;
      break;
    case 8:
      ((uint32_t *) &dtemp)[0] = ntohl (((uint32_t *)value_tmp)[1]);
      ((uint32_t *) &dtemp)[1] = ntohl (((uint32_t *)value_tmp)[0]);
      *value = dtemp;
      break;
    default: {
      char msg[128];
      sprintf (msg, "illegal float result length: %d (col %d)", len, colno);
      Pl_Err_System (Create_Allocate_Atom (msg));
      UPDATE_TIMINGS ();
      return FALSE;
    }
    }
  }
  else
    sscanf (value_tmp, "%lg", value);
  UPDATE_TIMINGS ();
  return TRUE;
}

// :- foreign(pq_get_data_bool(+integer, +integer, -atom))

Bool pq_get_data_bool (int resx, int colno, int *value)
{
  char *value_tmp;
  int connx;
  int ts[3];

  CHECK_RES (resx, 0, pq_get_data_int);
  connx = results[resx].connx;
  INIT_TIMINGS ();

  value_tmp = PQgetvalue (results[resx].res, results[resx].row, colno-1);
  if (connections[connx].binary) {
    int len = PQgetlength (results[resx].res, results[resx].row, colno-1);
    switch (len) {
    case 0:
      *value = FALSE;
      break;
    case 4:
      ((uint32_t *) value)[0] = ntohl (((uint32_t *)value_tmp)[0]);
      break;
    case 1:
      *value = * (char *) value_tmp;
      break;
    default: {
      char msg[128];
      sprintf (msg, "illegal bool result length: %d (col %d)", len, colno);
      Pl_Err_System (Create_Allocate_Atom (msg));
      UPDATE_TIMINGS ();
      return FALSE;
    }
    }
  }
  else
    *value = value_tmp[0] == 't';
  UPDATE_TIMINGS ();
  return TRUE;
}

// :- foreign(pq_get_data_date(+integer, +integer, -term))

Bool pq_get_data_date (int resx, int colno, PlTerm *value)
{
  fsec_t fsec;
  struct tm tm;
  char *value_tmp;
  int   connx;
  int    dt_v[6] = { 1, 2, 3, 4, 5, 6 };
  PlTerm dt_a[6];
  int ts[3];

  CHECK_RES (resx, 0, pq_get_data_date);
  connx = results[resx].connx;
  
  INIT_TIMINGS ();
  value_tmp = PQgetvalue (results[resx].res, results[resx].row, colno-1);

  if (connections[connx].binary) {
    // len must be 8:
    int len = PQgetlength (results[resx].res, results[resx].row, colno-1);
    Timestamp svalue_tmp;

    if (len != 8) {
      *value = Mk_String ("null");
      UPDATE_TIMINGS ();
      return TRUE;
    }

    ((uint32_t *) &svalue_tmp)[0] = ntohl (((uint32_t *)value_tmp)[1]);
    ((uint32_t *) &svalue_tmp)[1] = ntohl (((uint32_t *)value_tmp)[0]);

    timestamp2tm (svalue_tmp, 0, &tm, &fsec, 0);

    dt_v[0] = tm.tm_year;
    dt_v[1] = tm.tm_mon;
    dt_v[2] = tm.tm_mday;
    dt_v[3] = tm.tm_hour;
    dt_v[4] = tm.tm_min;
    dt_v[5] = tm.tm_sec;
  }
  else {
    sscanf (value_tmp, "%4d-%2d-%2d %2d:%2d:%2d",
	    &dt_v[0], &dt_v[1], &dt_v[2],
	    &dt_v[3], &dt_v[4], &dt_v[5]);
  }

  dt_a[0] = Mk_Integer (dt_v[0]); /* YYYY */
  dt_a[1] = Mk_Integer (dt_v[1]); /* MM */
  dt_a[2] = Mk_Integer (dt_v[2]); /* DD */
  dt_a[3] = Mk_Integer (dt_v[3]); /* hh */
  dt_a[4] = Mk_Integer (dt_v[4]); /* mm */
  dt_a[5] = Mk_Integer (dt_v[5]); /* ss */
  *value = Mk_Compound (Create_Atom ("dt"), 6, dt_a);
  UPDATE_TIMINGS ();
  return TRUE;
}

// :- foreign(pq_get_data_atom(+integer, +integer, -string))
// :- foreign(pq_get_data_codes(+integer, +integer, -codes))

Bool pq_get_data_string (int resx, int colno, char **value)
{
  int connx;
  int ts[3];

  CHECK_RES (resx, 0, pq_get_data_string);
  connx = results[resx].connx;

  INIT_TIMINGS ();
  *value = PQgetvalue (results[resx].res, results[resx].row, colno-1);
  UPDATE_TIMINGS ();
  return TRUE;
}

// :- foreign(pq_set_timing(+integer, +integer))

Bool pq_set_timing (int connx, int timing)
{
  CHECK_CONN (connx, 0, pq_set_timing/2);
  connections[connx].timing = timing;
  if (timing) {
    connections[connx].nqueries = 0;
    connections[connx].nresults = 0;
    connections[connx].times[PQT_U] = 0;
    connections[connx].times[PQT_S] = 0;
    connections[connx].times[PQT_E] = 0;
  }
  return TRUE;
}

// :- foreign(pq_stats(+integer, -integer, -integer, -list(integer))

Bool pq_stats (int connx, int *nqueries, int *nresults, PlTerm *times)
{
  PlTerm ts[3];

  CHECK_CONN (connx, 0, pq_set_timing/2);
  *nqueries = connections[connx].nqueries;
  *nresults = connections[connx].nresults;
  ts[0] = Mk_Integer (connections[connx].times[PQT_U]);
  ts[1] = Mk_Integer (connections[connx].times[PQT_S]);
  ts[2] = Mk_Integer (connections[connx].times[PQT_E]);
  *times = Mk_Proper_List (3, ts);
  return TRUE;
}

/* == Support functions ==================================================== */


Bool pq_clear (int resx)
{
  if (0 <= resx && resx < PQ_MAX_RES && results[resx].res) {
    PQclear (results[resx].res);
    results[resx].res = 0;
    connections[results[resx].connx].active -= 1;
  }

  return TRUE;
}


/*
 * $Log$
 * Revision 1.14  2005/05/09 12:25:18  spa
 * Change xwd/2 criterion: it's only used if X != READ(MAKE(X))...
 *
 * Revision 1.13  2005/05/07 15:38:30  spa
 * urk. fixed XWD_needed.
 *
 * Revision 1.12  2005/05/07 15:29:55  spa
 * Handle ints that can't fit in a tagged Prolog cell: use the DEC-10
 * approach "xwd(UPPER, LOWER)"; allow for 27 bits in LOWER to avoid going
 * into this scheme too often.
 *
 * Revision 1.11  2005/05/02 13:58:10  spa
 * Fix calls to timestamp2tm().
 *
 * Revision 1.10  2005/04/28 15:51:24  gjm
 * OID as string.
 *
 * Revision 1.9  2004/09/15 20:42:32  spa
 * *** empty log message ***
 *
 * Revision 1.8  2004/04/29 22:00:02  spa
 * Allow for null values everywhere.
 *
 * Revision 1.7  2004/04/29 21:26:10  spa
 * Handle bigints in integer results.  (mimmick float behaviour)
 *
 * Revision 1.6  2004/04/27 23:09:34  spa
 * Allow for counting and timing of queries and results.
 *
 * Revision 1.5  2004/04/27 11:19:16  spa
 * only call PQexecParams() when using binary connections.
 *
 * Revision 1.4  2004/04/27 09:31:46  spa
 * - Auto-detect binary capability based on server protocol version.  Do
 * - timestamp conversion using timestamp2tm() copied from PostgreSQL
 *   distribution.
 *
 * Revision 1.3  2004/04/26 13:40:40  spa
 * int4, float4, float8, bool, string: all working!
 * date: still needs attention.
 *
 * Revision 1.2  2004/04/26 11:53:29  spa
 * Initial attempt to get binary result transfer, with cursors.  To be
 * abandoned shortly!
 *
 * Revision 1.1.1.1  2003/01/07 19:56:02  spa
 * Initial import
 *
 * Revision 1.9  2002/05/05 07:33:54  spa
 * Backed off no-watermark changes.
 *
 * Revision 1.8  2002/04/23 21:11:42  spa
 * Don't use watermarks any more.
 *
 * Revision 1.7  2002/04/23 15:58:43  spa
 * *** empty log message ***
 *
 * Revision 1.6  2001/12/05 00:27:51  spa
 * pq_get_data() rewritten as a bunch of type-specific functions.
 * sscanf format nor different.
 *
 * Revision 1.5  2001/08/24 18:18:08  spa
 * pq_exec(): don't succeed if there are zero tuples!
 *
 * Revision 1.4  2001/08/23 23:41:20  spa
 * When returning the default (string) type, should have used Mk_String() instead
 * of just Create_Atom().
 *
 * Revision 1.3  2001/08/23 23:27:49  spa
 * pg_get_data/4: column index starting at 1 means SUBTRACT 1, not ADD 1!!!
 *
 * Revision 1.2  2001/08/23 22:07:02  spa
 * pg_get_data/4: column index now starts at 1.
 *
 * Revision 1.1  2001/08/23 19:45:28  spa
 * Initial version (seems to work).
 *
 *
 * Local Variables:
 * mode: font-lock
 * End:
 */
 
