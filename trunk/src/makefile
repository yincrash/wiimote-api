# mingw makefile for wiimote-api

DEFS = -D WINVER=0x0501
BIN  = ..\bin\mingw\


all: $(BIN)libwiimote-api.a
	copy wiimote-api.h ..\bin\

$(BIN)libwiimote-api.a: wiimote-api.o
	ar rcs $(BIN)libwiimote-api.a wiimote-api.o
	del wiimote-api.o

wiimote-api.o: wiimote-api.c
	gcc $(DEFS) -c wiimote-api.c

clean:
	del $(BIN)libwiimote-api.a

