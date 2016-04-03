/*  Copyright (C) 1987 Barak Pearlmutter and Kevin Lang    */


#include <stdio.h>
#include <ctype.h>
#if defined(__SunOS)
#include <stdlib.h>
#endif
#include "emulator.h"


/* These are for making the world zero-based and contiguous in dumps. */

ref contig(r, just_new)
     ref r;
     bool just_new;
{
  ref *p = ANY_TO_PTR(r);

  if (just_new)
    if (NEW_PTR(p))
      return (p - new.start)*4 | r&3;
    else
      printf("Non-new pointer %ld found.\n", r);
  else if (SPATIC_PTR(p))
    return (p - spatic.start)*4 | r&3;
  else if (NEW_PTR(p))
    return (p - new.start + spatic.size)*4 | r&3;
  else
    printf("Non-new or spatic pointer %ld found.\n", r);

  return r;
}

#define contigify(r) ((r)&0x2 ? contig((r),just_new) : (r))
#define CONTIGIFY(v) { if ((v)&2) (v) = contig((v),just_new); }





bool dump_decimal= FALSE;	/* dumped worlds in base 10 (not 16) */
bool dump_binary = FALSE;	/* dumped worlds in base 2  (not 16) */


/*
 * Format of Oaklisp world image:
 *
 * UNUSED: <size of value stack>
 * UNUSED: <size of context stack>
 * <reference to method for booting>
 * <number of words to load>
 *
 * <words to load>
 *
 * <size of weak pointer table>
 * <contents of weak pointer table>
 */


bool input_is_binary;

#ifdef Mac_LSC

long flread(ptr, size_of_ptr, count, stream)
	char		*ptr;
	unsigned int	size_of_ptr;
	long		count;
	FILE		*stream;
{
  long		result = 0;
  int		block, items;
	
  while (count > 0) {
    block = (count > 32000 ? 32000 : count);
    items = fread(ptr, size_of_ptr, block, stream);
    ptr = (char *) ((long) size_of_ptr * items + (long) ptr);
    count  -= items;
    result += items;
  }

  return result;
}

#else

/* Odd that the 3rd arg of fread is int; long would be more intuitive. */
#define flread(a,b,c,d) fread((a),(b),(int)(c),(d))

#endif

	
ref read_ref(d)  /* Read a reference from a file: */
     FILE *d;
{
  int c;
  ref a=0;

  /* It's easy to read a reference from a binary file. */
  if (input_is_binary) {
    (void)flread((char *)&a, sizeof(ref), 1L, d);
    return a;
  }

#ifdef BIG_ENDIAN
  while ( isspace(c=getc(d)) || c=='^' )
#else
  while ( isspace(c=getc(d)) )
#endif
    if (c == EOF)
      {
	(void)printf("Apparently truncated cold load file!\n");
	exit(1);
      }
  {
#ifndef BIG_ENDIAN
    bool swapem = c == '^';

    if (swapem)
      if ((c = getc(d)) == EOF)
	{
	  (void)printf("Apparently truncated cold load file!\n");
	  exit(1);
	}
#endif

    while (isxdigit(c))
      {
	a = a<<4;
	if (c <= '9')
	  a |= c-'0';
	else if (c <= 'Z')
	  a |= c-'A'+10;
	else
	  a |= c-'a'+10;
	c = getc(d);
      }

#ifndef BIG_ENDIAN
    if (c=='^') (void)ungetc(c,d);

    if (swapem)
      a = a<<16 | a>>16;
#endif
    
    return a;
  }
}




FILE *prompt_file(the_prompt,mode)
     char *the_prompt, *mode;
{
  char filename[80];
  FILE *fp;

  while (TRUE)
    {
      (void)printf(the_prompt);
      if (scanf("%s", filename) == EOF)
	/* Fix the problem with EOF in batch files. */
	{
	  (void)fprintf(stderr, "EOF in input.\n");
	  exit(1);
	}
      fp = fopen(filename, mode);
      if (fp != NULL)
	return fp;
      else
	(void)fprintf(stderr,"Can't open %s.\n", filename);
    }
}



#define REFBUFSIZ 1024

void dump_binary_world(just_new)
     bool just_new;
{
  FILE *wfp = NULL;
  ref *memptr;
  ref theref;
  ref refbuf[REFBUFSIZ];

  int imod = 0;
  unsigned long worlsiz = free_point - new.start;
  unsigned long DUMMY = 0;

#ifndef AMIGA
#ifndef mips
#ifndef __SunOS
  extern fwrite();
#else
#include <stdio.h>
#endif
#endif
#endif

  if (dump_file != NULL && ((wfp=fopen(dump_file,WRITE_BINARY_MODE)) == NULL))
    fprintf(stderr, "Can't open \"%s\"; querying user.\n", dump_file);

  while (wfp == NULL)
    {
      wfp = prompt_file("Binary world file to write: ", WRITE_BINARY_MODE);
    }

  if (!just_new) worlsiz += spatic.size;

  (void)putc('\002', wfp); (void)putc('\002', wfp);
  (void)putc('\002', wfp); (void)putc('\002', wfp);

  /* Header information. */
  (void)fwrite((char *)&DUMMY, sizeof(ref), 1, wfp);
  (void)fwrite((char *)&DUMMY, sizeof(ref), 1, wfp);
  theref = contigify(e_boot_code);
  (void)fwrite((char *)&theref, sizeof(ref), 1, wfp);
  (void)fwrite((char *)&worlsiz, sizeof(ref), 1, wfp);

  /* Dump the heap. */
  /* Maybe dump spatic space. */
  if (!just_new)
    for (memptr = spatic.start; memptr < spatic.end; memptr++)
      {
	theref = *memptr;
	CONTIGIFY(theref);
	refbuf[imod++] = theref;
	if (imod == REFBUFSIZ)
	  {
	    (void)fwrite((char *)&refbuf[0], sizeof(ref), imod, wfp);
	    imod = 0;
	  }
      }
  /* Dump new space. */
  for (memptr = new.start; memptr < free_point; memptr++)
    {
      theref = *memptr;
      CONTIGIFY(theref);
      refbuf[imod++] = theref;
      if (imod == REFBUFSIZ)
	{
	  (void)fwrite((char *)&refbuf[0], sizeof(ref), imod, wfp);
	  imod = 0;
	}
    }
  if (imod != 0)
    (void)fwrite((char *)&refbuf[0], sizeof(ref), imod, wfp);


  /* Weak pointer table. */
  theref = (ref)wp_index;
  (void)fwrite((char *)&theref, sizeof(ref), 1, wfp);

  for (imod = 0; imod<wp_index; imod++)
    {
      theref = wp_table[1+imod];
      CONTIGIFY(theref);
      (void)fwrite((char *)&theref, sizeof(ref), 1, wfp);
    }

  (void)fclose(wfp);
}







void dump_ascii_world(just_new)
     bool just_new;
{
  ref *memptr, theref;
  long	i;
  int	eighter = 0;
  char *control_string = (dump_decimal ? "%ld " : "%lx ");
  FILE *wfp = NULL;


  if (dump_file != NULL && ((wfp = fopen(dump_file, WRITE_MODE)) == NULL))
    fprintf(stderr, "Can't open \"%s\"; querying user.\n", dump_file);

  while (wfp == NULL)
    {
      wfp = prompt_file("World file to write: ", WRITE_MODE);
    }

  (void)fprintf(wfp, control_string, 0 /*val_stk_size*/);
  (void)fprintf(wfp, control_string, 0 /*cxt_stk_size*/);
  (void)fprintf(wfp, control_string, contigify(e_boot_code));
  (void)fprintf(wfp, control_string,
		free_point - new.start + (just_new ? 0 : spatic.size));

  /* Maybe dump spatic space. */
  if (!just_new)
    for (memptr = spatic.start; memptr < spatic.end; memptr++)
      {
	if (eighter == 0) (void)fprintf(wfp, "\n");
	theref = *memptr;
	CONTIGIFY(theref);
	(void)fprintf(wfp,control_string, theref);
	eighter = (eighter + 1) % 8;
      }

  eighter = 0;
  for (memptr = new.start; memptr < free_point; memptr++)
    {
      if (eighter == 0) (void)fprintf(wfp, "\n");
      theref = *memptr;
      CONTIGIFY(theref);
      (void)fprintf(wfp,control_string, theref);
      eighter = (eighter + 1) % 8;
    }
  (void)fprintf(wfp, "\n");

  /* Write the weak pointer table. */
     
  (void)fprintf(wfp, control_string, wp_index);
     
  eighter = 0;
     
  for (i = 0; i<wp_index; i++)
    {
      if (eighter == 0) (void)fprintf(wfp, "\n");
      theref = wp_table[1+i];
      CONTIGIFY(theref);
      (void)fprintf(wfp, control_string, theref);
      eighter = (eighter + 1) % 8;
    }

  (void)fclose(wfp);
}




void dump_world(just_new)
     bool just_new;
{
  if (dump_binary)
    dump_binary_world(just_new);
  else
    dump_ascii_world(just_new);
}



void reoffset(baseAddr, start, count)
	register ref	baseAddr;
	register ref	*start;
	register long	count;
{
  register long	index;
  register ref	*next;
	
  next = start;
  for (index=0; index<count; index++) {
    if ( *next&2 )
      *next += baseAddr;
    next++;
  }
}

void read_world(str)
     char *str;
{
  FILE *d;
  int magichar;

  /* Replace "%%" in file name with "ol" on bigendian machines and
     with "lo" on littleendian ones. */

  {
    char *p = str;
#ifdef BIG_ENDIAN
    char *next = "ol";
#else
    char *next = "lo";
#endif

    while ((*p != '\0') && (*next != '\0'))
      if (*p == '%')
	*p++ = *next++;
      else
	p++;
  }

  if ((d = fopen(str, READ_BINARY_MODE)) == NULL)
    {
      (void)printf("Can't open \"%s\".\n", str);
      exit(1);
    }

  magichar = getc (d);
  if (magichar == (int)'\002')
    {
      (void)getc(d); (void)getc(d); (void)getc(d);
      input_is_binary = 1;
    }
  else
    {
      (void)ungetc(magichar, d);
      input_is_binary = 0;
#ifdef BIG_ENDIAN
      printf("Big Endian.\n");
#else
      printf("Little Endian.\n");
#endif
    }

  /* OBSOLESCENT: read val_space_size and cxt_space_size: */
  (void)read_ref(d);
  (void)read_ref(d);
  
  e_boot_code = read_ref(d);

  spatic.size = (unsigned long)read_ref(d);
  alloc_space(&spatic);

  e_boot_code += (ref)spatic.start;

  {
    long load_count;
    ref *mptr, next;

    load_count = spatic.size;
    mptr = spatic.start;
 
    if (input_is_binary) {
      (void)flread((char *)spatic.start, sizeof(ref), load_count, d);
      reoffset((ref) spatic.start, spatic.start, load_count);
    }
    else
      while (load_count != 0)
	{
	  next = read_ref(d);
	  if ( next&2 ) next += (ref)spatic.start;
	  *mptr++ = next;
	  --load_count;
	}
    /* Load the weak pointer table. */
    wp_index = read_ref(d);
    (void)flread((char *)&wp_table[1], sizeof(ref), (long)wp_index, d);
    reoffset((ref) spatic.start, &wp_table[1], wp_index);
  }

  /* The weak pointer hash table is rebuilt when e_nil is set. */

  (void)fclose(d);
}





/* This is used to decode the -h option on the command line. */
long string_to_int(string)
     char *string;
{
  long n = 0;
  char *cs = string;
  while(*cs >= '0' && *cs <= '9')
    n = n*10 + *cs++ - '0';
  while (1)
    switch(*cs++)
      {
      case 'K':
      case 'k':
	n *= 1024;
	break;
      case 'M':
      case 'm':
	n *= (1024 * 1024);
	break;
      case 'W':
      case 'w':
	n *= sizeof(ref);
	break;
      case '\0':
	return(n);
      default:
	printf("Unable to parse %s as a number.\n", string);
	exit(1);
      }
}

