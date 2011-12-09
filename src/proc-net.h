/*
 proc-net.h

 Date Created: Fri Dec  9 19:07:17 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#ifndef __QUI_PROC_NET_H__
#define __QUI_PROC_NET_H__ 1

#include <sys/socket.h>
#include <netinet/in.h>

typedef struct ProcFileEntry *ProcFileEntry;

typedef struct ProcFileEntry
{
  struct sockaddr_storage	la;
  struct sockaddr_storage	ra;
  uint32_t			iq;
  uint32_t			oq;
}
ProcFileEntryRec;

typedef void (* SockEntryCallback)
  (ProcFileEntry, const struct timeval *, void *);

extern int parse_proc_files (Preferences, SockEntryCallback, void *);

#endif /* not __QUI_PROC_NET_H__ */
