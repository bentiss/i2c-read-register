CC=gcc
ODIR=obj
CFLAGS=-W -Wall
LIBS=-lm

_DEPS =
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = i2c-read-register.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

i2c-read-register: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core


#all: i2c-read-register
#
#IDIR =../include
#CC=gcc
#CFLAGS=-I$(IDIR)
#
#ODIR=obj
#LDIR =../lib
#
#LIBS=-lm
#
#_DEPS = hellomake.h
#DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
#
#_OBJ = hellomake.o hellofunc.o
#OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
#
#
#$(ODIR)/%.o: %.c $(DEPS)
#	$(CC) -c -o $@ $< $(CFLAGS)
#
#hellomake: $(OBJ)
#	gcc -o $@ $^ $(CFLAGS) $(LIBS)
#
#.PHONY: clean
#
#clean:
#	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
