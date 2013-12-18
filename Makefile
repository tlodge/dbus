LDFLAGS+=-ldbus-1
LDFLAGS+=-ldbus-glib-1
CFLAGS+=-I/usr/include/dbus-1.0
CFLAGS+=-I/usr/include/glib-2.0
CFLAGS+=-I/usr/lib/glib-2.0/include/
CFLAGS+=-I/usr/lib/dbus-1.0/include/
CC=gcc

dbus: dbus.o
	$(CC) $(CFLAGS) $(LDFLAGS) dbus.o -o dhtest 	

clean:
	rm -rf *.o dbus
