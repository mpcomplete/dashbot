LDFLAGS=-lX11 -lpng -pg
CCFLAGS=-O2 -ggdb3
#CCFLAGS=-ggdb3
OFILES=main.o readpng.o xutil.o

all: dashbot

run: all
	./dashbot
.cc.o: %.cc
	g++ -c $< $(CCFLAGS)

dashbot: $(OFILES)
	g++ -o $@ $(OFILES) $(LDFLAGS)

clean:
	rm dashbot $(OFILES)
