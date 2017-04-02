#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "SocketUDP.h"
#include "../AdresseInternet/AdresseInternet.h"

// Initialise la socket psocket préalablement allouée
int initSocketUDP(SocketUDP *psocket) {
	if (psocket == NULL) {
		return -1;
	}

	psocket -> socket = 0;
	psocket -> is_bounded = 0;
	psocket -> addrlocal = NULL;

	return 0;
}

// Attache la socket sock à l'adresse address et au port indiqué, en prenant en compte les options de flags
int attacherSocketUDP(SocketUDP *sock, const char *address, uint16_t port, int flags) {
	if (sock == NULL) {
		return -1;
	}

	if (address != NULL) {
		sock -> addrlocal = AdresseInternet_new(address, port);
	} else {
		if (flags == 0) {
			sock -> addrlocal = AdresseInternet_any(port);
		} else if (flags == LOOPBACK) {
			sock -> addrlocal = AdresseInternet_loopback(port);
		}
	}

	if (sock -> addrlocal == NULL) {
		return -1;
	}

	if ((sock -> socket = socket(sock -> addrlocal -> sockAddr.ss_family, SOCK_DGRAM, 0)) == -1) {
		AdresseInternet_free(sock -> addrlocal);
		return -1;
	}

	struct sockaddr addr;

	if (AdresseInternet_to_sockaddr(sock -> addrlocal, &addr) == -1) {
		AdresseInternet_free(sock -> addrlocal);
		return -1;
	}

	if (bind(sock -> socket, &addr, sock -> addrlocal -> addrlen) == -1) {
		AdresseInternet_free(sock -> addrlocal);
		return -1;
	}

	sock -> is_bounded = 1;

	return 0;
}

// Renvoie 0 si la socket a une adresse locale, -1 sinon
int estAttachee(SocketUDP *socket) {
	if (socket == NULL) {
		return -1;
	}

	if (socket -> is_bounded) {
		return 0;
	}

	return -1;
}

// Remplie buffer avec le nom local de la socket. 
// Si le buffer est trop petit, le nom est tronqué (et le 0 final n’est pas écrit). Si besoin, socket est mis à jour.
// Renvoie le nombre d’octets écrits dans buffer, ou -1 en cas d’erreur.
int getLocalName(SocketUDP *socket, char *buffer, int taille) {
	if (socket == NULL || buffer == NULL) {
		return -1;
	}

	if (AdresseInternet_getinfo(socket -> addrlocal, buffer, taille, NULL, 0) == -1) {
		return -1;
	}

	return (int) strlen(buffer);
}

// Remplie localIP avec l’IP locale de la SocketUDP. 
// Si le buffer est trop petit, le nom est tronqué (et le 0 final n’est pas écrit).
// Renvoie le nombre d’octets écrits dans buffer, ou -1 en cas d’erreur.
int getLocalIP(const SocketUDP *socket, char *localIP, int tailleIP) {
	if (socket == NULL || localIP == NULL) {
		return -1;
	}

	if (AdresseInternet_getIP(socket -> addrlocal, localIP, tailleIP) == -1) {
		return -1;
	}

	return (int) strlen(localIP);
}

// Renvoie le port de la socket
uint16_t getLocalPort(const SocketUDP *socket) {
	if (socket == NULL) {
		return 0;
	}

	uint16_t port = 0;

	if (socket -> addrlocal -> sockAddr.ss_family == AF_INET) {
		struct sockaddr_in addr;

		memcpy(&addr, &(socket -> addrlocal -> sockAddr), sizeof(struct sockaddr_in));
		
		port = ntohs(addr.sin_port);

	} else if (socket -> addrlocal -> sockAddr.ss_family == AF_INET6) {
		struct sockaddr_in6 addr;

		memcpy(&addr, &(socket -> addrlocal -> sockAddr), sizeof(struct sockaddr_in6));

		port = ntohs(addr.sin6_port);
	}

	return port;
}

// Ecrit sur la socket sock vers adresse un bloc d’octets buffer de taille length et retourne la
// taille des données écrites ou -1 s’il y a erreur.
ssize_t writeToSocketUDP(SocketUDP *sock, const AdresseInternet *address, const char *buffer, int length) {
	if (sock == NULL || address == NULL || buffer == NULL) {
		return -1;
	}

	struct sockaddr dest_addr;

	if (AdresseInternet_to_sockaddr(address, &dest_addr) == -1) {
		return -1;
	}

	socklen_t addrlen = sizeof(dest_addr);

	return sendto(sock -> socket, buffer, (size_t) length, 0, &dest_addr, addrlen);
}

// Lit sur socket les données envoyées par une machine d’adresse adresse. 
// La fonction est bloquante pendant timeout secondes. 
// Elle lit et place dans buffer un bloc d’octets de taille au plus length. 
// Renvoie la taille des données lues ou -1 s’il y a erreur.
ssize_t recvFromSocketUDP(SocketUDP *socket, char *buffer, int length, AdresseInternet *address, int timeout) {
	if (socket == NULL || buffer == NULL) {
		return -1;
	}

	struct timeval tval;

	tval.tv_sec = timeout;
	tval.tv_usec = 0;

	if (setsockopt(socket -> socket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tval, sizeof(struct timeval)) == -1) {
		return -1;
	}

	ssize_t len;

	if (address != NULL) {
		socklen_t socklen = sizeof(address -> sockAddr);

		if ((len = recvfrom(socket -> socket, buffer, (size_t) length, 0, (struct sockaddr *) &(address -> sockAddr), &socklen)) == -1) {
			return -1;
		}
	} else {
		if ((len = recvfrom(socket -> socket, buffer, (size_t) length, 0, NULL, NULL)) == -1) {
			return -1;
		}
	}

	return len;
}

// Ferme la connexion et libère la SocketUDP.
// Renvoie 0 en cas de succès, -1 en cas d’erreur.
int closeSocketUDP(SocketUDP *socket) {
	if (socket == NULL) {
		return -1;
	}

	AdresseInternet_free(socket -> addrlocal);

	close(socket -> socket);

	socket -> is_bounded = 0;

	free(socket);

	return 0;
}