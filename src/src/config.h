/*  Copyright (C) 1989, 1988, 1987 Barak Pearlmutter and Kevin Lang */

/*
 * This file describes the configuration of the machine that is being
 * compiled on.  It deals with quirks of the CPU, compiler, and OS.
 */


#ifndef DEFAULT_WORLD
#define DEFAULT_WORLD "/usr/misc/.oaklisp/lib/oaklisp.%%c"
#endif


/* This is measured in references, so the below is 1 megabyte of storage. */
#ifndef DEFAULT_NEW_SPACE_SIZE
#define DEFAULT_NEW_SPACE_SIZE	(256*1024L)
#endif

#if defined(__C11FEATURES__)
#if !defined(unix)
#define unix
#endif
#if !defined(__unix)
#define __unix
#endif
#if !defined(__unix__)
#define __unix__
#endif
#define SIGNALS
#if defined(__SunOS)
#define __with_signal_action
#define BSD_OR_MACH
#endif
#define WORDSIZE 32
#define PROTOTYPES
#define ASHR2(x) ((x)>>2)
#else

/* I'd love to be able to case things out here: */

/* Length of (long).  Must be <= length of (char *). */
#define WORDSIZE 32


#ifdef MACH
#define BSD_OR_MACH
#endif

#ifdef __MACH__
#define BSD_OR_MACH
#endif

#ifdef __sun__
#define BSD_OR_MACH
#endif

#ifdef __sun4__
#define BSD_OR_MACH
#endif

#ifdef BSD
#define BSD_OR_MACH
#endif

#ifdef sun
#define BSD_OR_MACH
#endif

#ifdef ultrix
#define BSD_OR_MACH
#endif



#ifdef AMIGA
#define MALLOC_STACK_BUFFER 
#define MALLOC_WP_TABLE__C11FEATURES__
#endif



/* Try to figure out the endianity. */

#ifdef CMUCS

#ifdef BYTE_MSF
#define BIG_ENDIAN
#endif

#else CMUCS

#ifndef vax
#ifndef mips
#ifndef i386

/* mac, AMIGA, sun, sparc, mc68000, convex: big endian */

#define BIG_ENDIAN

#endif i386
#endif mips
#endif vax

#endif CMUCS

/* This is for gcc with -ansi */
#ifdef __unix__
#define unix
#endif

/* This is for IBM RIOS under AIX */
#ifdef _AIX
#define unix
#define BSD_OR_MACH
#endif




#ifdef Mac_LSC
#define MALLOC_STACK_BUFFER 
#define MALLOC_WP_TABLE
#define PROTOTYPES
#define CANT_FLUSH_STD
#define mac
#endif



/* UNALIGNED should be defined if malloc() might return a pointer that isn't
   long aligned, ie, whose low two bits might not be 0. */

#ifdef mac
#define UNALIGNED
#endif




/* Expand this to include all systems with unsigned chars: */

#ifdef ibmrt
#define UNSIGNED_CHARS
#endif




#if defined(__STDC__ ) && !defined(__SunOS)
#define PROTOTYPES
#endif



/* Machines with int's smaller than (char *)'s should not define this. */
#ifndef Mac_LSC
#define BIGINT
#endif




/* unix_files is defined if things like ftell() and fseek() are
   around.  For things like fileno() and isatty() we need real unix. */

#ifdef unix
#define unix_files
#endif

#ifdef mac
#define unix_files
#endif

#ifdef AMIGA
#define unix_files
#endif






/* The following can be ((x)>>2) on machines with arithmetic right
   shifts of signed numbers.  But some machines (like the Convex)
   treat all numbers being shifted as unsigned.  If your compiler
   emits better code for (x>>2) than for (x/4) and does not have this
   weird problem, you should use the definition that is commented out.

   Most compilers emit the same code for x/4 as for x>>2, making this
   moot. */

#define ASHR2(x) ((x)/4)
/* #define ASHR2(x) ((x)>>2) */




/* Some machines have a 64 bit variant of long called a "long long",
   which makes multiplication overflow detection easier. */

#ifdef convex
#define HAVE_LONG_LONG
#endif



/* Some operating systems support trapping user interrupts. */

#ifdef unix
#define SIGNALS
#endif



/* Some operating systems support telling the VM system about big bites. */

/* REMOVED after benchmarks showed its uselessness. */

#ifdef unix
/* #define USE_VADVISE */
#endif



/* It is possible to use floating point hardware to detect overflows.
   This doesn't seem to be a speed win on any tested system, and is
   therefore commented out.  */

#ifndef HAVE_LONG_LONG
/* #define DOUBLES_FOR_OVERFLOW */
#endif
#endif




/* eof */
