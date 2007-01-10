# mingw makefile for wiimote-api

DEFS = -D WINVER=0x0501


all: libwiimote-api.a

libwiimote-api.a: wiimote-api.o
	ar rcs libwiimote-api.a wiimote-api.o 

wiimote-api.o:
	gcc $(DEFS) -c wiimote-api.c

clean:
	del libwiimote-api.a wiimote-api.o

