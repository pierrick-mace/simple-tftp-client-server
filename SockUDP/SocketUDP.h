#ifndef __SOCKET_UDP_H__
#define __SOCKET_UDP_H__

#include "SocketUDPType.h"

int initSocketUDP(SocketUDP *psocket);

int attacherSocketUDP(SocketUDP *sock, const char *address, uint16_t port, int flags);

int estAttachee(SocketUDP *socket);

int getLocalName(SocketUDP *socket, char *buffer, int taille);

int getLocalIP(const SocketUDP *socket, char *localIP, int tailleIP);

uint16_t getLocalPort(const SocketUDP *socket);

ssize_t writeToSocketUDP(SocketUDP *sock, const AdresseInternet *address, const char *buffer, int length);

ssize_t recvFromSocketUDP(SocketUDP *socket, char *buffer, int length, AdresseInternet *address, int timeout);

int closeSocketUDP(SocketUDP *socket);

#endif