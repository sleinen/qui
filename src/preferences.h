/*
 preferences.h

 Date Created: Fri Dec  9 19:00:46 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#ifndef __QUI_PREFERENCES_H__
#define __QUI_PREFERENCES_H__

typedef struct PreferencesRec *Preferences;

typedef struct PreferencesRec
{
  int specific_port;
  uint16_t portno;
  int want_tcp;
  int want_udp;
  int want_ipv4;
  int want_ipv6;
  unsigned rounds;
  int print_usecs;
  unsigned threshold;
  int close_proc_after_reading;
  int debug;
}
PreferencesRec, *Preferences;

#endif /* not __QUI_PREFERENCES_H__ */
