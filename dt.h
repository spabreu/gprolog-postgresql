// ----------------------------------------------------------------------------
// Part of gprolog-postgresql
//
// This file is modified from the PostgreSQL 7.4.7 distribution, taken from
// file: ./src/interfaces/ecpg/pgtypeslib/dt.h to which the following
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


#ifndef DT_H
#define DT_H

#include <pgtypes_timestamp.h>

#define MAXTZLEN			 10

#ifdef HAVE_INT64_TIMESTAMP

typedef int32 fsec_t;

#else

typedef double fsec_t;

#define TIME_PREC_INV 1000000.0
#define JROUND(j) (rint(((double) (j))*TIME_PREC_INV)/TIME_PREC_INV)
#endif

#define USE_POSTGRES_DATES				0
#define USE_ISO_DATES					1
#define USE_SQL_DATES					2
#define USE_GERMAN_DATES				3

#define DAGO			"ago"
#define EPOCH			"epoch"
#define INVALID			"invalid"
#define EARLY			"-infinity"
#define LATE			"infinity"
#define NOW				"now"
#define TODAY			"today"
#define TOMORROW		"tomorrow"
#define YESTERDAY		"yesterday"
#define ZULU			"zulu"

#define DMICROSEC		"usecond"
#define DMILLISEC		"msecond"
#define DSECOND			"second"
#define DMINUTE			"minute"
#define DHOUR			"hour"
#define DDAY			"day"
#define DWEEK			"week"
#define DMONTH			"month"
#define DQUARTER		"quarter"
#define DYEAR			"year"
#define DDECADE			"decade"
#define DCENTURY		"century"
#define DMILLENNIUM		"millennium"
#define DA_D			"ad"
#define DB_C			"bc"
#define DTIMEZONE		"timezone"
#define DCURRENT		   "current"

/*
 * Fundamental time field definitions for parsing.
 *
 *	Meridian:  am, pm, or 24-hour style.
 *	Millennium: ad, bc
 */

#define AM		0
#define PM		1
#define HR24	2

#define AD		0
#define BC		1

/*
 * Fields for time decoding.
 *
 * Can't have more of these than there are bits in an unsigned int
 * since these are turned into bit masks during parsing and decoding.
 *
 * Furthermore, the values for YEAR, MONTH, DAY, HOUR, MINUTE, SECOND
 * must be in the range 0..14 so that the associated bitmasks can fit
 * into the left half of an INTERVAL's typmod value.
 */

#define RESERV	0
#define MONTH	1
#define YEAR	2
#define DAY		3
#define JULIAN	4
#define TZ		5
#define DTZ		6
#define DTZMOD	7
#define IGNORE_DTF	8
#define AMPM	9
#define HOUR	10
#define MINUTE	11
#define SECOND	12
#define DOY		13
#define DOW		14
#define UNITS	15
#define ADBC	16
/* these are only for relative dates */
#define AGO		17
#define ABS_BEFORE		18
#define ABS_AFTER		19
/* generic fields to help with parsing */
#define ISODATE 20
#define ISOTIME 21
/* reserved for unrecognized string values */
#define UNKNOWN_FIELD	31

/*
 * Token field definitions for time parsing and decoding.
 * These need to fit into the datetkn table type.
 * At the moment, that means keep them within [-127,127].
 * These are also used for bit masks in DecodeDateDelta()
 *	so actually restrict them to within [0,31] for now.
 * - thomas 97/06/19
 * Not all of these fields are used for masks in DecodeDateDelta
 *	so allow some larger than 31. - thomas 1997-11-17
 */

#define DTK_NUMBER		0
#define DTK_STRING		1

#define DTK_DATE		2
#define DTK_TIME		3
#define DTK_TZ			4
#define DTK_AGO			5

#define DTK_SPECIAL		6
#define DTK_INVALID		7
#define DTK_CURRENT		8
#define DTK_EARLY		9
#define DTK_LATE		10
#define DTK_EPOCH		11
#define DTK_NOW			12
#define DTK_YESTERDAY	13
#define DTK_TODAY		14
#define DTK_TOMORROW	15
#define DTK_ZULU		16

#define DTK_DELTA		17
#define DTK_SECOND		18
#define DTK_MINUTE		19
#define DTK_HOUR		20
#define DTK_DAY			21
#define DTK_WEEK		22
#define DTK_MONTH		23
#define DTK_QUARTER		24
#define DTK_YEAR		25
#define DTK_DECADE		26
#define DTK_CENTURY		27
#define DTK_MILLENNIUM	28
#define DTK_MILLISEC	29
#define DTK_MICROSEC	30
#define DTK_JULIAN		31

#define DTK_DOW			32
#define DTK_DOY			33
#define DTK_TZ_HOUR		34
#define DTK_TZ_MINUTE	35


/*
 * Bit mask definitions for time parsing.
 */

#define DTK_M(t)		(0x01 << (t))

#define DTK_DATE_M		(DTK_M(YEAR) | DTK_M(MONTH) | DTK_M(DAY))
#define DTK_TIME_M		(DTK_M(HOUR) | DTK_M(MINUTE) | DTK_M(SECOND))

#define MAXDATELEN		51		/* maximum possible length of an input
								 * date string (not counting tr. null) */
#define MAXDATEFIELDS	25		/* maximum possible number of fields in a
								 * date string */
#define TOKMAXLEN		10		/* only this many chars are stored in
								 * datetktbl */

/* keep this struct small; it gets used a lot */
typedef struct
{
#if defined(_AIX)
	char	   *token;
#else
	char		token[TOKMAXLEN];
#endif   /* _AIX */
	char		type;
	char		value;			/* this may be unsigned, alas */
} datetkn;


/* FMODULO()
 * Macro to replace modf(), which is broken on some platforms.
 * t = input and remainder
 * q = integer part
 * u = divisor
 */
#define FMODULO(t,q,u) \
do { \
	q = ((t < 0) ? ceil(t / u): floor(t / u)); \
	if (q != 0) t -= rint(q * u); \
} while(0)

/* TMODULO()
 * Like FMODULO(), but work on the timestamp datatype (either int64 or float8).
 * We assume that int64 follows the C99 semantics for division (negative
 * quotients truncate towards zero).
 */
#ifdef HAVE_INT64_TIMESTAMP
#define TMODULO(t,q,u) \
do { \
	q = (t / u); \
	if (q != 0) t -= (q * u); \
} while(0)
#else
#define TMODULO(t,q,u) \
do { \
	q = ((t < 0) ? ceil(t / u): floor(t / u)); \
	if (q != 0) t -= rint(q * u); \
} while(0)
#endif

/* Global variable holding time zone information. */
#if !defined(__CYGWIN__) && !defined(WIN32)
#define TIMEZONE_GLOBAL timezone
#else
#define TIMEZONE_GLOBAL _timezone
#define tzname _tzname			/* should be in time.h? */
#endif

/*
 * Date/time validation
 * Include check for leap year.
 */
#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

/* Julian date support for date2j() and j2date()
 *
 * IS_VALID_JULIAN checks the minimum date exactly, but is a bit sloppy
 * about the maximum, since it's far enough out to not be especially
 * interesting.
 */

#define JULIAN_MINYEAR (-4713)
#define JULIAN_MINMONTH (11)
#define JULIAN_MINDAY (24)
#define JULIAN_MAXYEAR (5874898)

#define IS_VALID_JULIAN(y,m,d) ((((y) > JULIAN_MINYEAR) \
  || (((y) == JULIAN_MINYEAR) && (((m) > JULIAN_MINMONTH) \
  || (((m) == JULIAN_MINMONTH) && ((d) >= JULIAN_MINDAY))))) \
 && ((y) < JULIAN_MAXYEAR))

#define UTIME_MINYEAR (1901)
#define UTIME_MINMONTH (12)
#define UTIME_MINDAY (14)
#define UTIME_MAXYEAR (2038)
#define UTIME_MAXMONTH (01)
#define UTIME_MAXDAY (18)

#define IS_VALID_UTIME(y,m,d) ((((y) > UTIME_MINYEAR) \
 || (((y) == UTIME_MINYEAR) && (((m) > UTIME_MINMONTH) \
  || (((m) == UTIME_MINMONTH) && ((d) >= UTIME_MINDAY))))) \
 && (((y) < UTIME_MAXYEAR) \
 || (((y) == UTIME_MAXYEAR) && (((m) < UTIME_MAXMONTH) \
  || (((m) == UTIME_MAXMONTH) && ((d) <= UTIME_MAXDAY))))))

#ifdef HAVE_INT64_TIMESTAMP

#define DT_NOBEGIN		(-INT64CONST(0x7fffffffffffffff) - 1)
#define DT_NOEND		(INT64CONST(0x7fffffffffffffff))

#else

#ifdef HUGE_VAL
#define DT_NOBEGIN		(-HUGE_VAL)
#define DT_NOEND		(HUGE_VAL)
#else
#define DT_NOBEGIN		(-DBL_MAX)
#define DT_NOEND		(DBL_MAX)
#endif
#endif   /* HAVE_INT64_TIMESTAMP */

#define TIMESTAMP_NOBEGIN(j)	do {j = DT_NOBEGIN;} while (0)
#define TIMESTAMP_NOEND(j)			do {j = DT_NOEND;} while (0)
#define TIMESTAMP_IS_NOBEGIN(j) ((j) == DT_NOBEGIN)
#define TIMESTAMP_IS_NOEND(j)	((j) == DT_NOEND)
#define TIMESTAMP_NOT_FINITE(j) (TIMESTAMP_IS_NOBEGIN(j) || TIMESTAMP_IS_NOEND(j))

int DecodeTimeOnly(char **field, int *ftype,
			   int nf, int *dtype,
			   struct tm * tm, fsec_t *fsec, int *tzp);

int DecodeInterval(char **field, int *ftype,
			   int nf, int *dtype,
			   struct tm * tm, fsec_t *fsec);

int			EncodeTimeOnly(struct tm * tm, fsec_t fsec, int *tzp, int style, char *str);
int			EncodeDateTime(struct tm * tm, fsec_t fsec, int *tzp, char **tzn, int style, char *str, bool);
int			EncodeInterval(struct tm * tm, fsec_t fsec, int style, char *str);

int			tm2timestamp(struct tm *, fsec_t, int *, timestamp *);

int			DecodeUnits(int field, char *lowtoken, int *val);

bool		CheckDateTokenTables(void);

int			EncodeDateOnly(struct tm *, int, char *, bool);
void		GetEpochTime(struct tm *);
int			ParseDateTime(char *, char *, char **, int *, int, int *, char **);
int			DecodeDateTime(char **, int *, int, int *, struct tm *, fsec_t *, int *, bool);
void		j2date(int, int *, int *, int *);
void		GetCurrentDateTime(struct tm *);
int			date2j(int, int, int);

extern char *pgtypes_date_weekdays_short[];
extern char *pgtypes_date_months[];
extern char *months[];
extern char *days[];

#endif   /* DT_H */
