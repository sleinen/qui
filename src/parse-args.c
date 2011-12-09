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

static void init_prefs (Preferences);
static void usage (const char *);

void
parse_args (argc, argv, p)
     int argc;
     char **argv;
     Preferences p;
{
  long x;
  const struct option opts[] = {
    { "threshold",
      required_argument,
      0,
      't',
    },
    { "rounds",
      required_argument,
      0,
      'r',
    },
    { "tcp",
      no_argument,
      0,
      'T',
    },
    { "udp",
      no_argument,
      0,
      'U',
    },
    { "ipv4",
      no_argument,
      0,
      '4',
    },
    { "ipv6",
      no_argument,
      0,
      '6',
    },
    { "port",
      required_argument,
      0,
      'p',
    },
    { "input",
      no_argument,
      0,
      'i',
    },
    { "output",
      no_argument,
      0,
      'o',
    },
    { .name = "microseconds",
      .has_arg = no_argument,
      0,
      'm',
    },
    { .name = "close",
      .has_arg = no_argument,
      0,
      'c',
    },
    { .name = "debug",
      .has_arg = no_argument,
      0,
      'd',
    },
    { .name = "help",
      .has_arg = no_argument,
      0,
      'h',
    },
    { 0, 0, 0, 0, },
  };
  int opt;
  char *end;

  init_prefs (p);
  while ((opt = getopt_long (argc, argv, "t:r:TU46p:iomcdh", opts, 0)) != -1)
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
	if ((x = strtol (optarg, &end, 10)) < 0
	    || end == optarg || *end != 0)
	  {
	    fprintf (stderr, "Illegal threshold %s\n", optarg);
	    exit (1);
	  }
	else
	  {
	    p->threshold = x;
	  }
	break;
      case 'r':
	if ((x = strtol (optarg, &end, 10)) < 0
	    || end == optarg || *end != 0)
	  {
	    fprintf (stderr, "Illegal number of rounds %s\n", optarg);
	    exit (1);
	  }
	else
	  {
	    p->rounds = x;
	  }
	break;
      case 'p':
	if ((x = strtol (optarg, &end, 10)) < 0
	    || end == optarg || *end != 0
	    || x > 65535)
	  {
	    fprintf (stderr, "Illegal port number %s\n", optarg);
	    exit (1);
	  }
	else
	  {
	    p->specific_port = 1;
	    p->portno = x;
	  }
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
  p->rounds = 1;
  p->print_usecs = 0;
  p->threshold = 1;
  p->close_proc_after_reading = 0;
  p->debug = 0;
}

static void
usage (progname)
     const char *progname;
{
  fprintf (stderr, "Usage: %s [--threshold BYTES] [--rounds ROUNDS]\n"
	   "\t  [--tcp|-T] [--udp|-U] [--ipv4|-4] [--ipv6|-6]\n"
	   "\t  [--input|-i] [--output|-o]\n"
	   "\t  [--port PORT|-p PORT] [--microseconds|-m]\n"
	   "\t  [--debug|-d] [--help|-h]\n",
	   progname);
}
