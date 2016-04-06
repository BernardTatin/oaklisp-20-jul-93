/* Copyright (C) 1987,8,9 Barak Pearlmutter and Kevin Lang. */

/* An emulator for the Oaklisp virtual machine.  */


#include <stdio.h>
#include <ctype.h>
#if defined(__C11FEATURES__)
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "emulator.h"
#include "stacks.h"

#ifdef unix
#ifdef BSD_OR_MACH
#include <sys/time.h>
#include <sys/resource.h>
#endif
#ifdef USG
#include <sys/types.h>
#include <sys/param.h>
#include <sys/times.h>
#endif
#endif



#define CASE_FOUR 1

#ifdef FAST

#define trace_insts	0
#define trace_stcon	0
#define trace_cxcon	0
#define trace_meth	0
#define trace_segs	0
#define trace_mcache	0

#else

bool trace_insts = FALSE; /* trace instruction execution */
bool trace_stcon = FALSE; /* trace stack contents */
bool trace_cxcon = FALSE; /* trace contents stack contents */
bool trace_meth = FALSE; /* trace method lookup */
bool trace_segs = FALSE; /* trace stack segment manipulation */
bool trace_mcache = FALSE; /* trace method cache hits and misses */

extern char *ArglessInstrs[], *Instrs[];

#endif

bool trace_traps = FALSE; /* trace tag traps */
bool trace_files = FALSE; /* trace file opening */
bool trace_gc = FALSE; /* trace gc carefully */

bool dump_after = FALSE; /* dump world after running */
bool gc_before_dump = FALSE; /* do a GC before dumping the world */


#ifdef FAST
#define MAYBE_PUT(v,s)
#else
#define MAYBE_PUT(v,s) { if ((v)) {(void)printf(s); fflush_stdout();} }
#endif


#ifdef Mac_LSC
extern void Init_Primitives();
extern ref Call_Primitive(ref primRef, ref callRef, ref retRef, ref paramList);
#endif


/*
 * Processor registers
 */

stack val_stk, cxt_stk;

ref *e_bp, *e_env, e_t, e_nil,
e_fixnum_type, e_loc_type, e_cons_type, e_env_type, *e_subtype_table,
e_object_type, e_segment_type, e_boot_code, e_code_segment,
*e_arged_tag_trap_table, *e_argless_tag_trap_table, e_current_method,
e_uninitialized, e_method_type, e_operation_type; /*qqq*/

unsigned long
e_next_newspace_size,
        original_newspace_size = DEFAULT_NEW_SPACE_SIZE;

unsigned short *g_e_pc;

#define maybe_dump_world(dumpstackp)	\
{					\
  UNOPTV(val_stk.ptr = val_stk_ptr);	\
  UNOPTC(cxt_stk.ptr = cxt_stk_ptr);	\
  maybe_dump_world_aux((dumpstackp));	\
}


#define NEW_STORAGE e_uninitialized

void maybe_dump_world_aux(int dumpstackp) {
    if (dumpstackp > 2) /* 0,1,2 are normal exits. */ {
        printf("value ");
        dump_stack_proc(&val_stk);
        printf("context ");
        dump_stack_proc(&cxt_stk);
    }

    if (dump_after) {
        if (gc_before_dump && dumpstackp == 0) {
            gc(TRUE, TRUE, "impending world dump", 0L);
            dump_world(TRUE);
        } else
            dump_world(FALSE);
    }
}

void printref(ref refin) {
    unsigned long i;
    char suffix = '?';

    if ((refin & PTR_MASK) != 0) {
        ref *p = ((refin & 1) != 0) ? REF_TO_PTR(refin) : LOC_TO_PTR(refin);

        if (SPATIC_PTR(p)) {
            i = p - spatic.start;
            suffix = 's';
        } else if (NEW_PTR(p)) {
            i = (p - new.start) + spatic.size;
            suffix = 'n';
        } else i = (unsigned long) p >> 2;

        (void) printf("%ld~%d%c", i, refin&TAG_MASK, suffix);
    } else
        (void) printf("%ld~%d", refin >> 2, refin & TAG_MASK);
}



#define TRACEMETHOD(zz) {if (trace_meth) {printf("meth-trace%ld  ",zz);          \
					 printref("obj_type:%ld~%ld  ",obj_type); \
   				         printref("alist:%ld~%ld  ",alist);       \
				         printref("mptr:%ld~%ld\n",*method_ptr);  }}

#define TRACEASSQ(zz) {if (trace_meth) {printf("aq-trace%ld  ",zz);          \
   				         printref("elem:%ld~%ld  ",elem);       \
				         printref("list:%ld~%ld\n",list);  }}

#define TRACEPASSQ(zz) {printf("aq-trace%ld  ",zz);    \
			  printref("elem:%ld~%ld  ",elem);       \
			    printref("l:%ld~%ld ",l);  \
			    printref("cdr(l):%ld~%ld ",cdr(l));  \
			    if (locl) printref("*locl:%ld~%ld\n",*locl); }


/* these are inline coded now

ref assq(elem, list)
     ref elem, list;
{
  while (list != e_nil && car(car(list)) != elem) {
    list = cdr(list);
  }
  return ((list == e_nil)? e_nil : car(list));
}

ref old_pseudo_assq(elem, list)
     ref elem, list;
{
  while (list != e_nil && car(car(list)) != elem) {
    list = cdr(list);
  }
  return list;
}
 */


/* The following code uses the bring-to-front heuristic,
   and eventually needs a register to inhibit this behavior.
   This code is now inserted inline in the one place it is used.
ref pseudo_assq(elem, loclist)
ref elem, *loclist;
{
  ref thelist = *loclist;

  register ref l = thelist;
  register ref *locl = NULL;

  while (l != e_nil)
    {
      if (car(car(l)) == elem)
    {
      if (locl != NULL) {
 *locl = cdr(l);
 *loclist = l;
        cdr(l) = thelist;
      }
      return l;
    }
      l = *(locl = &(cdr(l)));
    }
  return l;
}
 */




#define get_type(x)							  \
((x)&1 ?								  \
 ((x)&2 ? REF_SLOT(x, 0) : *(e_subtype_table + ((x&SUBTAG_MASK)/4))) : \
 ((x)&2 ? e_loc_type : e_fixnum_type))

/* ((unsigned short *) (REF_TO_PTR(seg) + CODE_CODE_START_OFF)) */
#define CODE_SEG_FIRST_INSTR(seg) \
  ((unsigned short *)&REF_SLOT(seg,CODE_CODE_START_OFF))

void old_find_method_type_pair(ref op, ref obj_type, ref *method_ptr, ref *type_ptr) {
    register ref alist;
    register ref *locl = NULL;
    register ref thelist;
    register ref *loclist;

    while (1) {
        /* First look for it here: */
        /*alist=pseudo_assq(op,&REF_SLOT(obj_type,TYPE_OP_METHOD_ALIST_OFF));*/

        alist = thelist =
                *(loclist = &REF_SLOT(obj_type, TYPE_OP_METHOD_ALIST_OFF));

        while (alist != e_nil) {
            if (car(car(alist)) == op) {
                if (locl != NULL) {
                    *locl = cdr(alist);
                    *loclist = alist;
                    cdr(alist) = thelist;
                }
                *method_ptr = cdr(car(alist));
                *type_ptr = obj_type;
                return;
            }
            alist = *(locl = &cdr(alist));
        }

        /* Loop looking for it on supertypes: */
        alist = REF_SLOT(obj_type, TYPE_SUPER_LIST_OFF);
        if (alist == e_nil) return;

        while ((thelist = cdr(alist)) != e_nil) {
            old_find_method_type_pair(op, car(alist), method_ptr, type_ptr);

            /* If found on a supertype, we're done. */
            if (*method_ptr != e_nil) return;

            alist = thelist;
        }
        locl = NULL;
        obj_type = car(alist);
    }
}



/* This is a rewrite of find_method_type_pair that doesn't use
   recursion but rather an explicit stack.  Easier to inline. */

ref later_lists[100];

void find_method_type_pair(ref op, ref obj_type, ref *method_ptr, ref *type_ptr) {
    register ref alist;
    register ref *locl = NULL;
    ref thelist;
    ref *loclist;
    register ref *llp = &later_lists[0] - 1;

    while (1) {
        /* First look for it in the local method alist of obj_type: */

        alist = thelist =
                *(loclist = &REF_SLOT(obj_type, TYPE_OP_METHOD_ALIST_OFF));

        while (alist != e_nil) {
            if (car(car(alist)) == op) {
                if (locl != NULL) {
                    *locl = cdr(alist);
                    *loclist = alist;
                    cdr(alist) = thelist;
                }
                *method_ptr = cdr(car(alist));
                *type_ptr = obj_type;
                return;
            }
            alist = *(locl = &cdr(alist));
        }

        /* Not there, stack the supertype list and then fetch the top guy
       available from the stack. */

        *++llp = REF_SLOT(obj_type, TYPE_SUPER_LIST_OFF);

        while (*llp == e_nil) {
            if (llp == &later_lists[0]) return; /* Nothing. */
            llp -= 1;
        }

        locl = NULL;
        obj_type = car(*llp);
        *llp = cdr(*llp);
    }
}

/* This takes a length and a pointer to the beginning of an Oaklisp
   string and returns the equivalent C string.  You must remember to
   free() the storage returned by this routine. */

char *oak_c_string(unsigned int len, unsigned long *p) {
    char *stuff = my_malloc((long) (len + 1));
    int i = 0, j = 0;

    while (i + 2 < len) {
        unsigned long pp = *p++;
        stuff[j++] = (pp >> 2) & 0xFF;
        stuff[j++] = (pp >> 10) & 0xFF;
        stuff[j++] = (pp >> 18) & 0xFF;
        i += 3;
    }
    if (i + 1 < len) {
        unsigned long pp = *p;
        stuff[j++] = (pp >> 2) & 0xFF;
        stuff[j++] = (pp >> 10) & 0xFF;
        i += 2;
    } else if (i < len) {
        stuff[j++] = (*p >> 2) & 0xFF;
        i += 1;
    }
    stuff[j] = 0;
    return stuff;
}




char *dump_file = NULL;

void crunch_args(int argc, char **argv) {
    char *world_name;
    char *program_name = argv[0];
    argc -= 1;
    argv += 1;

    while (argc > 1 && (*argv)[0] == '-') {
        switch ((*argv)[1]) {
#ifndef FAST
            case 'i':
                trace_insts = 1;
                break;
            case 'c':
                trace_stcon = 1;
                break;
            case 'x':
                trace_cxcon = 1;
                break;
            case 'm':
                trace_meth = 1;
                break;
            case 'S':
                trace_segs = 1;
                break;
            case 'M':
                trace_mcache = 1;
                break;
#endif
            case 'T':
                trace_traps = 1;
                break;
            case 'F':
                trace_files = 1;
                break;
            case 'd':
                dump_after = 1;
                break;
            case 'h':
                argc -= 1;
                argv += 1;
                original_newspace_size = string_to_int(*argv) / sizeof (ref);
                break;
            case '9':
                dump_decimal = 1;
                break;
            case 'b':
                dump_binary = 1;
                break;
            case 'G':
                gc_before_dump = 1;
                break;
            case 'g':
                trace_gc = 1;
                break;
            case 'Q':
                gc_shutup = TRUE;
                break;
            case 'f':
                argc -= 1;
                argv += 1;
                dump_file = *argv;
                break;
            default:
                (void) printf("Unknown option %s.\n", argv[0]);
                break;
        }
        argc -= 1;
        argv += 1;
    }

#ifdef DEFAULT_WORLD
    if (argc == 0)
        world_name = DEFAULT_WORLD;
    else
#endif
        if (argc == 1)
        world_name = argv[0];
    else {
#ifndef FAST
        (void) printf("Usage: %s [-icxmSMTFd9bGgQ] [-h bytes] oaklisp-image\n",
                program_name);
#else
        (void) printf("Usage: %s [-TFd9bGgQ] [-h bytes] oaklisp-image\n",
                program_name);
#endif
        exit(2);
    }

#ifdef Mac_LSC
    Init_Primitives();
#endif

    init_wp();
    read_world(world_name);

    new.size = e_next_newspace_size = original_newspace_size;
    alloc_space(&new);
    free_point = new.start;

    init_stk(&val_stk);
    init_stk(&cxt_stk);
}




/* Do it this way so prototypes don't get messed up. */
#ifdef Mac_LSC
#define main _main
#endif

int main(int argc, char **argv) {
    unsigned int e_nargs;

    crunch_args(argc, argv);

    /* Get the registers set to the boot code. */

    e_current_method = e_boot_code;
    e_env = REF_TO_PTR(REF_SLOT(e_current_method, METHOD_ENV_OFF));
    e_code_segment = REF_SLOT(e_current_method, METHOD_CODE_OFF);
    g_e_pc = CODE_SEG_FIRST_INSTR(e_code_segment);

    /* Put a reasonable thing in e_bp so GC doesn't get pissed. */
    e_bp = e_env;

    /* Tell the boot function the truth: */
    e_nargs = 0;

    /* Okay, lets go: */

    {

        /* This is used for instructions to communicate with the trap code
           when a fault is encountered. */
        unsigned int trap_nargs;
        register unsigned short instr;
        register ref x, y;
        register unsigned short *e_pc = g_e_pc;
        register ref *val_stk_ptr = val_stk.ptr;
        ref *cxt_stk_ptr = cxt_stk.ptr;

#ifndef FAST
        FILE *debug;
        char str[255];
#endif

        /* This fixes a bug in which the initial CHECK-NARGS in the boot code
           tries to pop the operation and fails. */
        PUSHVAL_IMM(INT_TO_REF(4321));

        /* These TRAPx(n) macros jump to the trap code, notifying it that x
           arguments have been popped off the stack and need to be put back
           on (these are in the variables x, ...) and that the trap operation
           should be called with the top n guys on the stack as arguments. */


#define TRAP0(N) {trap_nargs=((N)); goto arg0_tt;}
#define TRAP1(N) {trap_nargs=((N)); goto arg1_tt;}

#define TRAP0_IF(C,N) {if ((C)) TRAP0((N));}
#define TRAP1_IF(C,N) {if ((C)) TRAP1((N));}

#define CHECKTAG0(X,TAG,N) TRAP0_IF(!TAG_IS((X),(TAG)),(N))
#define CHECKTAG1(X,TAG,N) TRAP1_IF(!TAG_IS((X),(TAG)),(N))

#define CHECKCHAR0(X,N) \
    TRAP0_IF(!SUBTAG_IS((X),CHAR_SUBTAG),(N))
#define CHECKCHAR1(X,N) \
    TRAP1_IF(!SUBTAG_IS((X),CHAR_SUBTAG),(N))

#define CHECKTAGS1(X0,T0,X1,T1,N) \
    TRAP1_IF( !TAG_IS((X0),(T0)) || !TAG_IS((X1),(T1)), (N))

#define CHECKTAGS_INT_1(X0,X1,N) \
    TRAP1_IF( (((X0)|(X1)) & TAG_MASK) != 0, (N))

#ifdef SIGNALS
#define POLL_SIGNALS()	if (signal_pending()) {goto intr_trap;}
#else
#define POLL_SIGNALS()
#endif

#ifndef FAST
        debug = fopen("debug", "w");
#endif

        /* This is the big instruction fetch/execute loop. */

#ifdef SIGNALS
        enable_signal_polling();
#endif

        while (1) {

#ifndef FAST
            if (trace_stcon) dump_val_stk();
            if (trace_cxcon) dump_cxt_stk();
#endif

            instr = *e_pc;

#define arg_field (instr>>8)
            /* #define signed_arg_field SIGN_8BIT_ARG(arg_field) */
#define signed_arg_field ((short)((short)instr >> 8))
#define op_field  ((instr & 0xFF) >> 2)

#ifndef FAST
            if (trace_insts) {
                (void) sprintf(str, "%ld: %s (%d %d)\n",
                        (SPATIC_PTR((ref *) e_pc) ?
                        e_pc - (unsigned short *) spatic.start :
                        e_pc - (unsigned short *) new.start
                        + 2 * spatic.size),
                        ((op_field == CASE_FOUR * 0) ?
                        ArglessInstrs[arg_field] :
                        Instrs[op_field / CASE_FOUR]),
                        op_field, arg_field);
                fputs(str, debug);
                fputs(str, stdout);
                fflush_stdout();
            }
#endif
            e_pc += 1;

            /* Interrupt polling belongs here, but in order to not slow
               things down too much it is instead put in all the
               instructions that can do transfers of control, eg branches
               and funcalls.  This cuts all loops without slowing down
               each instruction. */

            switch (op_field) {
                case (CASE_FOUR * 0): /* ARGLESS-INSTRUCTION xxxx */
                    switch (arg_field) {

                        case 0: /* NOOP */
                            break;

                        case 1: /* PLUS */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Tag trickery: */
#ifdef GUARD_BIT
                        {
                            register ref z = x + y;
                            if (OVERFLOWN(z)) TRAP1(2);
                            PEEKVAL() = z;
                        }
#else
#ifdef HAVE_LONG_LONG
                        {
                            long long a = (long) x + (long) y;
                            long highcrap = a >> (WORDSIZE - 2);
                            if (highcrap && (highcrap != -1L)) TRAP1(2);
                            PEEKVAL() = (ref) a;
                        }
#else
                        {
                            register long a = REF_TO_INT(x) + REF_TO_INT(y);
                            OVERFLOWN_INT(a, TRAP1(2));
                            PEEKVAL() = INT_TO_REF(a);
                        }
#endif
#endif
                            break;

                        case 2: /* NEGATE */
                            x = PEEKVAL();
                            CHECKTAG0(x, INT_TAG, 1);
                            /* The most negative fixnum's negation isn't a fixnum. */
                            if (x == MIN_REF) TRAP0(1);
                            /* Tag trickery: */
                            PEEKVAL() = -x;
                            break;

                        case 3: /* EQ? */
                            POPVAL(x);
                            PEEKVAL() = x == PEEKVAL() ? e_t : e_nil;
                            break;

                        case 4: /* NOT */
                            PEEKVAL() = PEEKVAL() == e_nil ? e_t : e_nil;
                            break;

                        case 5: /* TIMES */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Tag trickery: */
#ifdef HAVE_LONG_LONG
                        {
                            long long a = REF_TO_INT(x) * (long) y;
                            long highcrap = a >> (WORDSIZE - 2);
                            if ((highcrap != 0L) && (highcrap != -1L)) TRAP1(2);
                            PEEKVAL() = (ref) a;
                        }
#else
#ifdef GUARD_BIT
                        {
                            register ref z = REF_TO_INT(x) * (long) y;
                            /* Ineffective: */
                            if (OVERFLOWN(z)) TRAP1(2);
                            PEEKVAL() = z;
                        }
#else
#ifdef DOUBLES_FOR_OVERFLOW
                        {
                            double a = (double) REF_TO_INT(x)*(double) REF_TO_INT(y);
                            if (a < (double) ((long) MIN_REF / 4)
                                    || a > (double) ((long) MAX_REF / 4)) TRAP1(2);
                            PEEKVAL() = INT_TO_REF((long) a);
                        }
#else
                        {
                            long a = REF_TO_INT(x), b = REF_TO_INT(y);
                            unsigned long al, ah, bl, bh, hh, hllh, ll;
                            long answer;
                            bool neg = FALSE;
                            /* MNF check */
                            if (a < 0) {
                                a = -a;
                                neg = TRUE;
                            }
                            if (b < 0) {
                                b = -b;
                                neg = !neg;
                            }
                            al = a & 0x7FFF;
                            bl = b & 0x7FFF;
                            ah = (unsigned long) a >> 15;
                            bh = (unsigned long) b >> 15;
                            ll = al*bl;
                            hllh = al * bh + ah*bl;
                            hh = ah*bh;
                            if (hh || hllh >> 15) TRAP1(2);
                            answer = (hllh << 15) + ll;
                            OVERFLOWN_INT(answer, TRAP1(2));
                            PEEKVAL() = INT_TO_REF(neg ? -answer : answer);
                        }
#endif
#endif
#endif
                            break;

                        case 6: /* LOAD-IMM ; INLINE-REF */
                            /* align pc to next word boundary: */
                            if ((unsigned long) e_pc & 2)
                                e_pc += 1;

                            /*NOSTRICT*/
                            PUSHVAL(*(ref *) e_pc);
                            e_pc += sizeof (ref) / sizeof (*e_pc);
                            break;

                        case 7: /* DIV */
                            /* Sign of product of args. */
                            /* Round towards 0.  Obays identity w/ REMAINDER. */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Can't divide by 0, or the most negative fixnum by -1. */
                            if (y == INT_TO_REF(0) ||
                                    y == INT_TO_REF(-1) && x == MIN_REF) TRAP1(2);
                            /* Tag trickery: */
                            PEEKVAL() = INT_TO_REF((long) x / (long) y);
                            break;

                        case 8: /* =0? */
                            x = PEEKVAL();
                            CHECKTAG0(x, INT_TAG, 1);
                            PEEKVAL() = x == INT_TO_REF(0) ? e_t : e_nil;
                            break;

                        case 9: /* GET-TAG */
                            PEEKVAL() = INT_TO_REF(PEEKVAL() & TAG_MASK);
                            break;

                        case 10: /* GET-DATA */
                            /* With the moving gc, this should *NEVER* be used.

                               For ease of debugging with the multiple spaces, this
                               makes it seem like spatic and new spaces are contiguous,
                               is compatible with print_ref, and also with CRUNCH. */
                            x = PEEKVAL();
                            if (x & PTR_MASK) {
                                ref *p = (x & 1) ? REF_TO_PTR(x) : LOC_TO_PTR(x);

                                PEEKVAL() =
                                        INT_TO_REF(
                                        SPATIC_PTR(p) ?
                                        p - spatic.start :
                                        NEW_PTR(p) ?
                                        (p - new.start) + spatic.size :
                                        (/* This is one weird reference: */
                                        printf("GET-DATA of "),
                                        printref(x),
                                        printf("\n"),
                                        -(long) p - 1)
                                        );
                            } else
                                PEEKVAL() = x&~TAG_MASKL | INT_TAG;
                            break;

                        case 11: /* CRUNCH */
                            POPVAL(x); /* data */
                            y = PEEKVAL(); /* tag */
                            CHECKTAGS_INT_1(x, y, 2);
                        {
                            int tag = REF_TO_INT(y) & TAG_MASK;
                            ref z;

                            if (tag & PTR_MASK) {
                                long i = REF_TO_INT(x);

                                /* For now, preclude creation of very odd references. */
                                TRAP1_IF(i < 0, 2);
                                if (i < spatic.size)
                                    z = PTR_TO_LOC(spatic.start + i);
                                else if (i < (spatic.size + new.size))
                                    z = PTR_TO_LOC(new.start + (i - spatic.size));
                                else {
                                    TRAP1(2);
                                }
                            } else
                                z = x;

                            PEEKVAL() = z | tag;
                        }
                            break;

                        case 12: /* GETC */
                            /***************************** OBSOLETE? *********************/
                            /* Used in emergency cold load standard-input stream. */
                            PUSHVAL_IMM(CHAR_TO_REF(getc(stdin)));
                            break;

                        case 13: /* PUTC */
                            /* Used in emergency cold load standard-output stream and
                               for the warm boot message. */
                            x = PEEKVAL();
                            CHECKCHAR0(x, 1);
                            (void) putc(REF_TO_CHAR(x), stdout);
                            fflush_stdout();
                            if (trace_insts || trace_stcon || trace_cxcon)
                                (void)printf("\n");
                            break;

                        case 14: /* CONTENTS */
                            x = PEEKVAL();
                            CHECKTAG0(x, LOC_TAG, 1);
                            PEEKVAL() = *LOC_TO_PTR(x);
                            break;

                        case 15: /* SET-CONTENTS */
                            POPVAL(x);
                            CHECKTAG1(x, LOC_TAG, 2);
                            *LOC_TO_PTR(x) = PEEKVAL();
                            break;

                        case 16: /* LOAD-TYPE */
                            PEEKVAL() = get_type(PEEKVAL());
                            break;

                        case 17: /* CONS */
                        {
                            ref *p;

                            ALLOCATE_SS(p, 3L, "space crunch in CONS instruction");

                            *p = e_cons_type;
                            POPVAL(x);
                            *(p + CONS_PAIR_CAR_OFF) = x;
                            *(p + CONS_PAIR_CDR_OFF) = PEEKVAL();
                            PEEKVAL() = PTR_TO_REF(p);
                        }
                            break;

                        case 18: /* <0? */
                            x = PEEKVAL();
                            CHECKTAG0(x, INT_TAG, 1);
                            /* Tag trickery: */
                            PEEKVAL() = (long) x < 0 ? e_t : e_nil;
                            break;

                        case 19: /* MODULO */
                            /* Sign of divisor (thing being divided by). */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            if (y == INT_TO_REF(0)) TRAP1(2);
                        {
                            long a = REF_TO_INT(x) % REF_TO_INT(y);
                            if ((a < 0 && (long) y > 0) || ((long) y < 0 && (long) x > 0 && a > 0))
                                a += REF_TO_INT(y);
                            PEEKVAL() = INT_TO_REF(a);
                        }
                            break;

                        case 20: /* ASH */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Tag trickery: */
                        {
                            long b = REF_TO_INT(y);
                            if (b < 0)
                                PEEKVAL() = ((long) x >> -b) & ~TAG_MASKL;
                            else
                                PEEKVAL() = x << b;
                        }
                            break;

                        case 21: /* ROT */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Rotations can not overflow, but are kind of meaningless in
                               the infinite precision model we have.  This instr is used
                               only for computing string hashes and stuff like that. */
                        {
                            unsigned long a = (unsigned long) x;
                            long b = REF_TO_INT(y);

#ifdef GUARD_BIT
                            PEEKVAL()
                                    = FIX_GUARD_BIT((b < 0 ? (a>>-b | a << (WORDSIZE - 3 + b))
                                    : (a << b | a >> (WORDSIZE - 3 - b)))
                                    & ~TAG_MASKL);
#else
                            PEEKVAL()
                                    = (b < 0
                                    ? (a>>-b | a << (WORDSIZE - 2 + b))
                                    : (a << b | a >> (WORDSIZE - 2 - b)))
                                    & ~TAG_MASKL;
#endif
                        }
                            break;

                        case 22: /* STORE-BP-I */
                            POPVAL(x);
                            CHECKTAG1(x, INT_TAG, 2);
                            *(e_bp + REF_TO_INT(x)) = PEEKVAL();
                            break;

                        case 23: /* LOAD-BP-I */
                            x = PEEKVAL();
                            CHECKTAG0(x, INT_TAG, 1);
                            PEEKVAL() = *(e_bp + REF_TO_INT(x));
                            break;

                        case 24: /* RETURN */
                            POP_CONTEXT();
                            break;

                        case 25: /* ALLOCATE */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAG1(y, INT_TAG, 2);
                        {
                            ref *p;

                            ALLOCATE1(p, REF_TO_INT(y),
                                    "space crunch in ALLOCATE instruction", x);

                            *p = x;

                            PEEKVAL() = PTR_TO_REF(p++);

                            while (p < free_point)
                                *p++ = NEW_STORAGE;
                        }
                            break;

                        case 26: /* ASSQ */
                        {
                            register ref z;

                            POPVAL(z);
                            x = PEEKVAL();
                            /* y = assq(z,x); */
                            while (x != e_nil && car(car(x)) != z)
                                x = cdr(x);
                        }
                            PEEKVAL() = ((x == e_nil) ? e_nil : car(x));
                            break;

                        case 27: /* LOAD-LENGTH */
                            x = PEEKVAL();
                            PEEKVAL() =
                                    (TAG_IS(x, PTR_TAG) ?
                                    (REF_SLOT(REF_SLOT(x, 0), TYPE_VAR_LEN_P_OFF) == e_nil ?
                                    REF_SLOT(REF_SLOT(x, 0), TYPE_LEN_OFF) :
                                    REF_SLOT(x, 1)) :
                                    INT_TO_REF(0));
                            break;

                        case 28: /* PEEK */
                            PEEKVAL() = INT_TO_REF(*(short *) PEEKVAL());
                            break;

                        case 29: /* POKE */
                            POPVAL(x);
                            *(short *) x = REF_TO_INT(PEEKVAL());
                            break;

                        case 30: /* MAKE-CELL */
                        {
                            ref *p;

                            ALLOCATE_SS(p, 1L, "space crunch in MAKE-CELL instruction");

                            *p = PEEKVAL();
                            PEEKVAL() = PTR_TO_LOC(p);
                        }
                            break;

                        case 31: /* SUBTRACT */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Tag trickery: */
#ifdef GUARD_BIT
                        {
                            register ref z;
                            z = x - y;
                            if (OVERFLOWN(z)) TRAP1(2);
                            PEEKVAL() = z;
                        }
#else
                        {
                            long a = REF_TO_INT(x) - REF_TO_INT(y);
                            OVERFLOWN_INT(a, TRAP1(2));
                            PEEKVAL() = INT_TO_REF(a);
                        }
#endif
                            break;

                        case 32: /* = */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            PEEKVAL() = x == y ? e_t : e_nil;
                            break;

                        case 33: /* < */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Tag trickery: */
                            PEEKVAL() = (long) x < (long) y ? e_t : e_nil;
                            break;

                        case 34: /* LOG-NOT */
                            x = PEEKVAL();
                            CHECKTAG0(x, INT_TAG, 1);
                            /* Tag trickery: */
                            PEEKVAL() = ~x - (TAG_MASK - INT_TAG);
                            break;

                        case 35: /* LONG-BRANCH distance (signed) */
                            POLL_SIGNALS();
                            e_pc += ASHR2(SIGN_16BIT_ARG(*e_pc)) + 1;
                            break;

                        case 36: /* LONG-BRANCH-NIL distance (signed) */
                            POLL_SIGNALS();
                            POPVAL(x);
                            if (x == e_nil)
                                e_pc += ASHR2(SIGN_16BIT_ARG(*e_pc)) + 1;
                            else
                                e_pc += 1;
                            break;

                        case 37: /* LONG-BRANCH-T distance (signed) */
                            POLL_SIGNALS();
                            POPVAL(x);
                            if (x != e_nil)
                                e_pc += ASHR2(SIGN_16BIT_ARG(*e_pc)) + 1;
                            else
                                e_pc += 1;
                            break;

                        case 38: /* LOCATE-BP-I */
                            x = PEEKVAL();
                            CHECKTAG0(x, INT_TAG, 1);
                            PEEKVAL() = PTR_TO_LOC(e_bp + REF_TO_INT(x));
                            break;

                        case 39: /* LOAD-IMM-CON ; INLINE-REF */
                            /* This is like a LOAD-IMM followed by a CONTENTS. */
                            /* align pc to next word boundary: */
                            if ((unsigned long) e_pc & 2)
                                e_pc += 1;

                            /*NOSTRICT*/
                            x = *(ref *) e_pc;
                            e_pc += 2;

                            CHECKTAG1(x, LOC_TAG, 1);
                            PUSHVAL(*LOC_TO_PTR(x));
                            break;

                            /* Cons instructions. */

#define CONSINSTR(a,ins)					\
		{						\
		  x = PEEKVAL();				\
		  CHECKTAG0(x,PTR_TAG, a);			\
		  if (REF_SLOT(x,0) != e_cons_type)		\
		    {						\
		      if (trace_traps)				\
			(void)printf("Type trap in ins.\n");	\
		      TRAP0(a);					\
		    }						\
		}

                        case 40: /* CAR */
                            CONSINSTR(1, CAR);
                            PEEKVAL() = car(x);
                            break;

                        case 41: /* CDR */
                            CONSINSTR(1, CDR);
                            PEEKVAL() = cdr(x);
                            break;

                        case 42: /* SET-CAR */
                            CONSINSTR(2, SET - CAR);
                            POPVALS(1);
                            car(x) = PEEKVAL();
                            break;

                        case 43: /* SET-CDR */
                            CONSINSTR(2, SET - CDR);
                            POPVALS(1);
                            cdr(x) = PEEKVAL();
                            break;

                        case 44: /* LOCATE-CAR */
                            CONSINSTR(1, LOCATE - CAR);
                            PEEKVAL() = PTR_TO_LOC(&car(x));
                            break;

                        case 45: /* LOCATE-CDR */
                            CONSINSTR(1, LOCATE - CDR);
                            PEEKVAL() = PTR_TO_LOC(&cdr(x));
                            break;

                            /* Done with cons instructions. */

                        case 46: /* PUSH-CXT-LONG rel */
                            PUSH_CONTEXT(ASHR2(SIGN_16BIT_ARG(*e_pc)) + 1);
                            e_pc += 1;
                            break;

                        case 47: /* Call a primitive routine. */
#ifdef Mac_LSC
                        {
                            ref primRef, callRef, retRef, paramList;
                            POPVAL(primRef);
                            POPVAL(callRef);
                            POPVAL(retRef);
                            paramList = PEEKVAL();
                            PEEKVAL() = Call_Primitive(primRef, callRef, retRef, paramList);
                        }
#else
                            printf("Not configured for CALL-PRIMITIVE.\n");
#endif
                            break;

                        case 48: /* THROW */
                            POPVAL(x);
                            CHECKTAG1(x, PTR_TAG, 2);
                            y = PEEKVAL();
                            bash_val_height(REF_TO_INT(REF_SLOT(x, ESCAPE_OBJECT_VAL_OFF)));
                            bash_cxt_height(REF_TO_INT(REF_SLOT(x, ESCAPE_OBJECT_CXT_OFF)));
                            PUSHVAL(y);
                            POP_CONTEXT();
                            break;

                        case 49: /* GET-WP */
                            PEEKVAL() = ref_to_wp(PEEKVAL());
                            break;

                        case 50: /* WP-CONTENTS */
                            x = PEEKVAL();
                            CHECKTAG0(x, INT_TAG, 1);
                            PEEKVAL() = wp_to_ref(x);
                            break;

                        case 51: /* GC */
                            UNOPTC(cxt_stk.ptr = cxt_stk_ptr);
                            UNOPTV(val_stk.ptr = val_stk_ptr);
                            g_e_pc = e_pc;
                            gc(FALSE, FALSE, "explicit call", 0L);
                            e_pc = g_e_pc;
                            UNOPTV(val_stk_ptr = val_stk.ptr);
                            UNOPTC(cxt_stk_ptr = cxt_stk.ptr);
                            PUSHVAL(e_nil);
                            break;

                        case 52: /* BIG-ENDIAN? */
#ifdef BIG_ENDIAN
                            PUSHVAL(e_t);
#else
                            PUSHVAL(e_nil);
#endif
                            break;

                        case 53: /* VLEN-ALLOCATE */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAG1(y, INT_TAG, 2);
                        {
                            ref *p;

                            ALLOCATE1(p, REF_TO_INT(y),
                                    "space crunch in VARLEN-ALLOCATE instruction", x);

                            PEEKVAL() = PTR_TO_REF(p);

                            *p++ = x;
                            *p++ = y;

                            while (p < free_point)
                                *p++ = NEW_STORAGE;
                        }
                            break;

                        case 54: /* INC-LOC */
                            /* Increment a locative by an amount.  This is an instruction
                               rather than (%crunch (+ (%pointer loc) index) %locative-tag)
                               to avoid a window of gc vulnerability.  All such windows
                               must be fully closed before engines come up. */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS1(x, LOC_TAG, y, INT_TAG, 2);
                            PEEKVAL() = PTR_TO_LOC(LOC_TO_PTR(x) + REF_TO_INT(y));
                            break;

                        case 55: /* FILL-CONTINUATION */
                            /* This instruction fills a continuation object with
                               the appropriate values. */
                            CHECKVAL_POP(1);
                            FLUSHVAL(2);
                            FLUSHCXT(0);
#ifndef FAST
                            /* debugging check: */
                            if (val_stk_ptr != &val_stk.data[1])
                                printf("Value stack flushing error.\n");
                            if (cxt_stk_ptr != &cxt_stk.data[0] - 1)
                                printf("Context stack flushing error.\n");
#endif
                            x = PEEKVAL();
                            /* CHECKTAG0(x,PTR_TAG,1); */
                            REF_SLOT(x, CONTINUATION_VAL_SEGS) = val_stk.segment;
                            REF_SLOT(x, CONTINUATION_VAL_OFF)
                                    = INT_TO_REF(val_stk.pushed_count);
                            REF_SLOT(x, CONTINUATION_CXT_SEGS) = cxt_stk.segment;
                            REF_SLOT(x, CONTINUATION_CXT_OFF)
                                    = INT_TO_REF(cxt_stk.pushed_count);
                            /* Maybe it's a good idea to reload the buffer, but I'm
                               not bothering and things seem to work. */
                            /* CHECKCXT_POP(0); */
                            break;

                        case 56: /* CONTINUE */
                            /* Continue a continuation. */
                            /* Grab the continuation. */

                            POPVAL(x);
                            /* CHECKTAG1(x,PTR_TAG,1); */
                            y = PEEKVAL();
                            /* Pull the crap out of it. */

                            val_stk.segment = REF_SLOT(x, CONTINUATION_VAL_SEGS);
                            val_stk.pushed_count
                                    = REF_TO_INT(REF_SLOT(x, CONTINUATION_VAL_OFF));
                            val_stk_ptr = &val_stk.data[0] - 1;
                            PUSHVAL_NOCHECK(y);

                            cxt_stk.segment = REF_SLOT(x, CONTINUATION_CXT_SEGS);
                            cxt_stk.pushed_count
                                    = REF_TO_INT(REF_SLOT(x, CONTINUATION_CXT_OFF));
                            cxt_stk_ptr = &cxt_stk.data[0] - 1;
                            POP_CONTEXT();
                            break;

                        case 57: /* REVERSE-CONS */
                            /* This is just like CONS except that it takes its args
                               in the other order.  Makes open coded LIST better. */
                        {
                            ref *p;

                            ALLOCATE_SS(p, 3L, "space crunch in CONS instruction");

                            *p = e_cons_type;
                            POPVAL(x);
                            *(p + CONS_PAIR_CDR_OFF) = x;
                            *(p + CONS_PAIR_CAR_OFF) = PEEKVAL();
                            PEEKVAL() = PTR_TO_REF(p);
                        }
                            break;

                        case 58: /* MOST-NEGATIVE-FIXNUM? */
                            PEEKVAL() = PEEKVAL() == MIN_REF ? e_t : e_nil;
                            break;

                        case 59: /* FX-PLUS */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Tag trickery: */
                            PEEKVAL() = x + y;
                            break;

                        case 60: /* FX-TIMES */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Tag trickery: */
                            PEEKVAL() = REF_TO_INT(x) * y;
                            break;

                        case 61: /* GET-TIME */
                            /* Return CPU time in microseconds or #f if unavailable. */
#ifdef unix
                        {
#ifdef BSD_OR_MACH
                            struct rusage rusage_buff;
                            (void) getrusage(RUSAGE_SELF, &rusage_buff);
                            PUSHVAL_IMM(INT_TO_REF(1000000 * rusage_buff.ru_utime.tv_sec
                                    + rusage_buff.ru_utime.tv_usec));
#endif
#ifdef USG
                            struct tms tms;
                            (void) times(&tms);
                            PUSHVAL_IMM(INT_TO_REF(tms.tms_utime * (1000000 / HZ)));
#endif
                        }
#else
                            PUSHVAL(e_nil);
#endif
                            break;

                        case 62: /* REMAINDER */
                            /* Sign of dividend (thing being divided.) */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            if (y == INT_TO_REF(0)) TRAP1(2);
                            PEEKVAL() = INT_TO_REF(REF_TO_INT(x) % REF_TO_INT(y));
                            break;

                        case 63: /* QUOTIENTM */
                            /* Round towards -inf.  Obeys identity w/ MODULO. */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKTAGS_INT_1(x, y, 2);
                            /* Can't divide by 0, or the most negative fixnum by -1. */
                            if (y == INT_TO_REF(0) ||
                                    y == INT_TO_REF(-1) && x == MIN_REF) TRAP1(2);
                            /* Tag trickery: */
                            /* I can't seem to get anything like this to work: */
                            PEEKVAL() = INT_TO_REF(((long) x < 0 ^ (long) y < 0)
                                    ? -(long) x / -(long) y
                                    : (long) x / (long) y);
                        {
                            long a = (long) x / (long) y;
                            if ((long) x < 0 && (long) y > 0 && a * (long) y > (long) x ||
                                    (long) y < 0 && (long) x > 0 && a * (long) y < (long) x)
                                a -= 1;
                            PEEKVAL() = INT_TO_REF(a);
                        }
                            break;

                        case 64: /* FULL-GC */
                            UNOPTC(cxt_stk.ptr = cxt_stk_ptr);
                            UNOPTV(val_stk.ptr = val_stk_ptr);
                            g_e_pc = e_pc;
                            gc(FALSE, TRUE, "explicit call", 0L);
                            e_pc = g_e_pc;
                            UNOPTV(val_stk_ptr = val_stk.ptr);
                            UNOPTC(cxt_stk_ptr = cxt_stk.ptr);
                            PUSHVAL(e_nil);
                            break;

                        case 65: /* MAKE-LAMBDA */

                        {
                            ref *p;

                            ALLOCATE_SS(p, 8L, "space crunch in MAKE-LAMBDA instruction");

                            *p = e_operation_type;
                            *(p + OPERATION_LAMBDA_OFF) = PTR_TO_REF(p + OPERATION_LENGTH);
                            *(p + OPERATION_CACHE_TYPE_OFF) = NEW_STORAGE;
                            *(p + OPERATION_CACHE_METH_OFF) = NEW_STORAGE;
                            *(p + OPERATION_CACHE_TYPE_OFF_OFF) = NEW_STORAGE;
                            *(p + OPERATION_LENGTH) = e_method_type;
                            POPVAL(x);
                            *(p + OPERATION_LENGTH + METHOD_CODE_OFF) = x;
                            *(p + OPERATION_LENGTH + METHOD_ENV_OFF) = PEEKVAL();
                            PEEKVAL() = PTR_TO_REF(p);
                        }


                            break;


#ifndef FAST
                        default:
                            (void) printf("\nIllegal ARGLESS instruction %d.\n", arg_field);
                            maybe_dump_world(333);
                            exit(333);
#endif
                    }
                    break;

                case (CASE_FOUR * 1): /* HALT n */
                {
                    int halt_code = arg_field;
                    maybe_dump_world(halt_code);
                    exit(halt_code);
                }

                case (CASE_FOUR * 2): /* LOG-OP log-spec */
                    POPVAL(x);
                    y = PEEKVAL();
                    CHECKTAGS_INT_1(x, y, 2);
                    /* Tag trickery: */
                    PEEKVAL() = ((instr & (1 << 8) ? x & y : 0)
                            | (instr & (1 << 9) ? ~x & y : 0)
                            | (instr & (1 << 10) ? x&~y : 0)
                            | (instr & (1 << 11) ? ~x&~y : 0)) & ~TAG_MASKL;
                    break;

                case (CASE_FOUR * 3): /* BLT-STACK stuff,trash */
                {
                    register int stuff = arg_field & 0xF
                            , trash_m1 = (instr >> (8 + 4));

                    CHECKVAL_POP(stuff + trash_m1);

                    {
                        register ref *src = val_stk_ptr - stuff
                                , *dest = src - (trash_m1 + 1);

                        while (src < val_stk_ptr)
                            *++dest = *++src;

                        val_stk_ptr = dest;
                    }
                }
                    break;

                case (CASE_FOUR * 4): /* BRANCH-NIL distance (signed) */
                    POLL_SIGNALS();
                    POPVAL(x);
                    if (x == e_nil)
                        e_pc += signed_arg_field;
                    break;

                case (CASE_FOUR * 5): /* BRANCH-T distance (signed) */
                    POLL_SIGNALS();
                    POPVAL(x);
                    if (x != e_nil)
                        e_pc += signed_arg_field;
                    break;

                case (CASE_FOUR * 6): /* BRANCH distance (signed) */
                    POLL_SIGNALS();
                    e_pc += signed_arg_field;
                    break;

                case (CASE_FOUR * 7): /* POP n */
                    POPVALS((int) arg_field);
                    break;

                case (CASE_FOUR * 8): /* SWAP n */
                {
                    ref *other;
                    MAKE_BACK_VAL_PTR(other, (int) arg_field);
                    x = PEEKVAL();
                    PEEKVAL() = *other;
                    *other = x;
                }
                    break;

                case (CASE_FOUR * 9): /* BLAST n */
                    CHECKVAL_POP((int) arg_field);
                {
                    ref *other = val_stk_ptr - arg_field;
                    *other = POPVAL_NOCHECK();
                }
                    break;

                case (CASE_FOUR * 10): /* LOAD-IMM-FIX signed-arg */
                    /* Tag trickery and opcode knowledge changes this
                       PUSHVAL_IMM(INT_TO_REF(signed_arg_field));
                       to this: */
                    PUSHVAL_IMM((ref) (((short) instr) >> 6));
                    break;

                case (CASE_FOUR * 11): /* STORE-STK n */
                {
                    ref *other;

                    MAKE_BACK_VAL_PTR(other, (int) arg_field);
                    *other = PEEKVAL();
                }
                    break;

                case (CASE_FOUR * 12): /* LOAD-BP n */
                    PUSHVAL(*(e_bp + arg_field));
                    break;

                case (CASE_FOUR * 13): /* STORE-BP n */
                    *(e_bp + arg_field) = PEEKVAL();
                    break;

                case (CASE_FOUR * 14): /* LOAD-ENV n */
                    PUSHVAL(*(e_env + arg_field));
                    break;

                case (CASE_FOUR * 15): /* STORE-ENV n */
                    *(e_env + arg_field) = PEEKVAL();
                    break;

                case (CASE_FOUR * 16): /* LOAD-STK n */
                    /* All attempts to start this with if (arg_field == 0) for speed
                       have failed, so benchmark carefully before trying it. */
                {
                    ref *other;
                    MAKE_BACK_VAL_PTR(other, (int) arg_field);
                    PUSHVAL(*other);
                }
                    break;

                case (CASE_FOUR * 17): /* MAKE-BP-LOC n */
                    PUSHVAL(PTR_TO_LOC(e_bp + arg_field));
                    break;

                case (CASE_FOUR * 18): /* MAKE-ENV-LOC n */
                    PUSHVAL(PTR_TO_LOC(e_env + arg_field));
                    break;

                case (CASE_FOUR * 19): /* STORE-REG reg */
                    x = PEEKVAL();
                    switch (arg_field) {
                        case 0:
                            e_t = x;
                            break;
                        case 1:
                            e_nil = x;
                            wp_table[0] = e_nil;
                            rebuild_wp_hashtable();
                            break;
                        case 2:
                            e_fixnum_type = x;
                            break;
                        case 3:
                            e_loc_type = x;
                            break;
                        case 4:
                            e_cons_type = x;
                            break;
                        case 5:
                            CHECKTAG1(x, PTR_TAG, 1);
                            e_subtype_table = REF_TO_PTR(x) + 2;
                            break;
                        case 6:
                            CHECKTAG1(x, LOC_TAG, 1);
                            e_bp = LOC_TO_PTR(x);
                            break;
                        case 7:
                            CHECKTAG1(x, PTR_TAG, 1);
                            e_env = REF_TO_PTR(x);
                            break;
                        case 8:
                            CHECKTAG1(x, INT_TAG, 1);
                            e_nargs = REF_TO_INT(x);
                            break;
                        case 9:
                            e_env_type = x;
                            break;
                        case 10:
                            CHECKTAG1(x, PTR_TAG, 1);
                            e_argless_tag_trap_table = REF_TO_PTR(x) + 2;
                            break;
                        case 11:
                            CHECKTAG1(x, PTR_TAG, 1);
                            e_arged_tag_trap_table = REF_TO_PTR(x) + 2;
                            break;
                        case 12:
                            e_object_type = x;
                            break;
                        case 13:
                            e_boot_code = x;
                            break;
                        case 14:
                            CHECKTAG1(x, LOC_TAG, 1);
                            free_point = LOC_TO_PTR(x);
                            break;
                        case 15:
                            CHECKTAG1(x, LOC_TAG, 1);
                            new.end = LOC_TO_PTR(x);
                            break;
                        case 16:
                            e_segment_type = x;
                            BASH_SEGMENT_TYPE(x);
                            break;
                        case 17:
                            e_uninitialized = x;
                            break;
                        case 18:
                            CHECKTAG1(x, INT_TAG, 1);
                            e_next_newspace_size = REF_TO_INT(x);
                            break;
                        case 19:
                            e_method_type = x;
                            break;
                        case 20:
                            e_operation_type = x; /*qqq*/
                            break;
                        default:
                            (void) printf("STORE-REG %d, unknown register.\n", arg_field);
                            break;
                    }
                    break;

                case (CASE_FOUR * 20): /* LOAD-REG reg */
                {
                    ref z;

                    switch (arg_field) {
                        case 0:
                            z = e_t;
                            break;
                        case 1:
                            z = e_nil;
                            break;
                        case 2:
                            z = e_fixnum_type;
                            break;
                        case 3:
                            z = e_loc_type;
                            break;
                        case 4:
                            z = e_cons_type;
                            break;
                        case 5:
                            z = PTR_TO_REF(e_subtype_table - 2);
                            break;
                        case 6:
                            z = PTR_TO_LOC(e_bp);
                            break;
                        case 7:
                            z = PTR_TO_REF(e_env);
                            break;
                        case 8:
                            z = INT_TO_REF((long) e_nargs);
                            break;
                        case 9:
                            z = e_env_type;
                            break;
                        case 10:
                            z = PTR_TO_REF(e_argless_tag_trap_table - 2);
                            break;
                        case 11:
                            z = PTR_TO_REF(e_arged_tag_trap_table - 2);
                            break;
                        case 12:
                            z = e_object_type;
                            break;
                        case 13:
                            z = e_boot_code;
                            break;
                        case 14:
                            z = PTR_TO_LOC(free_point);
                            break;
                        case 15:
                            z = PTR_TO_LOC(new.end);
                            break;
                        case 16:
                            z = e_segment_type;
                            break;
                        case 17:
                            z = e_uninitialized;
                            break;
                        case 18:
                            z = INT_TO_REF(e_next_newspace_size);
                            break;
                        case 19:
                            z = e_method_type;
                            break;
                        case 20:
                            z = e_operation_type; /*qqq*/
                            break;
                        default:
                            (void) printf("LOAD-REG %d, unknown register.\n", arg_field);
                            z = e_nil;
                            break;
                    }
                    PUSHVAL(z);
                }
                    break;

                case (CASE_FOUR * 21): /* FUNCALL-CXT, FUNCALL-CXT-BR distance (signed) */
                    POLL_SIGNALS();
                    /* NOTE: (FUNCALL-CXT) == (FUNCALL-CXT-BR 0) */
                    PUSH_CONTEXT(signed_arg_field);

                    /* Fall through to tail recursive case: */
                    goto funcall_tail;

                case (CASE_FOUR * 22): /* FUNCALL-TAIL */

                    /* This polling should not be moved below the trap label, since
                       the interrupt code will fail on a fake instruction failure. */
                    POLL_SIGNALS();

                    /* This label allows us to branch here from the tag trap code. */
funcall_tail:

                    x = PEEKVAL();
                    CHECKTAG0(x, PTR_TAG, e_nargs + 1);
                    CHECKVAL_POP(1);
                    y = PEEKVAL_UP(1);

                    e_current_method = REF_SLOT(x, OPERATION_LAMBDA_OFF);

                    if (e_current_method == e_nil) { /* SEARCH */
                        ref y_type = (e_nargs == 0) ? e_object_type : get_type(y);
#ifndef NO_METH_CACHE
                        /* Check for cache hit: */
                        if (y_type == REF_SLOT(x, OPERATION_CACHE_TYPE_OFF)) {
                            MAYBE_PUT(trace_mcache, "H");
                            e_current_method = REF_SLOT(x, OPERATION_CACHE_METH_OFF);
                            e_bp =
                                    REF_TO_PTR(y) +
                                    REF_TO_INT(REF_SLOT(x, OPERATION_CACHE_TYPE_OFF_OFF));
                        } else
#endif
                        {
                            /* Search the type heirarchy. */
                            ref meth_type, offset = INT_TO_REF(0);

                            /******************************************************
                            find_method_type_pair(x, y_type,
                                      &e_current_method, &meth_type);
                             */

                            {
                                ref obj_type = y_type;
                                register ref alist;
                                register ref *locl = NULL;
                                ref thelist;
                                ref *loclist;
                                register ref *llp = &later_lists[0] - 1;

                                while (1) {
                                    /* First look for it in the local method alist of obj_type: */

                                    alist = thelist =
                                            *(loclist = &REF_SLOT(obj_type, TYPE_OP_METHOD_ALIST_OFF));

                                    while (alist != e_nil) {
                                        if (car(car(alist)) == x) {
                                            if (locl != NULL) {
                                                *locl = cdr(alist);
                                                *loclist = alist;
                                                cdr(alist) = thelist;
                                            }
                                            e_current_method = cdr(car(alist));
                                            meth_type = obj_type;
                                            goto found_it;
                                        }
                                        alist = *(locl = &cdr(alist));
                                    }

                                    /* Not there, stack the supertype list and then fetch the top guy
                                       available from the stack. */

                                    *++llp = REF_SLOT(obj_type, TYPE_SUPER_LIST_OFF);

                                    while (*llp == e_nil) {
                                        if (llp == &later_lists[0]) {
                                            if (trace_traps)
                                                (void)printf("No handler for operation!\n");
                                            TRAP0(e_nargs + 1);
                                        }
                                        llp -= 1;
                                    }

                                    locl = NULL;
                                    obj_type = car(*llp);
                                    *llp = cdr(*llp);
                                }
                            }

found_it:


                            /******************************************************/

                            /*
                            if (e_current_method == e_nil)
                              {
                            if (trace_traps)
                              (void)printf("No handler for operation!\n");
                            TRAP1(e_nargs+1);
                              }
                             */

                            /* This could be dispensed with if meth_type has no
                               ivars and isn't variable-length-mixin. */{
                                ref alist
                                = REF_SLOT(y_type, TYPE_TYPE_BP_ALIST_OFF);

                                while (alist != e_nil) {
                                    if (car(car(alist)) == meth_type) {
                                        offset = cdr(car(alist));
                                        break;
                                    }
                                    alist = cdr(alist);
                                }
                            }
                            e_bp = REF_TO_PTR(y) + REF_TO_INT(offset);
#ifndef NO_METH_CACHE
                            MAYBE_PUT(trace_mcache, "M");
                            /* Cache the results of this search. */
                            REF_SLOT(x, OPERATION_CACHE_TYPE_OFF) = y_type;
                            REF_SLOT(x, OPERATION_CACHE_METH_OFF) = e_current_method;
                            REF_SLOT(x, OPERATION_CACHE_TYPE_OFF_OFF) = offset;
#endif
                        }
                    } else if (!TAG_IS(e_current_method, PTR_TAG)
                            || REF_SLOT(e_current_method, 0) != e_method_type) {
                        /* TAG TRAP */
                        if (trace_traps)
                            (void)printf("Bogus or never defined operation.\n");
                        TRAP0(e_nargs + 1);
                    }

                    /* else it's a LAMBDA. */

                    x = e_current_method;

                    e_env = REF_TO_PTR(REF_SLOT(x, METHOD_ENV_OFF));
                    e_pc = CODE_SEG_FIRST_INSTR(e_code_segment =
                            REF_SLOT(x, METHOD_CODE_OFF));
                    break;

                case (CASE_FOUR * 23): /* STORE-NARGS n */
                    e_nargs = arg_field;
                    break;

                case (CASE_FOUR * 24): /* CHECK-NARGS n */
                    if (e_nargs != arg_field) {
                        if (trace_traps)
                            (void)printf("\n%d args passed; %d expected.\n",
                                e_nargs, arg_field);
                        TRAP0(e_nargs + 1);
                    }
                    POPVALS(1);
                    break;

                case (CASE_FOUR * 25): /* CHECK-NARGS-GTE n */
                    if (e_nargs < arg_field) {
                        if (trace_traps)
                            (void)printf("\n%d args passed; %d or more expected.\n", e_nargs, arg_field);
                        TRAP0(e_nargs + 1);
                    }
                    POPVALS(1);
                    break;

                case (CASE_FOUR * 26): /* STORE-SLOT n */
                    POPVAL(x);
                    CHECKTAG1(x, PTR_TAG, 2);
                    REF_SLOT(x, arg_field) = PEEKVAL();
                    break;

                case (CASE_FOUR * 27): /* LOAD-SLOT n */
                    CHECKTAG0(PEEKVAL(), PTR_TAG, 1);
                    PEEKVAL() = REF_SLOT(PEEKVAL(), arg_field);
                    break;

                case (CASE_FOUR * 28): /* MAKE-CLOSED-ENVIRONMENT n */
                    /* This code might be in error if arg_field == 0, which the
                       compiler should never generate. */
                {
                    register ref *p;
                    register int zarg_field = arg_field;
                    register ref z;

#ifndef FAST
                    if (zarg_field == 0) {
                        fprintf(stderr, "MAKE-CLOSED-ENVIRONMENT 0.\n");
                        fflush_stderr();
                    }
#endif

                    ALLOCATE_SS(p, (long) (zarg_field + 2),
                            "space crunch in MAKE-CLOSED-ENVIRONMENT");

                    CHECKVAL_POP(zarg_field - 1);

                    z = PTR_TO_REF(p);

                    *p++ = e_env_type;
                    *p++ = INT_TO_REF(zarg_field + 2);

                    while (zarg_field--)
                        *p++ = POPVAL_NOCHECK();

                    PUSHVAL_NOCHECK(z);

                    break;
                }

                case (CASE_FOUR * 29): /* PUSH-CXT rel */
                    PUSH_CONTEXT(signed_arg_field);
                    break;

                case (CASE_FOUR * 30): /* LOCATE-SLOT n */
                    PEEKVAL()
                            = PTR_TO_LOC(REF_TO_PTR(PEEKVAL()) + arg_field);
                    break;

                case (CASE_FOUR * 31): /* STREAM-PRIMITIVE n */
                    switch (arg_field) {
                        case 0: /* n=0: get standard input stream. */
                            PUSHVAL((ref) stdin);
                            break;
                        case 1: /* n=1: get standard output stream. */
                            PUSHVAL((ref) stdout);
                            break;
                        case 2: /* n=2: get standard error output stream. */
                            PUSHVAL((ref) stderr);
                            break;
                        case 3: /* n=3: fopen, mode READ */
                        case 4: /* n=4: fopen, mode WRITE */
                        case 5: /* n=5: fopen, mode APPEND */
                            POPVAL(x);
                            /* How about a CHECKTAG(x,LOC_TAG,) here, eh? */
                        {
                            char *s = oak_c_string((unsigned int) REF_TO_INT(PEEKVAL()),
                                    (unsigned long *) LOC_TO_PTR(x));
                            if (trace_files) (void)printf("About to open '%s'.\n", s);
                            PEEKVAL()
                                    = (ref) fopen(s, arg_field == 3 ? READ_MODE :
                                    arg_field == 4 ? WRITE_MODE : APPEND_MODE);
                            free(s);
                        }
                            break;

                        case 6: /* n=6: fclose */
                            PEEKVAL()
                                    = fclose((FILE *) PEEKVAL()) == EOF ? e_nil : e_t;
                            break;
                        case 7: /* n=7: fflush */
                            PEEKVAL() =
#ifdef Mac_LSC
                                    ((file *) PEEKVAL() == stdout || (file *) PEEKVAL() == stderr)
                                    ? e_t :
#endif
                                    fflush((FILE *) PEEKVAL()) == EOF ? e_nil : e_t;
                            break;
                        case 8: /* n=8: putc */
                            POPVAL(x);
                            y = PEEKVAL();
                            CHECKCHAR1(y, 2);
                            PEEKVAL()
                                    = putc(REF_TO_CHAR(y), (FILE *) x) == EOF ? e_nil : e_t;
                            break;
                        case 9: /* n=9: getc */
                        {
                            register int c = getc((FILE *) PEEKVAL());
#ifdef unix
                            /* When possible, if an EOF is read from an interactive
                               stream, the eof should be cleared so regular stuff
                               can be read thereafter. */
                            if (c == EOF) {
                                if (isatty(fileno((FILE *) PEEKVAL()))) {
                                    if (trace_files) (void)printf("Clearing EOF.\n");
                                    clearerr((FILE *) PEEKVAL());
                                }
                                PEEKVAL() = e_nil;
                            } else
                                PEEKVAL() = CHAR_TO_REF(c);
#else
                            PEEKVAL() = (c == EOF) ? e_nil : CHAR_TO_REF(c);
#endif
                        }
                            break;
                        case 10: /* n=10: check for interactiveness */
#ifdef unix
                            PEEKVAL() = isatty(fileno((FILE *) PEEKVAL())) ? e_t : e_nil;
#else
                            PEEKVAL() = PEEKVAL() == (ref) stdin ? e_t : e_nil;
#endif
                            break;
                        case 11: /* n=11: tell where we are */
#ifdef unix_files
                            PEEKVAL() = INT_TO_REF(ftell((FILE *) PEEKVAL()));
#else
                            PEEKVAL() = e_nil;
#endif
                            break;

                        case 12: /* n=12: set where we are */
                            POPVAL(x);
                        {
#ifdef unix_files
                            FILE *fd = (FILE *) x;
                            long i = REF_TO_INT(PEEKVAL());

                            PEEKVAL() = fseek(fd, i, 0) == 0 ? e_t : e_nil;
#else
                            PEEKVAL() = e_nil;
#endif
                        }
                            break;

                        case 13: /* n=13: change working directory */
                            POPVAL(x);
#ifdef unix_files
                        {
                            char *s = oak_c_string((unsigned int) REF_TO_INT(PEEKVAL()),
                                    (unsigned long *) LOC_TO_PTR(x));
                            PEEKVAL() = chdir(s) == 0 ? e_t : e_nil;
                            free(s);
                        }
#else
                            PEEKVAL() = e_nil;
#endif
                            break;

                        default:
                            (void) printf("\nNonexistent STREAM-PRIMITIVE %d.\n",
                                    arg_field);
                            maybe_dump_world(333);
                            exit(333);
                            break;
                    }
                    break;

                case (CASE_FOUR * 32): /* FILLTAG n */
                    x = PEEKVAL();
                    CHECKTAG0(x, PTR_TAG, 1);
                    REF_SLOT(x, ESCAPE_OBJECT_VAL_OFF) = INT_TO_REF(val_height()
                            - arg_field);
                    REF_SLOT(x, ESCAPE_OBJECT_CXT_OFF) = INT_TO_REF(cxt_height());
                    break;

                case (CASE_FOUR * 33): /* ^SUPER-CXT, ^SUPER-CXT-BR distance */
                    /* Analogous to FUNCALL-CXT[-BR]. */

                    POLL_SIGNALS();
                    PUSH_CONTEXT(signed_arg_field);

                    /* Fall through to tail recursive case: */
                    goto super_tail;

                case (CASE_FOUR * 34): /* ^SUPER-TAIL */

                    /* Do not move this below the label! */
                    POLL_SIGNALS();

super_tail:

                    /* No cache, no LAMBDA hack, things are easy.
                       Maybe not looking at the lambda hack is a bug?

                       On stack: type, operation, self, args... */{
                        ref the_type;
                        ref y_type;
                        ref meth_type;

                        POPVAL(the_type);
                        CHECKTAG1(the_type, PTR_TAG, e_nargs + 2);

                        x = PEEKVAL(); /* The operation. */
                        CHECKTAG1(x, PTR_TAG, e_nargs + 2);

                        CHECKVAL_POP(1);

                        y = PEEKVAL_UP(1); /* Self. */

                        y_type = get_type(y);

                        e_current_method = e_nil;

                        find_method_type_pair(x, the_type,
                        &e_current_method, &meth_type);

                        if (e_current_method == e_nil) {
                            if (trace_traps)
                                (void)printf("No handler for ^super operation.\n");
                            PUSHVAL(the_type);
                            TRAP0(e_nargs + 2);
                        }

                        /* This could be dispensed with if meth_type has no
                       ivars and isn't variable-length-mixin. */
                        {
                            ref alist = REF_SLOT(y_type, TYPE_TYPE_BP_ALIST_OFF);
                            ref offset = INT_TO_REF(0);

                            while (alist != e_nil) {
                                if (car(car(alist)) == meth_type) {
                                    offset = cdr(car(alist));
                                    break;
                                }
                                alist = cdr(alist);
                            }
                            e_bp = REF_TO_PTR(y) + REF_TO_INT(offset);
                        }
                    }

                    x = e_current_method;

                    e_env = REF_TO_PTR(REF_SLOT(x, METHOD_ENV_OFF));
                    e_pc = CODE_SEG_FIRST_INSTR(e_code_segment =
                            REF_SLOT(x, METHOD_CODE_OFF));
                    break;

#ifndef FAST
                default:
                    (void) printf("\nIllegal Bytecode %d.\n", op_field);
                    maybe_dump_world(333);
                    exit(333);
#endif
            }
        }

        /* The above loop is infinite; we branch down to here when instructions
           fail, normally from tag traps, and then branch back. */

#ifdef SIGNALS
intr_trap:

        clear_signal();

        if (trace_traps)
            (void)printf("\nINTR: opcode %d, argfield %d.", op_field, arg_field);

        /* We notify Oaklisp of the user trap by telling it that a noop
           instruction failed.  The Oaklisp trap code must be careful to
           return nothing extra on the stack, and to restore NARGS
           properly.  It is passed the old NARGS. */

        /* the NOOP instruction. */
        instr = 0;

        /* Back off of the current intruction so it will get executed when
           we get back from the trap code. */
        e_pc -= 1;

        /* Pass the trap code the current NARGS. */
        x = INT_TO_REF(e_nargs);
        TRAP1(1);
#endif

arg1_tt:
        CHECKVAL_PUSH(3);
        PUSHVAL_NOCHECK(x);
arg0_tt:
        if (trace_traps) {
            (void) printf("\nTag trap: opcode %d, argfield %d.\n",
                    op_field, arg_field);
            (void) printf("Top of stack: ");
            printref(x);
            (void) printf(", pc =  %ld\n",
                    (/*NOSTRICT*/ SPATIC_PTR((ref *) e_pc)
                    ? e_pc - (unsigned short *) spatic.start
                    : e_pc - (unsigned short *) new.start
                    + 2 * spatic.size));
        }

        /* Trick: to preserve tail recursiveness, push context only if next
           instruction isn't a RETURN and current instruction wasn't a FUNCALL.
           or a CHECK-NARGS[-GTE]. */

        /* NOTE: It might be worth making sure op_field isn't recomputed
           many times here if your compiler is stupid. */

        if (*e_pc != (24 << 8) + 0 && op_field != 21 && op_field != 22
                && op_field != 24 && op_field != 25)
            PUSH_CONTEXT(0);

        /* Trapping instructions stash their argument counts here: */
        e_nargs = trap_nargs;

        if (op_field == 0) {
            /* argless instruction. */
            PUSHVAL_NOCHECK(*(e_argless_tag_trap_table + arg_field));
        } else {
            /* arg'ed instruction, so push arg field as extra argument */

            PUSHVAL_NOCHECK(INT_TO_REF(arg_field));
            e_nargs += 1;

            PUSHVAL_NOCHECK(*(e_arged_tag_trap_table + op_field));
        }

        if (trace_traps) {
            (void) printf("Dispatching to ");
            printref(PEEKVAL());
            (void) printf(" with NARGS = %d.\n", e_nargs);
        }

        /* Set the instruction dispatch register in case the FUNCALL fails. */

        instr = (22 << 2);

        goto funcall_tail;
    }
}
