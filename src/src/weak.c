/*  Copyright (C) 1987, 1988 Barak Pearlmutter and Kevin Lang    */

/* This file has weak pointers in it. */

#include <stdio.h>
#include "emulator.h"
#include "gc.h"


/*
 * Weak pointers are done with a simple table that goes from weak
 * pointers to objects, and a hash table that goes from objects to
 * their weak pointers.

 * In the future, this will be modified to keep separate hash tables
 * for the different areas, so that objects in spatic space need not
 * be rehashed.

 * Plus another one for immediates and fixnums, which don't live in
 * any space at all.

 */

#define WP_TABLE_SIZE 10000L

#ifdef MALLOC_WP_TABLE
ref *wp_table;
#else
ref wp_table[WP_TABLE_SIZE+1];
#endif

long wp_index = 0;


/* A hash table from references to their weak pointers.  This hash
 * table is not saved in dumped worlds, and is rebuilt from scratch
 * after each GC and upon booting a new world.

 * Structure of this hash table:

 * Keys are references themselves, smashed about and xored if deemed
 * necessary.

 * Sequential rehash, single probe.
 */

#define WP_HASHTABLE_SIZE 10007L	/* Good idea for this to be prime. */

typedef struct
{
  ref obj, wp;
} wp_hashtable_entry;

#ifdef MALLOC_WP_TABLE
wp_hashtable_entry *wp_hashtable;
#else
wp_hashtable_entry wp_hashtable[WP_HASHTABLE_SIZE];
#endif


/* The following magic number is floor( 2^32 * (sqrt(5)-1)/2 ). */
#define wp_key(r) ((unsigned long) 0x9E3779BB*(r)) /* >>10, == 2654435771L */

void init_wp()
{
#ifdef MALLOC_WP_TABLE
  wp_table = (ref *)my_malloc((long)(sizeof(ref) * WP_TABLE_SIZE));
  wp_hashtable = (wp_hashtable_entry *)
    my_malloc((long)(sizeof(wp_hashtable_entry) * WP_HASHTABLE_SIZE));
#endif
}


/* Register r as having weak pointer wp. */
void enter_wp(r, wp)
     ref r, wp;
{
  unsigned long i = wp_key(r) % WP_HASHTABLE_SIZE;

  while (TRUE)
    if (wp_hashtable[i].obj == e_nil)
      {
	wp_hashtable[i].obj = r;
	wp_hashtable[i].wp = wp;
	return;
      }
    else if (++i == WP_HASHTABLE_SIZE)
      i = 0;
}




/* Rebuild the weak pointer hash table from the information in the table
   that takes weak pointers to objects. */
void rebuild_wp_hashtable()
{
  unsigned long i;
  
  for (i=0; i<WP_HASHTABLE_SIZE; i++)
    wp_hashtable[i].obj = e_nil;

  for (i = 0; i<wp_index; i++)
    if (wp_table[1+i] != e_nil)
      enter_wp(wp_table[1+i], INT_TO_REF(i));
}





/* Return weak pointer associated with obj, making a new one if necessary. */

ref ref_to_wp(r)
     ref r;
{
  unsigned long i = wp_key(r) % WP_HASHTABLE_SIZE;

  if (r == e_nil) return INT_TO_REF(-1);

  while (TRUE)
    {
      if (wp_hashtable[i].obj == r)
	return wp_hashtable[i].wp;
      else if (wp_hashtable[i].obj == e_nil)
	{
	  /* Make a new weak pointer, installing it in both tables: */
	  wp_hashtable[i].obj = wp_table[1+wp_index] = r;
	  return wp_hashtable[i].wp = INT_TO_REF(wp_index++);
	}
      else if (++i == WP_HASHTABLE_SIZE)
	i = 0;
    }
}



void wp_hashtable_distribution()
{
  unsigned long i;

  for (i=0; i<WP_HASHTABLE_SIZE; i++)
    {
      ref r = wp_hashtable[i].obj;
      
      if (r == e_nil)
	(void)putchar('.');
      else 
	{
	  unsigned long j = wp_key(r) % WP_HASHTABLE_SIZE;
	  long dist = i-j;

	  if (dist < 0) dist += WP_HASHTABLE_SIZE;

	  if (dist < 1+'9'-'0')
	    (void)putchar((char)('0'+dist));
	  else if (dist < 1+'Z'-'A' + 1+'9'-'0')
	    (void)putchar((char)('A' + dist - (1+'9'-'0')));
	  else
	    (void)putchar('*');
	}

      fflush_stdout();
    }
}






unsigned long post_gc_wp()
{
  /* Scan the weak pointer table.  When a reference to old space is found,
     check if the location has a forwarding pointer.  If not, discard the
     reference; if so, update it. */
  long i;
  unsigned long discard_count = 0;

  for (i=0; i<wp_index; i++)
    {
      ref r = wp_table[1+i], *p;

      if ( (r&PTR_MASK) && (p = ANY_TO_PTR(r), OLD_PTR(p)) )
	{
	  ref r1 = *p;

	  if (TAG_IS(r1,LOC_TAG) && NEW_PTR(LOC_TO_PTR(r1)))
	    {
	      wp_table[1+i] = TAG_IS(r,LOC_TAG) ? r1 : r1|PTR_TAG;
	    }
	  else
	    {
	      wp_table[1+i] = e_nil;
	      discard_count += 1;
	    }
	}
    }

  rebuild_wp_hashtable();

  return discard_count;
}


