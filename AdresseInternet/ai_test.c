#include <stdio.h>
#include <stdlib.h>
#include "AdresseInternet.h"

int main(void) {
	AdresseInternet *addr = AdresseInternet_new("127.0.0.1", 6969);

	struct sockaddr saddr;

	AdresseInternet_to_sockaddr(addr, &saddr);

	AdresseInternet *addr2 = malloc(sizeof(*addr2));

	sockaddr_to_AdresseInternet(&saddr, addr2);

	char ip[24];

	AdresseInternet_getIP(addr2, ip, 24);

	printf("IP: %s\n", ip);

	AdresseInternet *addr3 = malloc(sizeof(*addr3));

	AdresseInternet_copy(addr3, addr2);

	AdresseInternet_getIP(addr3, ip, 24);

	printf("Addr 3 IP: %s\n", ip);


	return EXIT_SUCCESS;
}