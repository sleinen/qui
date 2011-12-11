/*
 parse-args.c

 Date Created: Fri Dec  9 18:59:58 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include "preferences.h"
#include "parse-args.h"

static int convert_unsigned (const char *, unsigned *, const char *);
static int convert_u16 (const char *, uint16_t *, const char *);
static void init_prefs (Preferences);
static void usage (const char *);

void
parse_args (argc, argv, p)
     int argc;
     char **argv;
     Preferences p;
{
  long long_arg;
  double double_arg;
  const struct option opts[] = {
    { "threshold", required_argument, 0, 't',},
    { "sleep", required_argument, 0, 's',},
    { "tcp", no_argument, 0, 'T',},
    { "udp", no_argument, 0, 'U',},
    { "ipv4", no_argument, 0, '4',},
    { "ipv6", no_argument, 0, '6',},
    { "port", required_argument, 0, 'p',},
    { "input", no_argument, 0, 'i',},
    { "output", no_argument, 0, 'o',},
    { "microseconds", no_argument, 0, 'm',},
    { "blip-size", required_argument, 0, 'b',},
    { "close", no_argument, 0, 'c',},
    { "debug", no_argument, 0, 'd',},
    { "help", no_argument, 0, 'h',},
    { 0, 0, 0, 0, },
  };
  int opt;
  char *end;

  init_prefs (p);
  while ((opt = getopt_long (argc, argv, "t:s:b:TU46p:iomcdh", opts, 0)) != -1)
    {
      switch (opt) {
      case 'T': p->want_tcp = 1; break;
      case 'U': p->want_udp = 1; break;
      case '4': p->want_ipv4 = 1; break;
      case '6': p->want_ipv6 = 1; break;
      case 'i': p->want_input = 1; break;
      case 'o': p->want_output = 1; break;
      case 'm': p->print_usecs = 1; break;
      case 'c': p->close_proc_after_reading = 1; break;
      case 'd': p->debug = 1; break;
      case 't':
	if (convert_unsigned (optarg, &p->threshold, "threshold") != 0)
	  exit (1);
	break;
      case 's':
	if ((double_arg = strtod (optarg, &end)) < 0
	    || end == optarg || *end != 0)
	  {
	    fprintf (stderr, "Malformed sleep time %s\n", optarg);
	    exit (1);
	  }
	else
	  {
	    p->sleeptime.tv_sec = double_arg / 1000;
	    p->sleeptime.tv_nsec =
	      (double_arg - (p->sleeptime.tv_sec * 1000)) * 1000000;
	  }
	break;
      case 'b':
	if (convert_unsigned (optarg, &p->blipsize, "blip size") != 0)
	  exit (1);
	if (p->blipsize < 1)
	  {
	    fprintf (stderr, "Blip size must be >0\n");
	    exit (1);
	  }
	break;
      case 'p':
	if (convert_u16 (optarg, &p->portno, "port number") != 0)
	  exit (1);
	p->specific_port = 1;
	break;
      case 'h':
	usage (argv[0]);
	exit (0);
      }
    }
  if (!p->want_udp && !p->want_tcp)
    {
      p->want_udp = p->want_tcp = 1;
    }
  if (!p->want_ipv4 && !p->want_ipv6)
    {
      p->want_ipv4 = p->want_ipv6 = 1;
    }
  if (!p->want_input && !p->want_output)
    {
      p->want_input = p->want_output = 1;
    }
}

static int
convert_unsigned (arg, valp, name)
     const char *arg;
     unsigned *valp;
     const char *name;
{
  long long_arg;
  char *end;

  if ((long_arg = strtol (arg, &end, 10)) < 0
      || end == arg || *end != 0)
    {
      fprintf (stderr, "Malformed %s %s\n", name, arg);
      return -1;
    }
  else
    {
      *valp = long_arg;
      return 0;
    }
}

static int
convert_u16 (arg, valp, name)
     const char *arg;
     uint16_t *valp;
     const char *name;
{
  unsigned val;

  if (convert_unsigned (arg, &val, name) != 0)
    return -1;
  if (val > 65535)
    {
      fprintf (stderr, "value for %s too big: %u\n",
	       name, val);
      return -1;
    }
  else
    {
      *valp = val;
      return 0;
    }
}

static const unsigned default_threshold = 2000;
static const unsigned default_blipsize = 50000;
static const unsigned default_sleep = 10;

static void
init_prefs (p)
     Preferences p;
{
  p->want_tcp = 0;
  p->want_udp = 0;
  p->want_ipv4 = 0;
  p->want_ipv6 = 0;
  p->want_input = 0;
  p->want_output = 0;
  p->specific_port = 0;
  p->print_usecs = 0;
  p->threshold = default_threshold;
  p->blipsize = default_blipsize;
  p->sleeptime = default_sleep;
  p->close_proc_after_reading = 0;
  p->debug = 0;
}

static void
usage (progname)
     const char *progname;
{
  fprintf (stderr, "Usage: %s [--threshold BYTES] [--sleep MILLISECONDS]\n"
	   "\t  [--tcp|-T] [--udp|-U] [--ipv4|-4] [--ipv6|-6]\n"
	   "\t  [--input|-i] [--output|-o]\n"
	   "\t  [--port PORT|-p PORT] [--microseconds|-m]\n"
	   "\t  [--debug|-d] [--help|-h]\n",
	   progname);
}
