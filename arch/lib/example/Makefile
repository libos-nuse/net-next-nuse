
#SIM_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(SIM_SRC))))

TARGET=udp-client
SRCS=udp-client.c udp-server.c tcp-client.c tcp-server.c
OBJS=udp-client.o udp-server.o tcp-client.o tcp-server.o
LIBLINUX=-L ../../../../ -lnuse-linux

CFLAGS=-g -std=c99 -ffreestanding
LDOPT= -Wl,-zlazy
#LDOPT+= -Wl,--unresolved-symbols=ignore-in-object-files
LDOPT+= -Wl,--unresolved-symbols=ignore-in-shared-libs
LDOPT+= -Wl,--warn-unresolved-symbols
LDFLAGS= -L . $(LDOPT) -nodefaultlibs -nostdlib
GLIBCDIR=/usr/lib64
STARTFILES = -nostartfiles `gcc --print-file-name=crt1.o` `gcc --print-file-name=crti.o` `gcc --print-file-name=crtbegin.o`
ENDFILES = `gcc --print-file-name=crtend.o` `gcc --print-file-name=crtn.o`
#LIBGROUP = -Wl,--start-group /usr/lib/debug/usr/lib64/libc.a -lgcc -lgcc_eh -Wl,--end-group
#LIBGROUP = -llinux
LIBGROUP = -lc -lpthread

all: $(TARGET)

# order is a matter: 1) liblinux.so, then 2) libc.so makes it.
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o udp-client-ld $(STARTFILES) udp-client.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o udp-client udp-client.o
	$(CC) $(LDFLAGS) -o udp-server-ld $(STARTFILES) udp-server.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o udp-server udp-server.o
	$(CC) $(LDFLAGS) -o tcp-client-ld $(STARTFILES) tcp-client.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o tcp-client tcp-client.o
	$(CC) $(LDFLAGS) -o tcp-server-ld $(STARTFILES) tcp-server.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o tcp-server tcp-server.o

$(OBJS): Makefile $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS)

clean:
	rm -f $(TARGET)
	rm -f *.o
	rm -f *-ld
