// ----------------------------------------------------------------------------
// Part of gprolog-postgresql
//
// This file is modified from the PostgreSQL 7.4.7 distribution, taken from
// file: ./src/interfaces/ecpg/pgtypeslib/timestamp.c to which the following
// copyright notice applies:
//
//   PostgreSQL Data Base Management System
//   (formerly known as Postgres, then as Postgres95).
// 
//   Portions Copyright (c) 1996-2002, The PostgreSQL Global Development
//   Group
// 
//   Portions Copyright (c) 1994, The Regents of the University of
//   California
// 
//   Permission to  use, copy, modify,  and distribute this  software and
//   its  documentation  for any  purpose,  without  fee,  and without  a
//   written  agreement  is  hereby  granted,  provided  that  the  above
//   copyright notice and this paragraph and the following two paragraphs
//   appear in all copies.
// 
//   IN  NO EVENT SHALL  THE UNIVERSITY  OF CALIFORNIA  BE LIABLE  TO ANY
//   PARTY  FOR DIRECT, INDIRECT,  SPECIAL, INCIDENTAL,  OR CONSEQUENTIAL
//   DAMAGES,  INCLUDING LOST  PROFITS, ARISING  OUT OF  THE USE  OF THIS
//   SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
//   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
//   THE UNIVERSITY OF  CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
//   INCLUDING,   BUT  NOT   LIMITED  TO,   THE  IMPLIED   WARRANTIES  OF
//   MERCHANTABILITY AND FITNESS FOR  A PARTICULAR PURPOSE.  THE SOFTWARE
//   PROVIDED HEREUNDER  IS ON  AN "AS IS"  BASIS, AND THE  UNIVERSITY OF
//   CALIFORNIA  HAS  NO  OBLIGATIONS  TO PROVIDE  MAINTENANCE,  SUPPORT,
//   UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
// 
// ----------------------------------------------------------------------------

#undef Min			/* gprolog-awareness */
#undef Max			/* idem */

//#define HAVE_INT64_TIMESTAMP 1
//#define INT64_IS_BUSTED 1

#include "postgres_fe.h"
#include <time.h>
#include <float.h>
#include <math.h>

#ifdef __FAST_MATH__
#error -ffast-math is known to break this code
#endif

#include <server/postgres.h>
#include <server/utils/timestamp.h>

#define TimestampTz ZORG1	/* Crock. */
#define fsec_t ZORG2		/* here also. */
#include "dt.h"
#include "pgtypes_date.h"
#undef TimestampTz
#undef fsec_t

int PGTYPEStimestamp_defmt_scan(char **, char *, timestamp *, int *, int *, int *,
							int *, int *, int *, int *);

#ifdef HAVE_INT64_TIMESTAMP
static int64
time2t(const int hour, const int min, const int sec, const fsec_t fsec)
{
  return ((((((hour * 60) + min) * 60) + sec) * INT64CONST(1000000)) + fsec);
}	/* time2t() */

#else
static double
time2t(const int hour, const int min, const int sec, const fsec_t fsec)
{
  return ((((hour * 60) + min) * 60) + sec + fsec);
}	/* time2t() */
#endif

static timestamp
dt2local(timestamp dt, int tz)
{
#ifdef HAVE_INT64_TIMESTAMP
  dt -= (tz * INT64CONST(1000000));
#else
  dt -= tz;
  dt = JROUND(dt);
#endif
  return dt;
}	/* dt2local() */

/* tm2timestamp()
 * Convert a tm structure to a timestamp data type.
 * Note that year is _not_ 1900-based, but is an explicit full value.
 * Also, month is one-based, _not_ zero-based.
 *
 * Returns -1 on failure (overflow).
 */
int
tm2timestamp(struct tm * tm, fsec_t fsec, int *tzp, timestamp *result)
{
#ifdef HAVE_INT64_TIMESTAMP
  int		dDate;
  int64		time;

#else
  double		dDate, time;
#endif

  /* Julian day routines are not correct for negative Julian days */
  if (!IS_VALID_JULIAN(tm->tm_year, tm->tm_mon, tm->tm_mday))
    return -1;

  dDate = date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) - date2j(2000, 1, 1);
  time = time2t(tm->tm_hour, tm->tm_min, tm->tm_sec, fsec);
#ifdef HAVE_INT64_TIMESTAMP
  *result = (dDate * INT64CONST(86400000000)) + time;
  /* check for major overflow */
  if ((*result - time) / INT64CONST(86400000000) != dDate)
    return -1;
  /* check for just-barely overflow (okay except time-of-day wraps) */
  if ((*result < 0) ? (dDate >= 0) : (dDate < 0))
    return -1;
#else
  *result = ((dDate * 86400) + time);
#endif
  if (tzp != NULL)
    *result = dt2local(*result, -(*tzp));

  return 0;
}	/* tm2timestamp() */

timestamp
SetEpochTimestamp(void)
{
  timestamp	dt;
  struct tm	tt,
    *tm = &tt;

  GetEpochTime(tm);
  tm2timestamp(tm, 0, NULL, &dt);
  return dt;
}	/* SetEpochTimestamp() */

void
dt2time(timestamp jd, int *hour, int *min, int *sec, fsec_t *fsec)
{
#ifdef HAVE_INT64_TIMESTAMP
  int64		time;

#else
  double		time;
#endif

  time = jd;

#ifdef HAVE_INT64_TIMESTAMP
  *hour = (time / INT64CONST(3600000000));
  time -= ((*hour) * INT64CONST(3600000000));
  *min = (time / INT64CONST(60000000));
  time -= ((*min) * INT64CONST(60000000));
  *sec = (time / INT64CONST(1000000));
  *fsec = (time - (*sec * INT64CONST(1000000)));
  *sec = (time / INT64CONST(1000000));
  *fsec = (time - (*sec * INT64CONST(1000000)));
#else
  *hour = (time / 3600);
  time -= ((*hour) * 3600);
  *min = (time / 60);
  time -= ((*min) * 60);
  *sec = time;
  *fsec = JROUND(time - *sec);
#endif
  return;
}	/* dt2time() */

/* timestamp2tm()
 * Convert timestamp data type to POSIX time structure.
 * Note that year is _not_ 1900-based, but is an explicit full value.
 * Also, month is one-based, _not_ zero-based.
 * Returns:
 *	 0 on success
 *	-1 on out of range
 *
 * For dates within the system-supported time_t range, convert to the
 *	local time zone. If out of this range, leave as GMT. - tgl 97/05/27
 */
int
timestamp2tm(timestamp dt, int *tzp, struct tm * tm, fsec_t *fsec, char **tzn)
{
#ifdef HAVE_INT64_TIMESTAMP
  int		dDate, date0;
  int64		time;
#else
  double		dDate, date0;
  double		time;
#endif
  time_t		utime;

#if defined(HAVE_TM_ZONE) || defined(HAVE_INT_TIMEZONE)
  struct tm  *tx;
#endif

  date0 = date2j(2000, 1, 1);

  time = dt;
#ifdef HAVE_INT64_TIMESTAMP
  TMODULO(time, dDate, INT64CONST(86400000000));

  if (time < INT64CONST(0))
    {
      time += INT64CONST(86400000000);
      dDate -= 1;
    }
#else
  TMODULO(time, dDate, 86400e0);

  if (time < 0)
    {
      time += 86400;
      dDate -= 1;
    }
#endif

  /* Julian day routine does not work for negative Julian days */
  if (dDate < -date0)
    return -1;

  /* add offset to go from J2000 back to standard Julian date */
  dDate += date0;

  j2date((int) dDate, &tm->tm_year, &tm->tm_mon, &tm->tm_mday);
  dt2time(time, &tm->tm_hour, &tm->tm_min, &tm->tm_sec, fsec);

  if (tzp != NULL)
    {
      /*
       * Does this fall within the capabilities of the localtime()
       * interface? Then use this to rotate to the local time zone.
       */
      if (IS_VALID_UTIME(tm->tm_year, tm->tm_mon, tm->tm_mday))
	{
#ifdef HAVE_INT64_TIMESTAMP
	  utime = ((dt / INT64CONST(1000000))
		   + ((date0 - date2j(1970, 1, 1)) * INT64CONST(86400)));
#else
	  utime = (dt + ((date0 - date2j(1970, 1, 1)) * 86400));
#endif

#if defined(HAVE_TM_ZONE) || defined(HAVE_INT_TIMEZONE)
	  tx = localtime(&utime);
	  tm->tm_year = tx->tm_year + 1900;
	  tm->tm_mon = tx->tm_mon + 1;
	  tm->tm_mday = tx->tm_mday;
	  tm->tm_hour = tx->tm_hour;
	  tm->tm_min = tx->tm_min;
	  tm->tm_isdst = tx->tm_isdst;

#if defined(HAVE_TM_ZONE)
	  tm->tm_gmtoff = tx->tm_gmtoff;
	  tm->tm_zone = tx->tm_zone;

	  *tzp = -(tm->tm_gmtoff);	/* tm_gmtoff is Sun/DEC-ism */
	  if (tzn != NULL)
	    *tzn = (char *) tm->tm_zone;
#elif defined(HAVE_INT_TIMEZONE)
	  *tzp = ((tm->tm_isdst > 0) ? (TIMEZONE_GLOBAL - 3600) : TIMEZONE_GLOBAL);
	  if (tzn != NULL)
	    *tzn = tzname[(tm->tm_isdst > 0)];
#endif

#else							/* not (HAVE_TM_ZONE || HAVE_INT_TIMEZONE) */
	  *tzp = 0;
	  /* Mark this as *no* time zone available */
	  tm->tm_isdst = -1;
	  if (tzn != NULL)
	    *tzn = NULL;
#endif

	  dt = dt2local(dt, *tzp);
	}
      else
	{
	  *tzp = 0;
	  /* Mark this as *no* time zone available */
	  tm->tm_isdst = -1;
	  if (tzn != NULL)
	    *tzn = NULL;
	}
    }
  else
    {
      tm->tm_isdst = -1;
      if (tzn != NULL)
	*tzn = NULL;
    }

  return 0;
}	/* timestamp2tm() */


// Local Variables:
// compile-command: "gcc -c -I/usr/include/postgresql -I/usr/include/postgresql/internal -I/usr/include/postgresql/server timestamp.c"
// End:
