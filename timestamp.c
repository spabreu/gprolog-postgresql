#include "postgres_fe.h"
#include <time.h>
#include <float.h>
#include <math.h>

#ifdef __FAST_MATH__
#error -ffast-math is known to break this code
#endif

//#include "extern.h"
//#include "dt.h"

#include <postgres.h>
#include <pgtypes_timestamp.h>
#include <pgtypes_date.h>


//#include <utils/datetime.h>

#ifdef HAVE_INT64_TIMESTAM

typedef int32 fsec_t;
 
#else
 
typedef double fsec_t;
 
#define TIME_PREC_INV 1000000.0
#define JROUND(j) (rint(((double) (j))*TIME_PREC_INV)/TIME_PREC_INV)
#endif


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
 
/* Julian-date equivalents of Day 0 in Unix and Postgres reckoning */
#define UNIX_EPOCH_JDATE                2440588 /* == date2j(1970, 1, 1) */
#define POSTGRES_EPOCH_JDATE    2451545 /* == date2j(2000, 1, 1) */



/*
 * Info about limits of the Unix time_t data type.  We assume that time_t
 * is a signed int32 with origin 1970-01-01.  Note this is only relevant
 * when we use the C library's time routines for timezone processing.
 */
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
        q = ((t < 0) ? ceil(t / u) : floor(t / u)); \
        if (q != 0) t -= rint(q * u); \
} while(0)
#endif
                                                                                

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

static timestamp
SetEpochTimestamp(void)
{
	timestamp	dt;
	struct tm	tt,
			   *tm = &tt;

	GetEpochTime(tm);
	tm2timestamp(tm, 0, NULL, &dt);
	return dt;
}	/* SetEpochTimestamp() */

static void
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
// compile-command: "gcc -c -I/usr/include/postgresql -I/usr/include/postgresql/internal timestamp.c"
// End:
