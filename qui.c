/*
 qui.c

 Date Created: Sun Nov 27 20:56:46 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Prototypes */
static int parse_proc_file (const char *);
static int parse_proc_line (const char *, const char *);
static int parse_sockaddr (const char **, const char *,
			   struct sockaddr_storage *);
static int convert_sockaddr_in (const char *, const char *, uint16_t,
				struct sockaddr_in *);
static int convert_sockaddr_in6 (const char *, const char *, uint16_t,
				 struct sockaddr_in6 *);
static int parse_addr_port (const char **, const char *,
			    const char **, const char **,
			    uint16_t *);
static int parse_dec (const char **, const char *,
		      const char **, const char **);
static int parse_hex (const char **, const char *,
		      const char **, const char **);
static int parse_hex_u32 (const char **, const char *, uint32_t *);
static int parse_hex_u16 (const char **, const char *, uint16_t *);
static int skip_colon (const char **, const char *);
static int skip_spaces (const char **, const char *);
static const char * pretty_sockaddr (struct sockaddr *);
static const char * pretty_sockaddr_ipv4 (struct sockaddr_in *, char *);
static const char * pretty_sockaddr_ipv6 (struct sockaddr_in6 *, char *);

#define BUFSIZE 4096

int debug = 0;

uint32_t threshold = 0;

int
main ()
{
  const char *procfiles[] = {"/proc/net/udp",
			    "/proc/net/udp6",
			    "/proc/net/tcp",
			    "/proc/net/tcp6",
			    0};
  const char **procfile;

  for (procfile = &procfiles[0];
       *procfile != 0;
       ++procfile)
    {
      if (parse_proc_file (*procfile) != 0)
	{
	  fprintf (stderr, "Error parsing %s\n", *procfile);
	  exit (42);
	}
    }
  return 0;
}

static int
parse_proc_file (procfile)
     const char *procfile;
{
  int fd;
  struct stat st;
  char buf[BUFSIZE];
  ssize_t len;

  fd = open (procfile, 0);
  if (fd == -1)
    {
      fprintf (stderr, "Error opening %s: %s\n",
	       procfile, strerror (errno));
      return -1;
    }
  if (stat (procfile, &st) == -1)
    {
      fprintf (stderr, "stat() error for %s: %s\n",
	       procfile, strerror (errno));
      return -1;
    }
  {
    const char *cp = buf;
    const char *lim, *tokstart, *linestart;

    len = read (fd, buf, BUFSIZE);
    if (len <= 0)
      {
	fprintf (stderr, "Error reading from %s: %s\n",
		 procfile, strerror (errno));
	return -1;
      }
    lim = buf + len;

    /* Parse header line */
    linestart = cp;
    skip_spaces (&cp, lim);
    while (cp < lim && *cp != '\n')
      {
	tokstart = cp;
	while (cp < lim && *cp != '\n' && *cp != ' ')
	  ++cp;
	if (debug)
	  {
	    fprintf (stderr, "%2d: %*.*s\n",
		     (int) (tokstart-linestart),
		     (int) (cp-tokstart), (int) (cp-tokstart), tokstart);
	  }
	skip_spaces (&cp, lim);
      }
    if (cp >= lim)
      {
	fprintf (stderr, "Header line too long\n");
	return -1;
      }
    else
      {
	++cp;			/* skip newline */
      }

    /* Parse more lines... */
    for (;;)
      {
	linestart = cp;
	while (cp < lim && *cp != '\n') ++cp;
	if (cp >= lim)
	  {
	    if (linestart == buf)
	      {
		fprintf (stderr, "Cannot buffer\n");
		return -1;
	      }
	    else
	      {
		memcpy (buf, linestart, lim-linestart);
		cp -= (linestart - buf);
		linestart = buf;
		len = read (fd, (char *) cp, BUFSIZE - (cp - buf));
		if (len <= 0)
		  {
		    fprintf (stderr, "Error reading from %s: %s\n",
			     procfile, strerror (errno));
		    return -1;
		  }
		lim = cp + len;
	      }
	  }
	while (cp < lim && *cp != '\n') ++cp;
	if (cp >= lim)
	  {
	    fprintf (stderr, "Failed to read newline\n");
	    return -1;
	  }
	if (parse_proc_line (linestart, cp) == -1)
	  {
	    return -1;
	  }
	++cp;
	if (cp == lim)
	  break;
      }
  }
  if (close (fd) == -1)
    {
      fprintf (stderr, "Error closing %s: %s\n",
	       procfile, strerror (errno));
      return -1;
    }
  return 0;
}

static int
parse_proc_line (start, end)
     const char *start, *end;
{
  const char *cp = start;
  const char *sl_s, *sl_e;		/* internal hash */
  struct sockaddr_storage la;		/* local addr */
  struct sockaddr_storage ra;		/* remote addr */
  const char *st_s, *st_e;		/* internal status */
  uint32_t oq, iq;			/* out/in queues */

  skip_spaces (&cp, end);
  if (parse_dec (&cp, end, &sl_s, &sl_e) == -1)
    return -1;
  sl_s = cp;
  while (cp < end && isdigit (*cp))
    ++cp;
  sl_e = cp;
  if (skip_colon (&cp, end) == -1)
    return -1;
  ++cp;
  skip_spaces (&cp, end);
  if (parse_sockaddr (&cp, end, &la) == -1)
    return -1;
  skip_spaces (&cp, end);
  if (parse_sockaddr (&cp, end, &ra) == -1)
    return -1;
  skip_spaces (&cp, end);
  if (parse_hex (&cp, end, &st_s, &st_e) == -1)
    return -1;
  skip_spaces (&cp, end);
  if (parse_hex_u32 (&cp, end, &oq) == -1)
    return -1;
  if (skip_colon (&cp, end) == -1)
    return -1;
  if (parse_hex_u32 (&cp, end, &iq) == -1)
    return -1;
  if (iq >= threshold)
    {
      fprintf (stdout, "%s Q: %lu\n",
	       pretty_sockaddr ((struct sockaddr *) &la),
	       (unsigned long) iq);
    }
  
  return 0;
}

static int
parse_sockaddr (cpp, end, ap)
     const char **cpp;
     const char *end;
     struct sockaddr_storage *ap;
{
  const char *as, *ae;
  uint16_t port;
  if (parse_addr_port (cpp, end, &as, &ae, &port) == -1)
    return -1;
  if (ae-as == 8)
    {
      return convert_sockaddr_in (as, ae, port, (struct sockaddr_in *) ap);
    }
  else if (ae-as == 32)
    {
      return convert_sockaddr_in6 (as, ae, port, (struct sockaddr_in6 *) ap);
    }
  else
    {
      fprintf (stderr, "Unknown address length %d\n",
	       (int) (ae-as));
      return -1;
    }  
}

static int
convert_sockaddr_in (as, ae, port, ap)
     const char *as;
     const char *ae;
     uint16_t port;
     struct sockaddr_in *ap;
{
  const char *endp = ae;

  memset (ap, 0, sizeof (struct sockaddr_in));
  ap->sin_family = AF_INET;
  ap->sin_port = htons (port);
  ap->sin_addr.s_addr = strtoul (as, &endp, 16);
#if 0
  fprintf (stderr, "IPv4 address: %*.*s\n",
	   (int) (ae-as), (int) (ae-as), as);
#endif /* 0 */
  return 0;
 
}

static int
convert_sockaddr_in6 (as, ae, port, ap)
     const char *as;
     const char *ae;
     uint16_t port;
     struct sockaddr_in6 *ap;
{
  memset (ap, 0, sizeof (struct sockaddr_in6));
  ap->sin6_family = AF_INET6;
  ap->sin6_port = htons (port);
  fprintf (stderr, "IPv6 address: %*.*s\n",
	   (int) (ae-as), (int) (ae-as), as);
  return 0;
 
}

static int
parse_addr_port (cpp, end, as, ae, pp)
     const char **cpp;
     const char *end;
     const char **as, **ae;
     uint16_t *pp;
{
  const char *cp = *cpp;
  if (parse_hex (&cp, end, as, ae) == -1)
    return -1;
  if (cp >= end || *cp != ':')
    {
      fprintf (stderr, "`:' expected\n");
      return -1;
    }
  ++cp;
  if (parse_hex_u16 (&cp, end, pp) == -1)
    return -1;
  *cpp = cp;
  return 0;
}

static int
parse_hex_u32 (cpp, end, ulp)
     const char **cpp;
     const char *end;
     uint32_t *ulp;
{
  const char *s, *e;
  char *e2;
  if (parse_hex (cpp, end, &s, &e) == -1)
    return -1;
  *ulp = strtoul (s, &e2, 16);
  if (e2 != e)
    {
      fprintf (stderr, "OUCH\n");
      return -1;
    }
  return 0;
}

static int
parse_hex_u16 (cpp, end, usp)
     const char **cpp;
     const char *end;
     uint16_t *usp;
{
  uint32_t ul;
  if (parse_hex_u32 (cpp, end, &ul) == -1)
    return -1;
  *usp = (uint16_t) ul;
  return 0;
}

static int
parse_dec (cpp, end, s, e)
     const char **cpp;
     const char *end;
     const char **s, **e;
{
  const char *cp = *cpp;
  *s = cp;
  while (cp < end && isdigit (*cp))
    ++cp;
  *e = cp;
  if (s == e)
    {
      fprintf (stderr, "No digits found\n");
      return -1;
    }
  *cpp = cp;
  return 0;
}

static int
parse_hex (cpp, end, s, e)
     const char **cpp;
     const char *end;
     const char **s, **e;
{
  const char *cp = *cpp;
  *s = cp;
  while (cp < end && isxdigit (*cp))
    ++cp;
  *e = cp;
  if (s == e)
    {
      fprintf (stderr, "No hex digits found\n");
      return -1;
    }
  *cpp = cp;
  return 0;
}

static int
skip_colon (cpp, end)
     const char **cpp;
     const char *end;
{
  const char *cp = *cpp;
  if (cp >= end || *cp != ':')
    {
      fprintf (stderr, "`:' expected\n");
      return -1;
    }
  *cpp = cp;
  return 0;
}

static int
skip_spaces (cpp, end)
     const char **cpp;
     const char *end;
{
  const char *cp = *cpp;
  while (cp < end && *cp == ' ')
    ++cp;
  *cpp = cp;
  return 0;
}

#define MAX_PRETTY_SOCKADDR INET6_ADDRSTRLEN

static const char *
pretty_sockaddr (struct sockaddr *sa)
{
  char buf[MAX_PRETTY_SOCKADDR];

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
  if (inet_ntop (AF_INET, sa, buf, sizeof (struct sockaddr_in)) == 0)
    {
      return 0;
    }
  return buf;
}

static const char *
pretty_sockaddr_ipv6 (struct sockaddr_in6 *sa, char *buf)
{
  if (inet_ntop (AF_INET6, sa, buf, sizeof (struct sockaddr_in6)) == 0)
    {
      return 0;
    }
  return buf;
}
