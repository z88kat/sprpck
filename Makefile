# Lynx sprite packer
# Makefile for *nix

CC = gcc
RM = rm -f
EXT = #.exe

CFLAGS = -O3 -fomit-frame-pointer -DUNIX -Wall

C_SRC= io.c sprpck.c bin2obj.c

H_SRC= sprpck.h bmp.h

sprpck$(EXT):   io.o sprpck.o sprpck.h
	$(CC) -o $@ sprpck.o io.o
	
io.o : io.c bmp.h
clean:
	$(RM) *.o
	$(RM) core
	$(RM) sprpck$(EXT)

zip:
	zip -u sprpck $(C_SRC) $(H_SRC) makefile
	zip -u sprpckdos sprpck.exe

 
