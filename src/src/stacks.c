/* Copyright (C) 1987, Barak Pearlmutter & Kevin Lang. */

#include <stdio.h>
#include "emulator.h"
#include "stacks.h"

#ifndef FAST
extern bool trace_segs;
#endif

/* This routine flushes out the stack buffer, leaving amount_to_leave. */
void flush_buffer(stack *pstk, const int amount_to_leave)
{
  segment *s;
  long i , count = pstk->ptr - (&pstk->data[0]-1)
    , amount_to_flush = count - amount_to_leave
      , amount_unflushed = amount_to_flush;
  ref *src = &pstk->data[0]
    , *end = pstk->ptr - amount_to_leave;

#ifndef FAST
  if (trace_segs) printf("seg:flush-");
#endif
  
  while (src <= end)   /* was (scr < end) ... this was the call/cc bug */
    {
      /* Flush a single segment. */
      long size = amount_unflushed;

      if (size > MAX_SEGMENT_SIZE)
	size = MAX_SEGMENT_SIZE;
      
      {
	ref *p;

	ALLOCATE(p, size+SEGMENT_HEADER_LENGTH,
		 "space crunch allocating stack segment");

	s = (segment *)p;
      }

      s->type_field = e_segment_type;
      s->length_field = INT_TO_REF(size+SEGMENT_HEADER_LENGTH);
      s->previous_segment = pstk->segment;
      pstk->segment = PTR_TO_REF(s);

      for (i=0; i<size; i++)
	s->data[i] = *src++;

      amount_unflushed -= size;
#ifndef FAST
      if (trace_segs) printf("%d-", size);
#endif
    }

  for (i=0; i<amount_to_leave; i++)
    pstk->data[i] = *src++;

  pstk->ptr = &pstk->data[amount_to_leave-1];
  pstk->pushed_count += amount_to_flush;
#ifndef FAST
  if (trace_segs) printf(".\n");
#endif
}



/* This routine grabs some segments that have been flushed from the buffer
   and puts them back in.  Because the segments might be small, it
   may have to put more than one segment back in.  It grabs enough so that
   the buffer has at least n+1 values in it, so that at least n values could
   be popped off without underflow. */

void unflush_buffer(stack *pstk, const int n)
{
  long i
    , number_to_pull = 0
    , count = pstk->ptr - (&pstk->data[0]-1)
    , new_count = count;
  segment *s = (segment *)REF_TO_PTR(pstk->segment);
  ref *dest;

#ifndef FAST
  if (trace_segs) printf("seg:unflush-");
#endif

  /* First, figure out how many segments to pull. */
  for (; new_count <= n; s = (segment *)REF_TO_PTR(s->previous_segment))
    {
      int this = REF_TO_INT(s->length_field) - SEGMENT_HEADER_LENGTH;
#ifndef FAST
      if (trace_segs) printf("%d-",this);
#endif
      new_count += this;
      number_to_pull += 1;
    }
      
#ifndef FAST
  if (trace_segs) printf("(%d)-",number_to_pull);
#endif

  /* Copy the data in the buffer up to its new home. */
  dest = &pstk->data[new_count - 1];

  for (i = count-1; i>=0; i--)
    *dest-- = pstk->data[i];

  /* Suck in the segments. */
  for (s = (segment *)REF_TO_PTR(pstk->segment);
       number_to_pull>0; number_to_pull--)
    {
      /* Suck in this segment. */
      for (i = REF_TO_INT(s->length_field) - SEGMENT_HEADER_LENGTH - 1
	   ; i >= 0; i--)
	*dest-- = s->data[i];
      s = (segment *)REF_TO_PTR(s->previous_segment);
#ifndef FAST
      if (trace_segs) printf("p");
#endif
    }

  pstk->segment = PTR_TO_REF(s);
  pstk->ptr = &pstk->data[new_count-1];
  pstk->pushed_count -= new_count - count;
#ifndef FAST
  if (trace_segs) printf(".\n");
#endif
}

      



void dump_stack_proc(stack *pstk)
{
  ref *p;

  (void)printf("stack contents (%d): ", stk_height(*pstk));

  /* For now, we don't bother to print the segments. */

  for (p = &pstk->data[0]; p<=pstk->ptr; p++)
    {
      printref(*p);
      (void)putc(p == pstk->ptr ? '\n' : ' ', stdout);
    }

  fflush_stdout();
}



void init_stk(stack *pstk)
{
#ifdef MALLOC_STACK_BUFFER
  pstk->data = (ref *)my_malloc((long)(sizeof(ref) * STACK_BUFFER_SIZE));
#endif
  pstk->ptr = &pstk->data[0];
  pstk->data[0] = INT_TO_REF(1234);
  /* This becomes e_nil when segment_type is loaded. */
  pstk->segment = INT_TO_REF(0);
  pstk->pushed_count = 0;
}
