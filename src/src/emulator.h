/*  Copyright (C) 1987,8,9 Barak Pearlmutter and Kevin Lang    */

#include "config.h"

/* Define this if you want a guard bit in fixnums. */

/* #define GUARD_BIT */


#define READ_MODE "r"
#define WRITE_MODE "w"
#define APPEND_MODE "a"


#ifdef Mac_LSC

#define READ_BINARY_MODE "r+b"
#define WRITE_BINARY_MODE "w+b"

#else // Mac_LSC

#define READ_BINARY_MODE READ_MODE
#define WRITE_BINARY_MODE WRITE_MODE

#endif // Mac_LSC


#ifdef CANT_FLUSH_STD
#define fflush_stdout()
#define fflush_stderr()
#else
#define fflush_stdout() (void)fflush(stdout)
#define fflush_stderr() (void)fflush(stderr)
#endif



#ifndef NULL
#define NULL 0L
#endif


#if defined(__C11FEATURES__)
#include <stdbool.h>

#ifndef FALSE
#define FALSE	false
#endif
#ifndef TRUE
#define TRUE	true
#endif
#else
typedef int bool;

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif
#endif


#ifdef UNSIGNED_CHARS
#define SIGN_8BIT_ARG(c)	(((c) & 0x80) ? ((c) | 0xffffff80) : (c))
#else
#define SIGN_8BIT_ARG(x)	((char)(x))
#endif


#define SIGN_16BIT_ARG(x)	((short)(x))


typedef unsigned long ref;


#define TAG_MASK	3
#define TAG_MASKL	3L
#define SUBTAG_MASK	0xFF
#define SUBTAG_MASKL	0xFFL

#define INT_TAG		0
#define IMM_TAG		1
#define LOC_TAG 	2
#define PTR_TAG		3

#define PTR_MASK	2

#define CHAR_SUBTAG	((0<<2)+IMM_TAG)

#define TAG_IS(X,TAG)		(((X)&TAG_MASK)==(TAG))
#define SUBTAG_IS(X,SUBTAG)	(((X)&SUBTAG_MASK)==(SUBTAG))

#define REF_TO_INT(r)	((long)ASHR2((long)(r)))

#define REF_TO_PTR(r)	((ref *)((r)-PTR_TAG))
#define LOC_TO_PTR(r)	((ref *)((r)-LOC_TAG))

#define ANY_TO_PTR(r)	((ref *)((r)&~TAG_MASKL))


#define PTR_TO_LOC(p)	((ref)((ref)(p)+LOC_TAG))
#define PTR_TO_REF(p)	((ref)((ref)(p)+PTR_TAG))

#define REF_TO_CHAR(r)	((char)((r)>>8))
#define CHAR_TO_REF(c)	(((ref)(c)<<8)+CHAR_SUBTAG)




/* MIN_REF is the most negative fixnum.  There is no corresponding
   positive fixnum, an asymmetry inherent in a twos complement
   representation. */

#ifndef GUARD_BIT

#define INT_TO_REF(i)	((ref)(((long)(i)<<2)+INT_TAG))
#define MIN_REF		((ref)(1L<<(WORDSIZE-1)))
/* Check if high three bits are equal. */
#define OVERFLOWN_INT(i,code)					\
{ register int highcrap = ((unsigned long)(i)) >> (WORDSIZE-3);	\
  if ((highcrap != 0x0) && (highcrap != 0x7)) {code;} }

#else

#define INT_TO_REF(i)	((ref)(((long)(i)<<3)>>1)+INT_TAG)
#define MIN_REF		((ref)(3L<<(WORDSIZE-2)))
/* Check if high bit and next-to-high bit are unequal. */
#define OVERFLOWN(r) (((r)>>(WORDSIZE-1)) != (((r)>>(WORDSIZE-2))&1))
#define FIX_GUARD_BIT(r)	((ref)((((long)(r))<<1)>>1))

#endif

#define MAX_REF ((ref)-((long)MIN_REF+1))



/*
 * Offsets for wired types.  Offset includes type and
 * optional length fields when present.
 */

/* CONS-PAIR: */
#define CONS_PAIR_CAR_OFF	1
#define CONS_PAIR_CDR_OFF	2

/* TYPE: */
#define TYPE_LEN_OFF		1
#define TYPE_VAR_LEN_P_OFF	2
#define TYPE_SUPER_LIST_OFF	3
#define TYPE_IVAR_LIST_OFF	4
#define TYPE_IVAR_COUNT_OFF	5
#define TYPE_TYPE_BP_ALIST_OFF	6
#define TYPE_OP_METHOD_ALIST_OFF 7
#define TYPE_WIRED_P_OFF	8

/* METHOD: */
#define METHOD_CODE_OFF		1
#define METHOD_ENV_OFF		2

/* CODE-VECTOR: */
#define CODE_IVAR_MAP_OFF	2
#define CODE_CODE_START_OFF	3

/* OPERATION: */
#define OPERATION_LAMBDA_OFF		1
#define OPERATION_CACHE_TYPE_OFF	2
#define OPERATION_CACHE_METH_OFF	3
#define OPERATION_CACHE_TYPE_OFF_OFF	4
#define OPERATION_LENGTH		5

/* ESCAPE-OBJECT */
#define ESCAPE_OBJECT_VAL_OFF	1
#define ESCAPE_OBJECT_CXT_OFF	2

/* Continuation Objects */
#define CONTINUATION_VAL_SEGS	1
#define CONTINUATION_VAL_OFF	2
#define CONTINUATION_CXT_SEGS	3
#define CONTINUATION_CXT_OFF	4

#define car(x)	(REF_SLOT((x),CONS_PAIR_CAR_OFF))
#define cdr(x)	(REF_SLOT((x),CONS_PAIR_CDR_OFF))


extern char *dump_file;

extern ref e_t, e_nil, e_fixnum_type, e_loc_type, e_cons_type,
  *e_subtype_table, *e_bp, *e_env, e_env_type, *e_argless_tag_trap_table,
  *e_arged_tag_trap_table, e_object_type, e_segment_type, e_code_segment,
  e_boot_code, e_current_method, e_uninitialized, e_method_type;
extern unsigned long e_next_newspace_size, original_newspace_size;
extern unsigned short *g_e_pc;

extern bool trace_segs;

extern bool dump_decimal, dump_binary;



typedef struct
{
  ref *start, *end;
  long size;
#ifdef UNALIGNED
  char offset;
#endif
} space;

extern space spatic, new;
extern ref *free_point;


#define SPACE_PTR(s,p)	((s).start<=(p) && (p)<(s).end)

#define NEW_PTR(r)	SPACE_PTR(new,(r))
#define SPATIC_PTR(r)	SPACE_PTR(spatic,(r))


/* Leaving r unsigned lets us checks for negative and too big in one shot: */
#define wp_to_ref(r)					\
  ( (unsigned long)REF_TO_INT(r) >= wp_index ?		\
   e_nil : wp_table[1+(unsigned long)REF_TO_INT((r))] )


#ifdef MALLOC_WP_TABLE
extern ref *wp_table;
#else
extern ref wp_table[];
#endif

extern long wp_index;

extern bool gc_shutup;

extern ref *gc_examine_ptr;
#define GC_MEMORY(v) {*gc_examine_ptr++ = (v);}
#define GC_RECALL(v) {(v) = *--gc_examine_ptr;}


/* This is used to allocate some storage.  It calls gc when necessary. */

#define ALLOCATE(p, words, place)			\
  ALLOCATE_PROT(p, words, place, , )

/* This is used to allocate some storage while the stack pointers have not
   been backed into the structures and must be backed up before gc. */

#define ALLOCATE_SS(p, words, place)			\
  ALLOCATE_PROT(p, words, place,			\
		{ UNOPTC(cxt_stk.ptr = cxt_stk_ptr);	\
		  UNOPTV(val_stk.ptr = val_stk_ptr);	\
		  g_e_pc = e_pc; },			\
		{ e_pc = g_e_pc;			\
		  UNOPTV(val_stk_ptr = val_stk.ptr);	\
		  UNOPTC(cxt_stk_ptr = cxt_stk.ptr); })

/* This allocates some storange, assumeing the stack pointers have not been
   backed into the structures and that v must be protected from gc. */

#define ALLOCATE1(p, words, place, v)			\
  ALLOCATE_PROT(p, words, place,			\
		{ GC_MEMORY(v);				\
		  UNOPTC(cxt_stk.ptr = cxt_stk_ptr);	\
		  UNOPTV(val_stk.ptr = val_stk_ptr);	\
		  g_e_pc = e_pc; },			\
		{ e_pc = g_e_pc;			\
		  UNOPTV(val_stk_ptr = val_stk.ptr);	\
		  UNOPTC(cxt_stk_ptr = cxt_stk.ptr);	\
		  GC_RECALL(v); })

#define ALLOCATE_PROT(p, words, place, before, after)	\
  /* ref *p; int words; string place; */		\
{							\
  ref *new_free_point = free_point + (words);		\
							\
  if (new_free_point >= new.end)			\
    {							\
      before;						\
      gc(FALSE, FALSE, (place), (words));		\
      after;						\
      new_free_point = free_point + (words);		\
    }							\
							\
  (p) = free_point;					\
  free_point = new_free_point;				\
}



/* These get slots out of Oaklisp objects, and may be used as lvalues. */

#define SLOT(p,s)	(*((p)+(s)))
#define REF_SLOT(r,s)	SLOT(REF_TO_PTR(r),s)



#ifdef SIGNALS


#ifdef __STDC__
extern volatile int signal_poll_flag;
#else
extern int signal_poll_flag;
#endif

#define signal_pending()	(signal_poll_flag)

#endif






#ifdef PROTOTYPES

#include "stacks.h"
#include "Proto.h"

#if !defined(__C11FEATURES__)
extern void exit(int);

#ifdef unix
extern int isatty(int);
extern int chdir(char *);
extern char *malloc(unsigned);
extern void free(char *);
#else
#ifdef Mac_LSC
#include <storage.h>
#endif
#endif
#else
void free_space(), alloc_space(), realloc_space();
char *my_malloc();

void printref(), init_wp();

void read_world();
ref read_ref();
void dump_world();

long string_to_int();

unsigned long get_length();

ref ref_to_wp();
void rebuild_wp_hashtable();

void gc();
void gc_printref();
void enable_signal_polling(), disable_signal_polling(), clear_signal();
#endif

#else

extern void free_space(), alloc_space(), realloc_space();
extern char *my_malloc();

extern void printref(), init_wp();

extern void read_world();
extern ref read_ref();
extern void dump_world();

extern long string_to_int();

extern unsigned long get_length();

extern ref ref_to_wp();
extern void rebuild_wp_hashtable();

extern void gc();
extern void gc_printref();
extern void enable_signal_polling(), disable_signal_polling(), clear_signal();

#endif

/* eof */
