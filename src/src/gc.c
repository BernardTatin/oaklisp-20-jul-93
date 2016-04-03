/* Copyright (C) 1987,8,9 Barak Pearlmutter and Kevin Lang. */

/* This file contains the garbage collector. */

#include <stdio.h>
#if defined(__SunOS)
#include <stdlib.h>
#endif
#include "emulator.h"
#include "stacks.h"
#include "gc.h"

#ifdef USE_VADVISE
#ifdef unix
#ifdef BSD_OR_MACH
#include <sys/vadvise.h>
#endif
#endif
#endif

extern stack val_stk, cxt_stk;

bool gc_shutup=FALSE, full_gc;



#define GC_TOUCH(x)			\
{					\
  if ((x)&PTR_MASK)			\
    {					\
      ref *MACROp = ANY_TO_PTR((x));	\
					\
      if (OLD_PTR(MACROp))		\
	(x) = gc_touch0((x));		\
    }					\
}

#define GC_TOUCH_PTR(r,o)					\
{								\
  (r) = REF_TO_PTR(gc_touch0(PTR_TO_REF((r)-(o)))) + (o);	\
}



#define LOC_TOUCH(x)				\
{						\
  if (TAG_IS((x),LOC_TAG))			\
    {						\
      ref *MACROp = LOC_TO_PTR((x));		\
						\
      if (OLD_PTR(MACROp))			\
	(x)=loc_touch0((x),0);			\
    }						\
}

#define LOC_TOUCH_PTR(x)				\
{							\
  (x) = LOC_TO_PTR(loc_touch0(PTR_TO_LOC(x),1));	\
}



space new, spatic, old;
ref *free_point;

unsigned long transport_count;
unsigned long loc_transport_count;

ref pre_gc_nil;


#define GC_EXAMINE_BUFFER_SIZE 20

ref gc_examine_buffer[GC_EXAMINE_BUFFER_SIZE];
ref *gc_examine_ptr = &gc_examine_buffer[0];





void gc_printref(ref refin)
{
  long i;
  char suffix = '?';

  if (refin&PTR_MASK)
    {
      ref *p = ANY_TO_PTR(refin);
      
      if (SPATIC_PTR(p))
	{
	  i = p - spatic.start;
	  suffix = 's';
	}
      else if (NEW_PTR(p))
	{
	  i = p - new.start + spatic.size;
	  suffix = 'n';
	}
      else if (OLD_PTR(p))
	{
	  i = p - old.start + spatic.size;
	  suffix = 'o';
	}
      else
	i = (long)p >> 2;

      fprintf(stderr, "%ld~%ld%c", i, refin&TAG_MASK, suffix);
    }
  else
    fprintf(stderr, "%ld~%ld", refin>>2, refin&TAG_MASK);
}




#define GC_NULL(r) ((r)==pre_gc_nil || (r)==e_nil)





/* This variant of get_length has to follow forwarding pointers so
   that it will work in the middle of a gc, when an object's type might
   already have been transported. */

unsigned long gc_get_length(ref x)
{
  if TAG_IS(x,PTR_TAG)
    {
      ref typ = REF_SLOT(x,0);
      ref vlen_p = REF_SLOT(typ, TYPE_VAR_LEN_P_OFF);
      ref len;
      
      /* Is vlen_p forwarded? */
      if (TAG_IS(vlen_p,LOC_TAG)) 
	vlen_p = *LOC_TO_PTR(vlen_p);

      /* Is this object variable length? */
      if (GC_NULL(vlen_p))
	{
	  /* Not variable length. */
	  len = REF_SLOT(typ, TYPE_LEN_OFF);

	  /* Is length forwarded? */
	  if (TAG_IS(len,LOC_TAG)) 
	    len = *LOC_TO_PTR(len);
	  
	  return REF_TO_INT(len);
	}
      else
	return REF_TO_INT(REF_SLOT(x,1));
    }
  else
    {
      fprintf(stderr, "; WARNING!!!  gc_get_length(");
      gc_printref(x);
      fprintf(stderr, ") called; only a tag of %d is allowed.\n", PTR_TAG);
      return 0;
    }
}








ref gc_touch0(ref r)
{
  ref *p = ANY_TO_PTR(r);

  if (OLD_PTR(p))

    if (r&1)
      {
	ref type_slot = *p;
	if (TAG_IS(type_slot,LOC_TAG))
	  /* Already been transported. */
	  /* Tag magic transforms this:
	     return(PTR_TO_REF(LOC_TO_PTR(type_slot)));
	     to this: */
	  return type_slot|1L;
	else
	  {
	    /* Transport it */
	    long i;
	    long len = gc_get_length(r);
	    ref *new_place = free_point;
	    ref *p0 = p;
	    ref *q0 = new_place;

	    transport_count += 1;

	    /*
	      fprintf(stderr, "About to transport ");
	      gc_printref(r);
	      fprintf(stderr, " len = %ld.\n", len);
	      */

	    free_point += len;
#ifndef FAST
	    if (free_point >= new.end)
	      {
		fprintf(stderr, "\n; New space exhausted while transporting ");
		gc_printref(r);
		fprintf(stderr, ".\n; This indicates a bug in the garbage collector.\n");
		exit(1);
	      }
#endif

	    for (i=0; i<len; i++, p0++, q0++)
	      {
		*q0 = *p0;
		*p0 = PTR_TO_LOC(q0);
	      }

	    return(PTR_TO_REF(new_place));
	  }
      }
    else
      {
	/* Follow the chain of locatives to oldspace until we find a
	   real object or a circularity. */
	ref r0 = r, r1 = *p, *pp;
	/* int chain_len = 1; */

	while (TAG_IS(r1,LOC_TAG) && (pp = LOC_TO_PTR(r1), OLD_PTR(pp)))
	  {
	    if (r0==r1)
	      {
		/* fprintf(stderr, "Circular locative chain.\n"); */
		goto forwarded_loc;
	      }
	    r0 = *LOC_TO_PTR(r0);
	    r1 = *pp;
	    /* chain_len += 1; */
		
	    if (r0==r1)
	      {
		/* fprintf(stderr, "Circular locative chain.\n"); */
		goto forwarded_loc;
	      }
	    if (!TAG_IS(r1,LOC_TAG) || (pp = LOC_TO_PTR(r1), !OLD_PTR(pp)))
	      break;

	    r1 = *pp;
	    /* chain_len += 1; */
	  }

	/* We're on an object, so touch it. */
	/* 
	  fprintf(stderr, "Locative chain followed to ");
	  gc_printref(r1);
	  fprintf(stderr, " requiring %d dereferences.\n", chain_len);
	  */
	GC_TOUCH(r1);
	/* (void)gc_touch(r1); */

	/* Now see if we're looking at a forwarding pointer. */
      forwarded_loc:
	return(r);
      }
  else
    return(r);
}





ref loc_touch0(ref r, const bool warn_if_unmoved)
{
  ref *p = LOC_TO_PTR(r);

  if (OLD_PTR(p))
    {
      /* A locative into old space.  See if it's been transported yet. */
      ref r1 = *p;
      if (TAG_IS(r1,LOC_TAG) && NEW_PTR(LOC_TO_PTR(r1)))
	/* Already been transported. */
	return(r1);
      else
	{
	  /* Better transport this lonely cell. */

	  ref *new_place = free_point++; /* make a new cell. */
	  ref new_r = PTR_TO_LOC(new_place);
	      
#ifndef FAST
	  if (free_point >= new.end)
	    {
	      fprintf(stderr, "\n; New space exhausted while transporting the cell ");
	      gc_printref(r);
	      fprintf(stderr, ".\n; This indicates a bug in the garbage collector.\n");
	      exit(1);
	    }
#endif

	  *p = new_r;		/* Record the transportation. */

	  /* Put the right value in the new cell. */

	  *new_place =
	    TAG_IS(r1,PTR_TAG) && (p = REF_TO_PTR(r1),OLD_PTR(p))
	      ? *p|1 : r1;
	  /* ? PTR_TO_REF(REF_TO_PTR(*p)) : r1; */

	  loc_transport_count += 1;

	  if (warn_if_unmoved)
	    {
	      fprintf(stderr, "\nWarning: the locative ");
	      gc_printref(r);
	      fprintf(stderr, " has just had its raw cell moved.\n");
	    }

	  return(new_r);
	}
    }
  else
    return(r);			/* Not a locative into old space. */
}




void scavenge(void)
{
  ref *scavenge_p;

  for (scavenge_p = new.start; scavenge_p < free_point; scavenge_p += 1)
    GC_TOUCH(*scavenge_p);
}




void loc_scavenge(void)
{
  ref *scavenge_p;

  for (scavenge_p = new.start; scavenge_p < free_point; scavenge_p += 1)
    LOC_TOUCH(*scavenge_p);
}






#ifndef FAST

#define GGC_CHECK(r) GC_CHECK(r,"r")

/* True if r seems like a messed up reference. */
bool gc_check(ref r)
{
  return (r&PTR_MASK) && !NEW_PTR(ANY_TO_PTR(r))
    && (full_gc || !SPATIC_PTR(ANY_TO_PTR(r)));
}



void GC_CHECK(ref x, char *st)
{
  if (gc_check((x)))
    {
      fprintf(stderr, "%s = ", (st));
      gc_printref((x));
      if (OLD_PTR(ANY_TO_PTR(x)))
	{
	  fprintf(stderr, ",  cell contains ");
	  gc_printref(*ANY_TO_PTR(x));
	}
      fprintf(stderr, "\n");
    }
}




void GC_CHECK1(ref x, char *st, const long i)
{
  if (gc_check((x)))
    {
      fprintf(stderr, (st), (i));
      gc_printref((x));
      if (OLD_PTR(ANY_TO_PTR(x)))
	{
	  fprintf(stderr, ",  cell contains ");
	  gc_printref(*ANY_TO_PTR(x));
	}
      fprintf(stderr, "\n");
    }
}

#endif






unsigned short *pc_touch(unsigned short *o_pc)
{
  ref *pcell = (ref *)((unsigned long)o_pc & ~TAG_MASKL);

  LOC_TOUCH_PTR(pcell);
  return
    (unsigned short *)((unsigned long)pcell | (unsigned long)o_pc&TAG_MASK);
}



void set_external_full_gc(const bool full)
{
  full_gc = full;
}






void gc(bool pre_dump, bool full_gc, char *reason, long amount)
     // bool pre_dump;		/* About to dump world?  (discards stacks) */
     // bool full_gc;		/* Reclaim garbage from spatic space too? */
     // char *reason;		/* The reason for this GC, in English. */
     // long amount;		/* The amount of space that is needed. */
{
  long old_taken;
  ref *p;

#ifdef mac
  GrafPtr savePort;
  GetPort(&savePort);
#endif

  /* The full_gc flag is also a global to avoid ugly parameter passing. */
  set_external_full_gc(full_gc);

  if (!gc_shutup)
    fprintf(stderr, "\n; %sGC due to %s.\n", full_gc ? "Full " : "", reason);

 gc_top:

  if (trace_gc && !pre_dump)
    {
      fprintf(stderr, "value ");
      dump_stack_proc(&val_stk);
      fprintf(stderr, "context ");
      dump_stack_proc(&cxt_stk);
    }

  if (!gc_shutup) fprintf(stderr, "; Flipping...");

  old_taken = free_point - new.start;
  old = new;

  if (full_gc)
    new.size += spatic.size;
  else
    new.size = e_next_newspace_size;

  alloc_space(&new);
  free_point = new.start;

  transport_count = 0;

  if (!gc_shutup) fprintf(stderr, " rooting...");

  {
    /* Hit the registers: */

    pre_gc_nil = e_nil;
    GC_TOUCH(e_nil);
    GC_TOUCH(e_boot_code);

    if (!pre_dump)
      {
	GC_TOUCH(e_t);
	GC_TOUCH(e_fixnum_type);
	GC_TOUCH(e_loc_type);
	GC_TOUCH(e_cons_type);
	GC_TOUCH_PTR(e_subtype_table,2);
	/* e_bp is a locative, but a pointer to the object should exist, so we
	   need only touch it in the locative pass. */
	GC_TOUCH_PTR(e_env,0);
	/* e_nargs is a fixnum.  Nor is it global... */
	GC_TOUCH(e_env_type);
	GC_TOUCH_PTR(e_argless_tag_trap_table,2);
	GC_TOUCH_PTR(e_arged_tag_trap_table,2);
	GC_TOUCH(e_object_type);
	GC_TOUCH(e_segment_type);
	GC_TOUCH(e_code_segment);
	GC_TOUCH(e_current_method);
	GC_TOUCH(e_uninitialized);
	GC_TOUCH(e_method_type);

	for (p = &gc_examine_buffer[0]; p < gc_examine_ptr; p++)
	  GC_TOUCH(*p);

	gc_prepare(&val_stk);
	gc_prepare(&cxt_stk);

	/* Scan the stacks. */
	for (p = &val_stk.data[0]; p <= val_stk.ptr; p++)
	  GC_TOUCH(*p);
      
	for (p = &cxt_stk.data[0]; p <= cxt_stk.ptr; p++)
	  GC_TOUCH(*p);

	/* Scan the stack segments. */
	GC_TOUCH(val_stk.segment);
	GC_TOUCH(cxt_stk.segment);

	/* Scan static space. */
	if (!full_gc)
	  for (p = spatic.start; p<spatic.end; p++)
	    GC_TOUCH(*p);
      }

  
    /* Scavenge. */
    if (!gc_shutup)
      fprintf(stderr, " scavenging...");
    scavenge();

    if (!gc_shutup)
      fprintf(stderr, " %ld object%s transported.\n",
	      transport_count, transport_count != 1 ? "s" : "");



    /* Clean up the locatives. */
    if (!gc_shutup)
      fprintf(stderr, "; Scanning locatives...");
    loc_transport_count = 0;
  
    if (!pre_dump)
      {
	LOC_TOUCH_PTR(e_bp);
	g_e_pc = pc_touch(g_e_pc);

	LOC_TOUCH(e_uninitialized);

	for (p = &gc_examine_buffer[0]; p < gc_examine_ptr; p++)
	  LOC_TOUCH(*p);

	for (p = &val_stk.data[0]; p <= val_stk.ptr; p++)
	  LOC_TOUCH(*p);
      
	for (p = &cxt_stk.data[0]; p <= cxt_stk.ptr; p++)
	  LOC_TOUCH(*p);

	/* Scan spatic space. */
	if (!full_gc)
	  for (p = spatic.start; p<spatic.end; p++)
	    LOC_TOUCH(*p);
      }
  
    if (!gc_shutup)
      fprintf(stderr, " scavenging...");
    loc_scavenge();

    if (!gc_shutup)
      fprintf(stderr, " %ld naked cell%s transported.\n",
	      loc_transport_count, loc_transport_count != 1 ? "s" : "");


    /* Discard weak pointers whose targets have not been transported. */
    if (!gc_shutup)
      fprintf(stderr, "; Scanning weak pointer table...");
    {
      long count = post_gc_wp();

      if (!gc_shutup)
	fprintf(stderr, " %ld entr%s discarded.\n",
		count, count != 1 ? "ies" : "y");
    }
  }
  

#ifndef FAST
  {
    /* Check GC consistency. */

    if (!gc_shutup)
      fprintf(stderr, "; Checking consistency...\n");

    GGC_CHECK(e_nil);
    GGC_CHECK(e_boot_code);

    if (!pre_dump)
      {
	GGC_CHECK(e_t);
	GGC_CHECK(e_fixnum_type);
	GGC_CHECK(e_loc_type);
	GGC_CHECK(e_cons_type);
	GC_CHECK(PTR_TO_REF(e_subtype_table-2),"e_subtype_table");
	GC_CHECK(PTR_TO_LOC(e_bp),"PTR_TO_LOC(e_bp)");
	GC_CHECK(PTR_TO_REF(e_env),"e_env");
	/* e_nargs is a fixnum.  Nor is it global... */
	GGC_CHECK(e_env_type);
	GC_CHECK(PTR_TO_REF(e_argless_tag_trap_table-2),"e_argless_tag_trap_table");
	GC_CHECK(PTR_TO_REF(e_arged_tag_trap_table-2),"e_arged_tag_trap_table");
	GGC_CHECK(e_object_type);
	GGC_CHECK(e_segment_type);
	GGC_CHECK(e_code_segment);
	GGC_CHECK(e_current_method);
	GGC_CHECK(e_uninitialized);
	GGC_CHECK(e_method_type);

	/* Scan the stacks. */
	for (p = &val_stk.data[0]; p <= val_stk.ptr; p++)
	  GC_CHECK1(*p, "val_stk.data[%d] = ", (long)(p - &val_stk.data[0]));

	for (p = &cxt_stk.data[0]; p <= cxt_stk.ptr; p++)
	  GC_CHECK1(*p, "cxt_stk.data[%d] = ", (long)(p - &cxt_stk.data[0]));

	GGC_CHECK(val_stk.segment);
	GGC_CHECK(cxt_stk.segment);

	/* Make sure the program counter is okay. */
	GC_CHECK((ref)((ref)g_e_pc|LOC_TAG), "g_e_pc");
      }

    /* Scan the heap. */

    if (!full_gc)
      for (p = spatic.start; p<spatic.end; p++)
	GC_CHECK1(*p, "static_space[%ld] = ", (long)(p-spatic.start));

    for (p = new.start; p<free_point; p++)
      GC_CHECK1(*p, "new_space[%ld] = ", (long)(p-new.start));
  }
#endif				/* ndef FAST */

  /* Hopefully there are no more references into old space. */
  free_space(&old);
  if (full_gc) free_space(&spatic);

#ifdef USE_VADVISE
#ifdef unix
#ifdef BSD_OR_MACH
#ifdef VA_FLUSH
  /* Tell the virtual memory system that recent statistics are useless. */
  vadvise(VA_FLUSH);
#endif
#endif
#endif
#endif

  if (trace_gc && !pre_dump)
    {
      fprintf(stderr, "val_stk ");
      dump_stack_proc(&val_stk);
      fprintf(stderr, "cxt_stk ");
      dump_stack_proc(&cxt_stk);
    }


  {
    long new_taken = free_point - new.start;
    long old_total = old_taken + (full_gc ? spatic.size : 0);
    long reclaimed = old_total - new_taken;

    if (!gc_shutup)
      {
	fprintf(stderr, "; GC complete.  %ld ", old_total);
	if (full_gc) fprintf(stderr, "(%ld+%ld) ", spatic.size, old_taken);
	fprintf(stderr, "compacted to %ld; %ld (%ld%%) garbage.\n",
		new_taken, reclaimed, (100*reclaimed)/old_total);
      }

    /* Make the next new space bigger if the current was too small. */
    if (!full_gc && !pre_dump && (RECLAIM_FACTOR*new_taken + amount > new.size))
      {
	e_next_newspace_size = RECLAIM_FACTOR*new_taken + amount;
	if (!gc_shutup)
	  fprintf(stderr, "; Expanding next new space from %ld to %ld (%d%%).\n",
		  new.size, e_next_newspace_size,
		  (100*(e_next_newspace_size - new.size))/new.size);
	if ((new.end - free_point) < amount)
	  {
	    reason = "immediate new space expansion necessity";
	    goto gc_top;
	  }
      }

    if (full_gc && !pre_dump)
      {
	/* Ditch old spatic, move new to spatic, and reallocate new. */
	free_space(&spatic);
	spatic = new;
	realloc_space(&spatic, free_point);

	if (!gc_shutup && e_next_newspace_size != original_newspace_size)
	  fprintf(stderr, "; Setting new space size to %ld.\n", original_newspace_size);
	new.size = e_next_newspace_size = original_newspace_size;
	if (e_next_newspace_size <= amount)
	  {
	    e_next_newspace_size = RECLAIM_FACTOR*amount;
	    if (!gc_shutup)
	      fprintf(stderr, "; Expanding next new space from %ld to %ld (%d%%).\n",
		      new.size, e_next_newspace_size,
		      (100*(e_next_newspace_size - new.size))/new.size);
	    new.size = e_next_newspace_size;
	  }
	alloc_space(&new);
	free_point = new.start;
      }
    if (!gc_shutup)
      fflush_stdout();
  }
#ifdef mac
  SetPort(savePort);
#endif
}
