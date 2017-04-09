# export LD_LIBRARY_PATH=libx86-64:$LD_LIBRARY_PATH
CC = gcc
CFLAGS = -Wall -Wextra -Wconversion -Werror -pedantic -std=c11 -fPIC -D_XOPEN_SOURCE=700
LDLIBS = -lAdresseInternet -lSockUDP -lTFTP
LDFLAGS = -L./libx86-64 -L. -pthread
PRGS = libSockUDP.so libAdresseInternet.so libTFTP.so server client

all: $(PRGS)

libAdresseInternet.so: AdresseInternet/AdresseInternet.o
	$(CC) -shared $(CFLAGS) $^ -o $@

libSockUDP.so: libAdresseInternet.so SockUDP/SocketUDP.o
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) -lAdresseInternet 

libTFTP.so: TFTP/TFTP.o libSockUDP.so libAdresseInternet.so
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS) -lAdresseInternet -lSockUDP

AdresseInternet/AdresseInternet.o: AdresseInternet/AdresseInternet.c AdresseInternet/AdresseInternet.h \
	AdresseInternet/AdresseInternetType.h

SockUDP/SockUDP.o: SockUDP/SocketUDP.c SockUDP/SocketUDP.h \
 SockUDP/SocketUDPType.h AdresseInternet/AdresseInternetType.h

utils/utils.o: utils/utils.c utils/utils.h

server: server.o

client: client.o utils/utils.o

install: libAdresseInternet.so libSockUDP.so libTFTP.so
	mv libAdresseInternet.so  libx86-64
	mv libSockUDP.so libx86-64
	mv libTFTP.so libx86-64

clean:
	$(RM) -r *~ *.o $(PRGS) *.aux *.log *.synctex.gz auto 
	$(RM) AdresseInternet/*.o
	$(RM) SockUDP/*.o

dist-clean: clean
	$(RM) -rv libx86-64/* $(PRGS)

