CC = gcc
CFLAGS = -g

COM = commons.o


OSSO = oss.o $(COM)
OSS = oss

UPO = UserProc.o $(COM)
UP = UserProc

.SUFFIXES: .c .o

all: $(OSS) $(UP)

$(OSS): $(OSSO)
	$(CC) -o $@ $(OSSO)

$(UP): $(UPO)
	$(CC) -o $@ $(UPO)

.c.o:
	$(CC) -c -o $@ $<

clean:
	rm *.o $(UP) $(OSS)

pclean:
	rm *.o
