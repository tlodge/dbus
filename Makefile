LDFLAGS+=-ldbus-1
LDFLAGS+=-ldbus-glib-1
CFLAGS+=-I/usr/include/dbus-1.0
CFLAGS+=-I/usr/include/glib-2.0
CFLAGS+=-I/usr/lib/glib-2.0/include/
CFLAGS+=-I/usr/lib/dbus-1.0/include/
CC=g++

CLIENTS = dbus sigsend usbsignaller

all: $(CLIENTS)

dbus: dbus.o
	$(CC) $(CFLAGS) $(LDFLAGS) dbus.o -o dbustest 	

sigsend: sigsend.o
	$(CC) $(CFLAGS) $(LDFLAGS) sigsend.o -o sigsend

usbsignaller: usbsignaller.o 
	$(CC) $(CFLAGS) $(LDFLAGS) usbsignaller.o  hashtable.c -o usbsignaller 

clean:
	rm -rf *.o dbus usbsignaller sigsend
