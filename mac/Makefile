

# NOTE: This makefile is automatically generated by make-makefile.oak,
# and should not normally be edited by hand.  New makefiles are
# generated with 'make Makefile'.

COLDFILES = st.oa da.oa pl.oa do.oa cold-booting.oa kernel0.oa kernel0types.oa kernel1-install.oa kernel1-funs.oa kernel1-make.oa kernel1-freeze.oa kernel1-maketype.oa kernel1-inittypes.oa kernel1-segments.oa super.oa kernel.oa patch0symbols.oa mix-types.oa operations.oa ops.oa truth.oa logops.oa consume.oa conses.oa coerce.oa eqv.oa mapping.oa fastmap.oa fluid.oa vector-type.oa vl-mixin.oa numbers.oa subtypes.oa weak.oa strings.oa sequences.oa undefined.oa subprimitive.oa gc.oa tag-trap.oa code-vector.oa hash-table.oa format.oa signal.oa error.oa symbols.oa print-noise.oa patch-symbols.oa predicates.oa print.oa print-integer.oa print-list.oa reader-errors.oa reader.oa read-token.oa reader-macros.oa hash-reader.oa read-char.oa locales.oa expand.oa make-locales.oa patch-locales.oa freeze.oa bp-alist.oa describe.oa warm.oa interpreter.oa top-level.oa booted.oa dump-stack.oa file-errors.oa streams.oa cold.oa nargs.oa has-method.oa op-error.oa error2.oa error3.oa backquote.oa file-io.oa fasl.oa load-oaf.oa load-file.oa string-stream.oa list.oa catch.oa continuation.oa unwind-protect.oa bounders.oa anonymous.oa sort.oa exit.oa export.oa cold-boot-end.oa

COLDFILESD = cold-booting.oa kernel0.oa do.oa kernel0types.oa do.oa kernel1-install.oa do.oa kernel1-funs.oa do.oa kernel1-make.oa do.oa kernel1-freeze.oa do.oa kernel1-maketype.oa pl.oa kernel1-inittypes.oa pl.oa kernel1-segments.oa pl.oa super.oa pl.oa kernel.oa pl.oa patch0symbols.oa pl.oa mix-types.oa st.oa operations.oa st.oa ops.oa st.oa truth.oa st.oa logops.oa st.oa consume.oa st.oa conses.oa st.oa coerce.oa st.oa eqv.oa pl.oa mapping.oa pl.oa fastmap.oa pl.oa fluid.oa pl.oa vector-type.oa pl.oa vl-mixin.oa pl.oa numbers.oa pl.oa subtypes.oa pl.oa weak.oa pl.oa strings.oa pl.oa sequences.oa pl.oa undefined.oa da.oa subprimitive.oa da.oa gc.oa da.oa tag-trap.oa da.oa code-vector.oa da.oa hash-table.oa da.oa format.oa da.oa signal.oa pl.oa error.oa da.oa symbols.oa da.oa print-noise.oa da.oa patch-symbols.oa da.oa predicates.oa da.oa print.oa do.oa print-integer.oa do.oa print-list.oa do.oa reader-errors.oa do.oa reader.oa do.oa read-token.oa do.oa reader-macros.oa do.oa hash-reader.oa pl.oa read-char.oa pl.oa locales.oa do.oa expand.oa do.oa make-locales.oa do.oa patch-locales.oa do.oa freeze.oa do.oa bp-alist.oa do.oa describe.oa do.oa warm.oa do.oa interpreter.oa pl.oa top-level.oa pl.oa booted.oa st.oa dump-stack.oa do.oa file-errors.oa do.oa streams.oa do.oa cold.oa do.oa nargs.oa pl.oa has-method.oa pl.oa op-error.oa pl.oa error2.oa pl.oa error3.oa pl.oa backquote.oa pl.oa file-io.oa pl.oa fasl.oa pl.oa load-oaf.oa pl.oa load-file.oa pl.oa string-stream.oa pl.oa list.oa pl.oa catch.oa da.oa continuation.oa da.oa unwind-protect.oa da.oa bounders.oa do.oa anonymous.oa pl.oa sort.oa pl.oa exit.oa pl.oa export.oa st.oa st.oa st.oa cold-boot-end.oa

MISCFILES = macros0.oa obsolese.oa destructure.oa macros1.oa macros2.oa icky-macros.oa define.oa del.oa promise.oa bignum.oa bignum2.oa rational.oa complex.oa rounding.oa lazy-cons.oa math.oa trace.oa apropos.oa time.oa

COMPFILES = crunch.oa mac-comp-stuff.oa mac-compiler-nodes.oa mac-compiler1.oa mac-compiler2.oa mac-compiler3.oa mac-code.oa assembler.oa peephole.oa file-compiler.oa compiler-exports.oa

BATFILES = batch-compiler.oa

RNRSFILES = scheme-upgrade.oa scheme.oa scheme-macros.oa

COLDFILESDNX = $(COLDFILESD:.oa=)
TOOLFILES  = tool.oa
FILESFILES = files.oa
MAKEFILES  = make-makefile.oa
# These are gravy.  The first two are our standard benchmarks.  The others
# are neat.
GRAVY = tak.oak compile-bench.oak prolog.oak prolog-examples.oak

ALLOAFILES = $(TOOLFILES) $(MAKEFILES) $(FILESFILES) $(COLDFILES) \
		$(MISCFILES) $(COMPFILES) $(BATFILES) $(RNRSFILES)

SOURCE = $(ALLOAFILES:.oa=.oak) Makefile

# How we deal with the endianity problem:
#
# Bigendian machines need binary oaklisp worlds with extension .ol*
# while littleendian ones need .lo*.
#
# If on a system without the fancy CMU mods to make, just change the
# line 'E = ol' to 'E = ol' if you are on a littleendian machine
#
# On the other hand, this might work even if you're on such a machine.

VAX_EXT		= lo
PMAX_EXT	= lo
PDP11_EXT	= lo
SUN_EXT		= ol
SUN4_EXT	= ol
IBMRT_EXT	= ol
CONVEX_EXT	= ol
MMAX_EXT	= lo

# default for unknown machines
E = ol

# get the right value now, hoping for soft failure
E = ${$(TARGET_MACHINE)_EXT}



all: Makefile new.$Ez new.$Es

# Make correct_endian_worlds when bringing up a release distributed
# with only a bigendian world on a littleendian machine.

correct_endian_worlds: ../lib/oaklisp.$Ez ../lib/oaklisp.$Es



# How to compile oaklisp source files to make object files:
.SUFFIXES: .oa .oak

.oak.oa:
	oakliszt $*

# Get the compiler sources compiled in the correct locale:
$(COMPFILES) $(BATFILES): $$@k
	oakliszt $* compiler-locale

# How to build a new cold world using the world builder:
new.cold: $(COLDFILES) $(TOOLFILES)
	rm -f new.cold new.sym
	echo "(load 'tool)(tool-files '($(COLDFILESDNX)) 'new)"|oaklisp

# How to boot cold worlds into warm worlds:
.SUFFIXES: .$E .cold

.cold.$E:
	../etc/fliw $*.cold $*.$E

# How to load misc stuff into warm worlds:
.SUFFIXES: .$Em .$E

.$E.$Em:
	../etc/fliw $*.$E $*.$Em system-locale $(MISCFILES)

%.$Em       : %.$E       $(MISCFILES)
new.$Em     : new.$E     $(MISCFILES)

# How to load the compiler into misc worlds:
.SUFFIXES: .$Ec .$Em

.$Em.$Ec:
	../etc/fliw $*.$Em $*.$Ec compiler-locale $(COMPFILES)

%.$Ec       : %.$Em       $(COMPFILES)
new.$Ec     : new.$Em     $(COMPFILES)

# How to load the batch compiler into worlds with the compiler in 'em:
.SUFFIXES: .$Ez .$Ec

.$Ec.$Ez:
	../etc/fliw $*.$Ec $*.$Ez compiler-locale $(BATFILES)

%.$Ez       : %.$Ec       $(BATFILES)
new.$Ez     : new.$Ec     $(BATFILES)

# How to load the scheme compatibility package:
.SUFFIXES: .$Es .$Ec

.$Ec.$Es:
	../etc/fliw $*.$Ec $*.$Es system-locale scheme-upgrade

%.$Es       : %.$Ec       $(RNRSFILES)
new.$Es     : new.$Ec     $(RNRSFILES)
oaklisp.$Es : oaklisp.$Ec $(RNRSFILES)






# How to rebuild this makefile:

Makefile: $(MAKEFILES) $(FILESFILES)
	echo '(load "make-makefile")(make-makefile "Makefile")'|oaklisp


clean-oa-files:
	rm -f $(ALLOAFILES)

echosource:
	echo $(SOURCE)

# What goes in an official release:

release:
	copy -l $(SOURCE) $(GRAVY) ../$(TRELEASEDIR)

#end of long automatically generated Makefile.
