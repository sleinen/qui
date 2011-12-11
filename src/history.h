/*
 history.h

 Date Created: Fri Dec  9 20:45:04 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#ifndef __QUI_HISTORY_H__
#define __QUI_HISTORY_H__

#include <sys/time.h>
#include <stdint.h>

typedef struct BufferEventRec *BufferEvent;
typedef struct BufferSampleRec *BufferSample;
typedef struct BufferHistoryRec *BufferHistory;

typedef struct BufferEventRec {
  struct timeval  s_ts;		/* start timestamp */
  uint32_t	  s_occ;	/* min. occupancy */
  struct timeval  m_ts;		/* peak timestamp */
  uint32_t	  m_occ;	/* max. occupancy */
  struct timeval  e_ts;		/* end timestamp */
  uint32_t	  e_occ;	/* max. occupancy */
} BufferEventRec;

typedef struct BufferSampleRec {
  struct timeval  ts;		/* timestamp */
  uint32_t	  occ;		/* occupancy */
} BufferSampleRec;

typedef struct BufferHistoryRec {
  unsigned	  n_samples;	/* space for samples */
  unsigned	  first_sample;
  unsigned	  last_sample;
  BufferSample	  samples;

  unsigned	  n_events;	/* space for events */
  unsigned	  first_event;
  unsigned	  last_event;
  BufferEvent	  events;
} BufferHistoryRec;

extern BufferHistory make_buffer_history (unsigned, unsigned);
extern int destroy_buffer_history (BufferHistory);
extern int insert_sample (BufferHistory, struct timeval *, uint32_t);
extern int consume_active_samples (BufferHistory, int (*) (BufferHistory, unsigned, void *), void *);
extern int map_active_samples (BufferHistory, int (*) (BufferHistory, unsigned, void *), void *);
extern int map_active_samples_1 (BufferHistory, int (*) (BufferHistory, unsigned, void *), void *, unsigned, unsigned, int);

#endif /* not __QUI_HISTORY_H__ */
