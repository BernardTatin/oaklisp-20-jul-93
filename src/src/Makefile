# NOTE: If you are not using a fancy make that looks at Makeconf,
# uncomment one the the following.

# Enable all the speed hacks:
DEFINES = -DFAST
CFLAGS = -O

# Enable some hopefully unnecessary checks:
# DEFINES =
# CFLAGS = -g
# EXTRAS = instrs.o

# Disable the method cache:
# DEFINES = -DFAST -DNO_METH_CACHE
# CFLAGS = -O

#
# LIBS = $$LDPATH -lmisc -lmalloc -lc_s
#
CC = cc
LD = ld


all: emulator

OBJF1 = emulator.o mymalloc.o worldio.o gc.o stacks.o weak.o 
OBJF2 = signal.o $(EXTRAS) 
OBJFILES = $(OBJF1) $(OBJF2)
INCLUDES = emulator.h config.h stacks.h gc.h
MISC = Makefile instruction-table.oak
GRAVY = instrs.c Proto.h

CFILES = $(OBJFILES:.o=.c)

Proto.h: $(CFILES) $(INCLUDES)
	/usr/misc/.case/bin/mkptypes -e -A -x $(CFILES) > Proto.h

signal.o: config.h
$(OBJF1): config.h emulator.h Proto.h
emulator.o gc.o stacks.o: stacks.h
gc.o weak.o: gc.h

.c.o: ; $(CC) $(CFLAGS) $(DEFINES) -c $*.c

emulator: $(OBJFILES)
	$(CC) $(CFLAGS) $(LDCFLAGS) -o $@ $(OBJFILES)

clean:
	$(RM) $(OBJFILES) emulator

instrs.c: instruction-table.oak ../../mac/assembler.oak
	echo '(set (fluid current-locale) compiler-locale)	\
	      (load "instruction-table.oak")			\
	      (dump-instruction-table "instrs.c")'		\
	| oaklisp

.linted: $(CFILES) $(INCLUDES)
	lint -b $(DEFINES) $(CFILES)
	touch .linted

SOURCE = $(CFILES) $(INCLUDES) $(MISC) $(GRAVY)

release: $(SOURCE)
	copy ../fast/makefile			../../$(RELEASEDIR)/src/fast
	copy ../safe/makefile			../../$(RELEASEDIR)/src/safe
	copy ../Makeconf			../../$(RELEASEDIR)/src
	copy $(SOURCE)				../../$(RELEASEDIR)/src/src
