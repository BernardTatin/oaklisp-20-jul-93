/* Copyright (C) 1987, Barak Pearlmutter & Kevin Lang. */


/* STACK_BUFFER_SIZE is how big the active part of the stack is, while
   STACK_BUFFER_HYSTERESIS is how much of the buffer is not flushed to
   the heap when an overflow occurs.  MAX_SEGMENT_SIZE is the maximum
   amount of data put into a single stack segment when flushing the buffer.

   The tradeoffs are as follows:

   Making STACK_BUFFER_SIZE bigger increases performance by making overflows
   less frequent.  Although it makes continuation creation more expensive
   each time, the amortized cost remains the same or decreases.

   Making STACK_BUFFER_HYSTERESIS bigger should decrease underflow
   frequency, but make call/cc more expensive.  It should also makes
   pushing like mad forever more expensive, since the data in the
   hysteretic section has to be copied twice, once from top to bottom and
   once out into the heap.

   Making MAX_SEGMENT_SIZE bigger makes continuations somewhat more costly.
   Making it smaller makes the header overhead of each segment
   a greater fraction of the cost.

   For portability, STACK_BUFFER_SIZE had better be an int! */

#ifndef STACKS_H
#define STACKS_H

#ifdef BIGINT
#define STACK_BUFFER_SIZE 32768L
#else
#define STACK_BUFFER_SIZE 8192L
#endif

#define STACK_BUFFER_HYSTERESIS 32

#define MAX_SEGMENT_SIZE 64


/*
  #define val_stk_ptr val_stk.ptr
  #define UNOPTV(x)
  #define cxt_stk_ptr cxt_stk.ptr
  #define UNOPTC(x)
  */

#define UNOPTV(x) x
#define UNOPTC(x) x




#define CONTEXT_FRAME_SIZE 3	/* This is not a tunable parameter. */




#define SAVE_PTRS()			\
{					\
  UNOPTV(val_stk.ptr = val_stk_ptr);	\
  UNOPTC(cxt_stk.ptr = cxt_stk_ptr);	\
  g_e_pc = e_pc;			\
}

#define RETR_PTRS()			\
{					\
  e_pc = g_e_pc;			\
  UNOPTC(cxt_stk_ptr = cxt_stk.ptr);	\
  UNOPTV(val_stk_ptr = val_stk.ptr);	\
}

typedef struct
{
  ref type_field;
  ref length_field;
  ref previous_segment;
  ref data[1];			/* I want 0 here, but C gets mad. */
} segment;

#define SEGMENT_HEADER_LENGTH	(sizeof(segment)/sizeof(ref)-1L)


typedef struct
{
  ref segment;			/* The segment to pop from. */
#ifdef MALLOC_STACK_BUFFER
  ref *data;			/* The buffer itself. */
#else
  ref data[STACK_BUFFER_SIZE];	/* The buffer itself. */
#endif
  ref *ptr;			/* This is updated when calling out.  It points
				   to the top value. */
  int pushed_count;		/* Number of references heaped. */
} stack;




#define FLUSHVAL(amount_to_leave)		\
{						\
  SAVE_PTRS();					\
  flush_buffer(&val_stk, (amount_to_leave));	\
  RETR_PTRS();					\
}

#define FLUSHCXT(amount_to_leave)		\
{						\
  SAVE_PTRS();					\
  flush_buffer(&cxt_stk, (amount_to_leave));	\
  RETR_PTRS();					\
}

#define UNFLUSHVAL(n)			\
{					\
  UNOPTV(val_stk.ptr = val_stk_ptr);	\
  unflush_buffer(&val_stk, (n));	\
  UNOPTV(val_stk_ptr = val_stk.ptr);	\
}

#define UNFLUSHCXT(n)			\
{					\
  UNOPTC(cxt_stk.ptr = cxt_stk_ptr);	\
  unflush_buffer(&cxt_stk, (n));	\
  UNOPTC(cxt_stk_ptr = cxt_stk.ptr);	\
}



/* Use this for looking at the top of stack at any time.  Use as lvalue too. */
#define PEEKVAL()	(*val_stk_ptr)

/* Use this for looking before the top of stack, only when you're sure the
   buffer has enough stuff in it. */
#define PEEKVAL_UP(x)	(*(val_stk_ptr-(x)))

#ifdef NOCALLCC

#define PUSHVAL(r) PUSHVAL_NOCHECK(r)

#else

#define PUSHVAL(r)						\
{								\
  if (val_stk_ptr == &val_stk.data[STACK_BUFFER_SIZE-1])	\
    {								\
      GC_MEMORY(r);						\
      SAVE_PTRS();						\
      flush_buffer(&val_stk, 0);				\
      RETR_PTRS();						\
      GC_RECALL(*++val_stk_ptr);				\
    }								\
  else								\
    *++val_stk_ptr = (r);					\
}

#endif


/* Use this if you're SURE that an overflow is impossible. */
#define PUSHVAL_NOCHECK(r)	{ *++val_stk_ptr = (r); }


#define POPVAL(v)		\
{				\
  CHECKVAL_POP(1);		\
  (v) = *val_stk_ptr--;		\
}


#define PUSHVAL_IMM(r)	\
{			\
  CHECKVAL_PUSH(1);	\
  PUSHVAL_NOCHECK((r));	\
}

/* This pops and return the top of the value stack, without checking for
   underflow.  Use it if you're SURE that underflow is not possible. */
#define POPVAL_NOCHECK()	(*val_stk_ptr--)

#ifdef NOCALLCC
#define CHECKVAL_POP(n)
#define CHECKCXT_POP(n)
#define CHECKVAL_PUSH(n)
#define CHECKCXT_PUSH(n)
#else

/* This ensures that n guys can be popped without underflow. */
#define CHECKVAL_POP(n)				\
{						\
  if (val_stk_ptr <= &val_stk.data[(n)-1])	\
    UNFLUSHVAL((n));				\
}

/* This ensures that n guys can be popped without underflow. */
#define CHECKCXT_POP(n)				\
{						\
  if (cxt_stk_ptr <= &cxt_stk.data[(n)-1])	\
    UNFLUSHCXT((n));				\
}

/* This ensures that n guys can be pushed without overflow. */
#define CHECKVAL_PUSH(n)					\
{								\
  if (val_stk_ptr >= &val_stk.data[STACK_BUFFER_SIZE-(n)])	\
    FLUSHVAL(STACK_BUFFER_HYSTERESIS);				\
}

/* This ensures that n guys can be pushed without overflow. */
#define CHECKCXT_PUSH(n)					\
{								\
  if (cxt_stk_ptr >= &cxt_stk.data[STACK_BUFFER_SIZE-(n)])	\
    FLUSHCXT(STACK_BUFFER_HYSTERESIS);				\
}

#endif

/* This routine avoids having a bogus reference in the segments. */
#define BASH_SEGMENT_TYPE(x) { cxt_stk.segment = val_stk.segment = e_nil; }
  


extern void init_stk(), flush_buffer(), unflush_buffer();



/* This, which pops some guys off the value stack, is inefficient because
   it copies guys into the buffer and then pops them off.  A better
   thing should be written.  */
#define POPVALS(x)	\
{			\
  CHECKVAL_POP((x));	\
  val_stk_ptr -= (x);	\
}

#define POPCXTS(x)	\
{			\
  CHECKCXT_POP((x));	\
  cxt_stk_ptr -= (x);	\
}


#define PUSH_CONTEXT(offset)					\
{								\
  CHECKCXT_PUSH(CONTEXT_FRAME_SIZE);				\
  *++cxt_stk_ptr =						\
	   INT_TO_REF((long)e_pc - (long)e_code_segment		\
		      + 2*(offset));				\
  *++cxt_stk_ptr = e_current_method;				\
  *++cxt_stk_ptr = PTR_TO_LOC(e_bp);				\
}


#define POP_CONTEXT()					\
{							\
  CHECKCXT_POP(CONTEXT_FRAME_SIZE);			\
  e_bp = LOC_TO_PTR(*cxt_stk_ptr--);			\
  e_current_method = *cxt_stk_ptr--;			\
  e_env = REF_TO_PTR(e_current_method);			\
  e_code_segment = SLOT(e_env,METHOD_CODE_OFF);		\
  e_env = REF_TO_PTR(SLOT(e_env,METHOD_ENV_OFF));	\
  e_pc = (unsigned short *)				\
    ((long)e_code_segment				\
     + REF_TO_INT(*cxt_stk_ptr--));			\
}


/* No preparation needed! */
#define gc_prepare(pstk)


#define bash_val_height(h)		\
{					\
  int to_pop = val_height()-(h);	\
					\
  POPVALS(to_pop);			\
}


#define bash_cxt_height(h)		\
{					\
  int to_pop = cxt_height()-(h);	\
					\
  POPCXTS(to_pop);			\
}



#define MAKE_BACK_VAL_PTR(v,dist)	\
{					\
  CHECKVAL_POP((dist));			\
  (v) = val_stk_ptr - (dist);		\
}


#define stk_height(stk) ((stk).ptr - (&(stk).data[0]-1) + (stk).pushed_count)
#define val_height() (val_stk_ptr - (&val_stk.data[0]-1) + val_stk.pushed_count)
#define cxt_height() (cxt_stk_ptr - (&cxt_stk.data[0]-1) + cxt_stk.pushed_count)

extern void dump_stack_proc();


#define dump_val_stk()			\
{					\
  UNOPTV(val_stk.ptr = val_stk_ptr);	\
  dump_stack_proc(&val_stk);		\
}


#define dump_cxt_stk()			\
{					\
  UNOPTC(cxt_stk.ptr = cxt_stk_ptr);	\
  dump_stack_proc(&cxt_stk);		\
}

#endif
