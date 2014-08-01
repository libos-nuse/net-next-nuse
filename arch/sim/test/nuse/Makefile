
#SIM_OBJ=$(addprefix $(ARCH_DIR)/,$(addsuffix .o,$(basename $(SIM_SRC))))

TARGET=udp-client
SRCS=udp-client.c udp-server.c tcp-client.c tcp-server.c
OBJS=udp-client.o udp-server.o tcp-client.o tcp-server.o
LIBLINUX=-L ../../../../ -llinux

CFLAGS=-g -std=c99 -ffreestanding
LDOPT= -Wl,-zlazy
#LDOPT+= -Wl,--unresolved-symbols=ignore-in-object-files
LDOPT+= -Wl,--unresolved-symbols=ignore-in-shared-libs
LDOPT+= -Wl,--warn-unresolved-symbols
LDFLAGS= -L . $(LDOPT) -nodefaultlibs -nostdlib
GLIBCDIR=/usr/lib64
STARTFILES = -nostartfiles $(GLIBCDIR)/crt1.o $(GLIBCDIR)/crti.o `gcc --print-file-name=crtbegin.o`
ENDFILES = `gcc --print-file-name=crtend.o` $(GLIBCDIR)/crtn.o
#LIBGROUP = -Wl,--start-group /usr/lib/debug/usr/lib64/libc.a -lgcc -lgcc_eh -Wl,--end-group
#LIBGROUP = -llinux
LIBGROUP = -lc -lpthread

all: $(TARGET)

# order is a matter: 1) liblinux.so, then 2) libc.so makes it.
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o udp-client-ld $(STARTFILES) udp-client.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o udp-client udp-client.o
        #sudo chown root udp-client
        #sudo chmod 4755 udp-client
	$(CC) $(LDFLAGS) -o udp-server-ld $(STARTFILES) udp-server.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o udp-server udp-server.o
	#sudo chown root udp-server
	#sudo chmod 4755 udp-server
	$(CC) $(LDFLAGS) -o tcp-client-ld $(STARTFILES) tcp-client.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o tcp-client tcp-client.o
	#sudo chown root tcp-client
	#sudo chmod 4755 tcp-client
	$(CC) $(LDFLAGS) -o tcp-server-ld $(STARTFILES) tcp-server.o $(LIBLINUX) $(LIBGROUP) $(ENDFILES)
	$(CC) -o tcp-server tcp-server.o
	#sudo chown root tcp-server
	#sudo chmod 4755 tcp-server

$(OBJS): Makefile $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS)

clean:
	rm -f $(TARGET)
	rm -f *.o
