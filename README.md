# note
Try to compile oaklisp on Solaris 11

# original _README_
This directory contains a release of the Oaklisp system.

Oaklisp is (C) Copyright 1986,87,88,89,90,91,92 Kevin Lang and Barak
Pearlmutter.  It is released under the GNU Public License 2.0; see the
file COPYING, enclosed, for details.

If you wish to use Oaklisp under different terms, please contact the
authors.  We are reasonable people.


Enhancements, bug reports, and bug fixes should be mailed to
Barak.Pearlmutter@CS.CMU.EDU for those with access to email, or

	Barak Pearlmutter
	Yale University
	Department of Psychology
	11A Yale Station
	New Haven, CT  06520-7447


Some of the Makefiles in this collection use CMU CS specific features.
If you can't figure out what's going on, try just ignoring and
deleting the weird stuff.

An Oaklisp Language Manual is available as a CMU CS technical report,
or online by anonymous ftp from CS.CMU.EDU
/afs/cs.cmu.edu/user/bap/ftpable/oaklisp/



The contents of this directory are as follows:

Makefile; type "make install" to get things set up.

README	this file

COPYING	GNU GPL 2.0, the license under which this software is released

bin/	contains some shell scripts.  Either put this directory in
	your search path or make links from some directory on your
	search path to the scripts oaklisp, oakliszt, and scheme.

	The command to run Oaklisp is "oaklisp".  The command to run
	Oaklisp with an R3RS compatibility package loaded is "scheme".
	The command to compile an Oaklisp source file is "oakliszt".
	For instance, to compile foo.oak, producing foo.oa, one types
	"oakliszt foo".

	To use these scripts, you should setenv OAKPATH to the
	directory this file is in, with no trailing slash, or change
	the default location by editing them.

etc/	contains a symbolic link to the bytecode emulator executable,
	"emulator", which lives down in the src/ subdirectory.

src/	contains C source to make the bytecode emulator.  If Oaklisp
	won't boot, you may need to edit the file config.h to reflect
	your machine, although it tries to adapt itself to various
	environments.  In particular, make sure it gets BIG_ENDIAN and
	UNSIGNED_CHARS set correctly.

	If you have a fancy enough make, just cd to fast/ and "make
	emulator".  If you have a System-V style make that recognizes
	VPATH, add the line "VPATH = ../src" to the beginning of
	fast/makefile, cd to fast/ and "make emulator".  If you have a
	lowest common denominator make, cd to fast/ and type "cp
	../src/{Makefile,*.[ch]} ." and then "make emulator".

lib/	contains Oaklisp bootable worlds.  The only crucial one is
	"oaklisp.??c" which is the normal Oaklisp world.
	"oaklisp.??z" is the batch compiler world, necessary for the
	"oakliszt" command.  The "??" in these filenames is either
	"ol" or "lo" for big endian and little endian machines,
	respectively.  Of course, you only really need the ones for
	the endianity of your machine.

	It is possible to rebuild these from oaklisp.cold, or from the
	files in the mac/ director using tool.oak.

mac/	**OPTIONAL**

	contains all the Oaklisp source files needed to build a world,
	including the compiler, the scheme compatibility package, and
	the world builder tool.  The latter, tool.oak, is used for
	building new cold world loads, necessary only when extremely
	low level changes are made to the system.  Even then, it is
	typically possible to just patch a warm world instead of
	building a new cold one.

