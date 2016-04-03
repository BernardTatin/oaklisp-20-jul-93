/*  Copyright (C) 1987 Barak Pearlmutter and Kevin Lang    */

#include <stdio.h>
#if defined(__SunOS)
#include <stdlib.h>
#endif
#include "emulator.h"


char *my_malloc(const long i)
{
#ifdef Mac_LSC
  char *p;
  ResrvMem(i);
  p = mlalloc(i);
#else
#ifdef unix
  char *p = (char *)malloc((unsigned)i);
#else
  char *p = (char *)malloc(i);
#endif
#endif

  if (p == NULL)
    {
      (void)fprintf(stderr, "\nUnable to allocate %ld bytes.\n", i);
      exit(1);
    }

  return p;
}


void realloc_space(space *pspace, ref *pfree)
{
  long new_size = pfree - pspace->start;
  pspace->size = new_size;
  pspace->end = pspace->start + new_size;
  /* realloc commented out because it might move the block, even though
     it's sure to be shrunk. */
  /* realloc((char *)pspace->start, sizeof(ref)*new_size); */
}



void free_space(space *pspace)
{
#ifdef UNALIGNED
  free((char *)pspace->start - pspace->offset);
  if (pspace->offset)
    pspace->size += 1;
#else
  free((char *)pspace->start);
#endif

  pspace->start = pspace->end = NULL;
}




void alloc_space(space *pspace)
{
  char *p = my_malloc(sizeof(ref) * pspace->size);

#ifdef UNALIGNED
  pspace->offset = (long)p & 3L;
  p = (char *) ((long)p + pspace->offset);
  if (pspace->offset)
    pspace->size -= 1;
#endif

  /*NOSTRICT*/
  pspace->start = (ref *) p;
  pspace->end = pspace->start + pspace->size;
}



