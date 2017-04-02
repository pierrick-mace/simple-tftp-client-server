#ifndef __SOCKET_UDP_TYPE_H__
#define __SOCKET_UDP_TYPE_H__

#include <inttypes.h>
#include "../AdresseInternet/AdresseInternetType.h"

typedef struct {
	int socket;
	AdresseInternet *addrlocal;
	int is_bounded;
} SocketUDP;

enum {
	LOOPBACK = 1
};

#endif