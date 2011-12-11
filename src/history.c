/*
 history.c

 Date Created: Fri Dec  9 20:53:45 2011
 Author:       Simon Leinen  <simon.leinen@switch.ch>
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "history.h"

BufferHistory
make_buffer_history (n_samples, n_events)
     unsigned n_samples;
     unsigned n_events;
{
  BufferHistory hist;

  if ((hist = malloc (sizeof (BufferHistoryRec))) == 0
      || (hist->samples = (BufferSample) calloc (n_samples, sizeof (BufferSampleRec))) == 0
      || (hist->events = (BufferEvent) calloc (n_events, sizeof (BufferEventRec))) == 0) 
    {
      fprintf (stderr, "Out of memory\n");
      return 0;
    }
  hist->n_samples = n_samples;
  hist->first_sample = 0;
  hist->last_sample = 0;
  hist->n_events = n_events;
  hist->first_event = 0;
  hist->last_event = 0;
  return hist;
}

int
destroy_buffer_history (h)
     BufferHistory h;
{
  if (h->samples)
    {
      free (h->samples);
      h->samples = 0;
    }
  if (h->events)
    {
      free (h->events);
      h->events = 0;
    }
  free (h);
  return 0;
}

int
insert_sample (h, tv, val)
     BufferHistory h;
     struct timeval *tv;
     uint32_t val;
{
  BufferSample s;
  unsigned insert_pointer = h->last_sample;

  h->last_sample = insert_pointer + 1;
  if (h->last_sample == h->n_samples)
    {
      h->last_sample = 0;
    }
  if (h->last_sample == h->first_sample)
    {
      h->first_sample = (h->first_sample + 1 % h->n_samples);
    }
  s = &(h->samples[insert_pointer]);
  s->ts = *tv;
  s->occ = val;
  return 0;
}

int
consume_active_samples (h, mapfn, closure)
     BufferHistory h;
     int (* mapfn) (BufferHistory, unsigned, void *);
     void * closure;
{
  return map_active_samples_1 (h, mapfn, closure, h->first_sample, h->last_sample, 1);
}

int
map_active_samples (h, mapfn, closure)
     BufferHistory h;
     int (* mapfn) (BufferHistory, unsigned, void *);
     void * closure;
{
  return map_active_samples_1 (h, mapfn, closure, h->first_sample, h->last_sample, 0);
}

int
map_active_samples_1 (h, mapfn, closure, start, end, consume_p)
     BufferHistory h;
     int (* mapfn) (BufferHistory, unsigned, void *);
     void * closure;
     unsigned start, end;
     int consume_p;
{
  unsigned k;
  int result;

  for (k = start;
       k < end;
       k = ((k + 1) == h->n_samples) ? 0 : k + 1)
    {
      result = (* mapfn) (h, k, closure);
      if (result == -1)
	{
	  break;
	}
      else
	{
	  if (consume_p)
	    {
	      h->first_sample = ((k + 1) == h->n_samples) ? 0 : k + 1;
	    }
	}
    }
  return 0;
}
