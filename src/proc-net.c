/*
 proc-net.c

 Date Created: Fri Dec  9 19:07:13 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>

 Parse /proc/net/{udp,tcp} and /proc/net/{udp6,tcp6}

 These files all have more or less the same structure.

 The functions here work in the way that they traverse the /proc/net
 files and, parse each line into a data structure, and for each entry,
 call a user-provided "callback" function on that data structure.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "preferences.h"
#include "proc-net.h"

typedef struct ProcFileRec *ProcFile;

static int relevant_procfile_p (ProcFile, Preferences);
static int parse_proc_file (ProcFile, Preferences, SockEntryCallback, void *);
static int parse_proc_line (const char *, const char *,
			    const struct timeval *, Preferences,
			    SockEntryCallback, void *);
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

#define BUFSIZE 4096

typedef struct ProcFileRec
{
  const char  *	pathname;
  int		fd;
  int		af;
  int		proto;
}
ProcFileRec;

ProcFileRec
procfiles[] = {
  { .pathname = "/proc/net/udp",
    .fd	      = -1,
    .af	      = AF_INET,
    .proto    = IPPROTO_UDP,
  },
  { .pathname = "/proc/net/udp6",
    .fd	      = -1,
    .af	      = AF_INET6,
    .proto    = IPPROTO_UDP,
  },
  { .pathname = "/proc/net/tcp",
    .fd	      = -1,
    .af	      = AF_INET,
    .proto    = IPPROTO_TCP,
  },
  { .pathname = "/proc/net/tcp6",
    .fd	      = -1,
    .af	      = AF_INET6,
    .proto    = IPPROTO_TCP,
  },
  { .pathname = 0,
    .fd	      = -1,
    .af	      = 0,
    .proto    = 0,
  }
};

int
parse_proc_files (p, callback, closure)
     Preferences p;
     SockEntryCallback callback;
     void *closure;
{
  ProcFile procfile;

  for (procfile = &procfiles[0]; procfile->pathname != 0; ++procfile)
    {
      if (relevant_procfile_p (procfile, p))
	{
	  if (parse_proc_file (procfile, p, callback, closure) != 0)
	    {
	      fprintf (stderr, "error parsing %s\n", procfile->pathname);
	      return -1;
	    }
	}
    }
  return 0;
}

static int
relevant_procfile_p (procfile, p)
     ProcFile procfile;
     Preferences p;
{
  switch (procfile->proto)
    {
    case IPPROTO_TCP:
      if (!p->want_tcp)
	return 0;
      break;
    case IPPROTO_UDP:
      if (!p->want_udp)
	return 0;
      break;
    }
  switch (procfile->af)
    {
    case AF_INET:
      if (!p->want_ipv4)
	return 0;
      break;
    case AF_INET6:
      if (!p->want_ipv6)
	return 0;
      break;
    }
  return 1;
}

static int
parse_proc_file (procfile, p, callback, closure)
     ProcFile procfile;
     Preferences p;
     SockEntryCallback callback;
     void *closure;
{
  int fd;
  struct stat st;
  char buf[BUFSIZE];
  ssize_t len;
  struct timeval tv;

  if (gettimeofday (&tv, 0) == -1)
    {
      fprintf (stderr, "Failed to get time of day\n");
      return -1;
    }
  if ((fd = procfile->fd) == -1)
    {
      fd = open (procfile->pathname, 0);
      if (fd == -1)
	{
	  fprintf (stderr, "Error opening %s: %s\n",
		   procfile->pathname, strerror (errno));
	  return -1;
	}
      else
	{
	  procfile->fd = fd;
	}
    }
  else
    {
      if (lseek (fd, 0, SEEK_SET) == (off_t) -1)
	{
	  fprintf (stderr, "Error rewinding %s: %s\n",
		   procfile->pathname, strerror (errno));
	  return -1;
	}
    }
  if (stat (procfile->pathname, &st) == -1)
    {
      fprintf (stderr, "stat() error for %s: %s\n",
	       procfile->pathname, strerror (errno));
      return -1;
    }
  {
    const char *cp = buf;
    const char *lim, *tokstart, *linestart;

    len = read (fd, buf, BUFSIZE);
    if (len <= 0)
      {
	fprintf (stderr, "Error reading from %s: %s\n",
		 procfile->pathname, strerror (errno));
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
	if (p->debug)
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
			     procfile->pathname, strerror (errno));
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
	if (parse_proc_line (linestart, cp, &tv, p, callback, closure) == -1)
	  {
	    return -1;
	  }
	++cp;
	if (cp == lim)
	  break;
      }
  }
  if (p->close_proc_after_reading)
    {
      if (close (fd) == -1)
	{
	  fprintf (stderr, "Error closing %s: %s\n",
		   procfile->pathname, strerror (errno));
	  return -1;
	}
      procfile->fd = -1;
    }
  return 0;
}

static int
parse_proc_line (start, end, tv, p, callback, closure)
     const char *start, *end;
     const struct timeval *tv;
     Preferences p;
     SockEntryCallback callback;
     void *closure;
{
  const char *cp = start;
  const char *sl_s, *sl_e;		/* internal hash */
  ProcFileEntryRec pfe;
  const char *st_s, *st_e;		/* internal status */

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
  if (parse_sockaddr (&cp, end, &(pfe.la)) == -1)
    return -1;
  skip_spaces (&cp, end);
  if (parse_sockaddr (&cp, end, &(pfe.ra)) == -1)
    return -1;
  skip_spaces (&cp, end);
  if (p->specific_port)
    {
      if (((struct sockaddr *) &(pfe.la))->sa_family == AF_INET)
	{
	  if (ntohs (((struct sockaddr_in *) &(pfe.la))->sin_port) != p->portno
	      && ntohs (((struct sockaddr_in *) &(pfe.ra))->sin_port) != p->portno)
	    {
	      return 0;
	    }
	}
      else if (((struct sockaddr *) &(pfe.la))->sa_family == AF_INET6)
	{
	  if (ntohs (((struct sockaddr_in6 *) &(pfe.la))->sin6_port) != p->portno
	      && ntohs (((struct sockaddr_in6 *) &(pfe.ra))->sin6_port) != p->portno)
	    {
	      return 0;
	    }
	}
    }
  if (parse_hex (&cp, end, &st_s, &st_e) == -1)
    return -1;
  skip_spaces (&cp, end);
  if (parse_hex_u32 (&cp, end, &(pfe.oq)) == -1)
    return -1;
  if (skip_colon (&cp, end) == -1)
    return -1;
  if (parse_hex_u32 (&cp, end, &(pfe.iq)) == -1)
    return -1;
  (* callback) (&pfe, tv, closure);
  
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
  ap->sin_addr.s_addr = strtoul (as, (char **) &endp, 16);
  if (endp != ae)
    {
      fprintf (stderr, "Unexpected: %08lx %08lx %08lx\n",
	       (unsigned long) as,
	       (unsigned long) ae,
	       (unsigned long) endp);
      return -1;
    }
  return 0;
 
}

static int
convert_sockaddr_in6 (as, ae, port, ap)
     const char *as;
     const char *ae;
     uint16_t port;
     struct sockaddr_in6 *ap;
{
  unsigned k;
  char buf[33];

  if (as + 32 != ae)
    {
      fprintf (stderr, "Unexpected length of IPv6 address encoding: %d\n",
	       (int) (ae-as));
      return -1;
    }
  memset (ap, 0, sizeof (struct sockaddr_in6));
  ap->sin6_family = AF_INET6;
  ap->sin6_port = htons (port);

#define BYTES_PER_CHUNK 4
#define CHUNKS_PER_ADDR 4
  for (k = 0; k < CHUNKS_PER_ADDR; ++k)
    {
      memset (buf, 0, BYTES_PER_CHUNK * 2 + 1);
      memcpy (buf, as + (k * BYTES_PER_CHUNK * 2), BYTES_PER_CHUNK * 2);
      ap->sin6_addr.s6_addr32[k] = strtoul (buf, 0, 16);
    }
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
  *cpp = cp + 1;
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
