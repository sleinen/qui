/*
 qui.c

 Date Created: Sun Nov 27 20:56:46 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "preferences.h"
#include "parse-args.h"
#include "proc-net.h"

/* Prototypes */
static void per_entry (ProcFileEntry,
		       const struct timeval *, void *);
static const char *pretty_sockaddr (struct sockaddr *, char *);
static const char *pretty_sockaddr_ipv4 (struct sockaddr_in *, char *);
static const char *pretty_sockaddr_ipv6 (struct sockaddr_in6 *, char *);
static char *strtime (const struct timeval *, Preferences);

#define MAX_SERVNAME 20

#define MAX_PRETTY_SOCKADDR (INET6_ADDRSTRLEN + MAX_SERVNAME + 3)

int close_proc_after_reading = 0;

int
main (argc, argv)
     int argc;
     char **argv;
{
  unsigned iter;
  PreferencesRec p;

  parse_args (argc, argv, &p);
  for (iter = 0; iter < p.rounds; ++iter)
    {
      parse_proc_files (&p, per_entry, &p);
    }
  return 0;
}

static void
per_entry (pfe, tv, closure)
     ProcFileEntry pfe;
     const struct timeval *tv;
     void *closure;
{
  Preferences p = (Preferences) closure;
  if (pfe->iq >= p->threshold || pfe->oq >= p->threshold)
    {
      char lap[MAX_PRETTY_SOCKADDR];
      char rap[MAX_PRETTY_SOCKADDR];
      pretty_sockaddr ((struct sockaddr *) &(pfe->la), lap);
      pretty_sockaddr ((struct sockaddr *) &(pfe->ra), rap);
      fprintf (stdout, "%s %s %s Q: %lu %lu\n",
	       strtime (tv, p),
	       lap, rap,
	       (unsigned long) pfe->iq,
	       (unsigned long) pfe->oq);
    }
}
static const char *
pretty_sockaddr (struct sockaddr *sa, char *buf)
{
  if (sa->sa_family == AF_INET)
    {
      return pretty_sockaddr_ipv4 ((struct sockaddr_in *) sa, buf);
    }
  else if (sa->sa_family == AF_INET6)
    {
      return pretty_sockaddr_ipv6 ((struct sockaddr_in6 *) sa, buf);
    }
  else
    {
      sprintf (buf, "<UNKNOWN-AF-%d>", sa->sa_family);
      return 0;
    }
}

static const char *
pretty_sockaddr_ipv4 (struct sockaddr_in *sa, char *buf)
{
  char servname[MAX_SERVNAME];
  int result;

  result = getnameinfo ((struct sockaddr *) sa, sizeof (struct sockaddr_in),
			buf, MAX_PRETTY_SOCKADDR,
			servname, MAX_SERVNAME,
			NI_NUMERICHOST|NI_NUMERICSERV);
  if (result == 0)
    {
      char *cp = buf + strlen (buf);
      sprintf (cp, ":%s", servname);
      return buf;
    }
  else
    {
      fprintf (stderr, "Error pretty-printing IPv6 address: %s\n",
	       gai_strerror (result));
      return 0;
    }
}

static const char *
pretty_sockaddr_ipv6 (struct sockaddr_in6 *sa, char *buf)
{
  char servname[MAX_SERVNAME];
  int result;

  buf[0] = '[';
  result = getnameinfo ((struct sockaddr *) sa, sizeof (struct sockaddr_in6),
			buf+1, MAX_PRETTY_SOCKADDR-1,
			servname, MAX_SERVNAME,
			NI_NUMERICHOST|NI_NUMERICSERV);
  if (result == 0)
    {
      char *cp = buf + strlen (buf);
      sprintf (cp, "]:%s", servname);
      return buf;
    }
  else
    {
      fprintf (stderr, "Error pretty-printing IPv6 address: %s\n",
	       gai_strerror (result));
      fprintf (stderr, "  AF = %d\n", (int) sa->sin6_family);
      return 0;
    }
}

static char *strtime (tv, p)
     const struct timeval *tv;
     Preferences p;
{
  static struct timeval cached_tv = { .tv_sec = 0, .tv_usec = 0 };
  time_t time;
  static struct tm cached_tm;
  static char timebuf[30];
  static char *sec_end;

  if (cached_tv.tv_sec != tv->tv_sec)
    {
      time = tv->tv_sec;
      if (localtime_r (&time, &cached_tm) == 0)
	{
	  fprintf (stderr, "Cannot convert time\n");
	  return 0;
	}
      strftime (timebuf, 30, "%H:%M:%S", &cached_tm);
      sec_end = timebuf+strlen (timebuf);
    }

  if (p->print_usecs)
    {
      sprintf (sec_end, ".%06lu", (unsigned long) tv->tv_usec);
    }
  else
    {
      sprintf (sec_end, ".%03lu", (unsigned long) tv->tv_usec/1000);
    }
  return timebuf;
}
