/*
 preferences.h

 Date Created: Fri Dec  9 19:00:46 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#ifndef __QUI_PREFERENCES_H__
#define __QUI_PREFERENCES_H__

#include <time.h>

typedef struct PreferencesRec *Preferences;

typedef struct PreferencesRec
{
  /* whether we are interested in TCP sockets */
  int		want_tcp;

  /* whether we are interested in UDP sockets */
  int		want_udp;

  /* whether we are interested in IPv4 sockets */
  int		want_ipv4;

  /* whether we are interested in IPv6 sockets */
  int		want_ipv6;

  /* whether we are interested socket with a specific local/remote
     port */
  int		specific_port;

  /* if so, which port */
  uint16_t	portno;

  /* whether we are interested in the receive-queue length */
  int		want_input;

  /* whether we are interested in the send-queue length */
  int		want_output;

  /* whether timestamps should be printed in microsecond (as opposed
     to millisecond) resolution */
  int		print_usecs;

  /* the queue must reach at least this size, in bytes, for an event
     to be accounted for. */
  unsigned	threshold;

  /* visualization unit for queue occupancy. */
  unsigned	blipsize;

  /* whether we should close and re-open the /proc/net files for every
     round.  If this is zero, the files will just be reset using
     lseek(0).  This was used to experiment with the performance
     implications of the different variants.  Since the lseek()
     variant seems to work reliably, and has somewhat lower overhead,
     the option will probably be removed. */
  int		close_proc_after_reading;

  /* debugging mode with more verbose output */
  int		debug;

  /* how long the tool should wait between rounds */
  struct timespec sleeptime;
}
PreferencesRec;

#endif /* not __QUI_PREFERENCES_H__ */
