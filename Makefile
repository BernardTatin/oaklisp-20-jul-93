all: install

install:
	cd src/fast; $(MAKE) emulator

SOURCE = Makefile COPYING

release:
	copy $(SOURCE) $(TRELEASEDIR)
