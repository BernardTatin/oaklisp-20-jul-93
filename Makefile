CC = cc
LD = ld

all: install

install:
	cd src/src; $(MAKE) emulator
	cd mac; $(MAKE)

SOURCE = Makefile COPYING

clean:
	@echo "clean occurs..."
	cd src/src; $(MAKE) clean

release:
	copy $(SOURCE) $(TRELEASEDIR)
