#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "AdresseInternet.h"

/** 
 * construit une adresse internet à partir d’un éventuel nom (sous
 * forme DNS ou IP) et d’un numéro de port. L’argument adresse est un
 * pointeur vers une variable de type AdresseInternet, qui est remplie
 * avec l’adresse internet construite et allouée. Valeur de retour :
 * en cas de succès, la fonction renvoie le pointeur. En cas d’erreur
 * la fonction renvoie NULL.
 */
AdresseInternet * AdresseInternet_new(const char* nom, uint16_t port) {
	AdresseInternet *addr = malloc(sizeof(*addr));
	if (addr == NULL) {
		return NULL;
	}

	memset(addr, 0, sizeof(*addr));

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_CANONNAME;

	struct addrinfo *result;

	char port_str[5];
	sprintf(port_str, "%"PRIu16, port);

	if (getaddrinfo(nom, port_str, &hints, &result) != 0) {
		free(addr);
		return NULL;
	}

  
	addr -> addrlen = result -> ai_addrlen;
  
  	memcpy(((struct sockaddr_storage *) &addr -> sockAddr), 
  			((struct sockaddr_storage *) result -> ai_addr), 
  			sizeof(result -> ai_addr));

	strncpy(addr -> nom, result -> ai_canonname, sizeof(addr -> nom));
	
	getnameinfo(result -> ai_addr, result -> ai_addrlen, NULL, 
					0,
					addr -> service,
					sizeof(addr -> service),
					0);
	
	freeaddrinfo(result);

	return addr;
}

/**
 * Idem, mais construit une adresse internet correspondant à toutes
 * les interfaces réseau à partir d’un numéro de port.
 */
AdresseInternet * AdresseInternet_any(uint16_t port) {
	
	AdresseInternet *addr = malloc(sizeof(*addr));	
	if (addr == NULL) {
		printf("addr null\n");
		return NULL;
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *result;

	char port_str[5];
	sprintf(port_str, "%"PRIu16, port);

	int err;
	if ((err = getaddrinfo(NULL, port_str, &hints, &result)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(err));
		free(addr);
		return NULL;
	}

  	memcpy(((struct sockaddr_storage *) &addr -> sockAddr), 
  			((struct sockaddr_storage *) result -> ai_addr), 
  			sizeof(result -> ai_addr));

	memset(&addr -> nom, 0, sizeof(addr -> nom));	


	getnameinfo(result -> ai_addr, result -> ai_addrlen, NULL, 
					0,
					addr -> service,
					sizeof(addr -> service),
					0);

	freeaddrinfo(result);

	return addr;
}
/**
 * Idem, mais construit une adresse internet correspondant à
 * l'interface loopback à partir d’un numéro de port.
 */
AdresseInternet * AdresseInternet_loopback(uint16_t port) {
	return AdresseInternet_new("127.0.0.1", port);
}
/** 
 * libère la mémoire allouée par AdresseInternet_free, any et loopback
 */
void AdresseInternet_free(AdresseInternet *adresse) {
	free(adresse);
}

/**
 * extrait d’une adresse internet l’adresse IP et le port
 * correspondants. L’argument adresse pointe vers un buffer 
 * contenant une adresse. L’argument nomDNS (resp. nomPort)
 * pointe vers un buffer alloué de taille au moins tailleDNS
 * (resp. taillePort) dans lequel la fonction va écrire une chaîne
 * de caractère (terminant par un 0) contenant le nom (resp. le port)
 * associé à l’adresse fournie. Lorsque cela est possible, la
 * résolution de nom est faite.  Si nomDNS ou nomPort est NULL,
 * l’extraction correspondante ne sera pas effectuée. Les deux ne
 * peuvent pas être NULL en même temps.  Valeur de retour : rend 0 en
 * cas de succès, et -1 en cas d’erreur.
 */
int AdresseInternet_getinfo(AdresseInternet *adresse,
			    char *nomDNS, int tailleDNS, char *nomPort, int taillePort) {

	if ((nomDNS == NULL && nomPort == NULL) || adresse == NULL) {
		return -1;
	} else {
		if (nomDNS != NULL) {
			if (strncpy(nomDNS, adresse -> nom, (size_t) tailleDNS) == NULL) {
				return -1;
			}
		}

		if (nomDNS != NULL) {
			if (strncpy(nomPort, adresse -> service, (size_t) taillePort) == NULL) {
				return -1;
			}
		}
	}

	return 0;
}

int AdresseInternet_getIP(const AdresseInternet *adresse, char *IP, int tailleIP) {
	if (adresse == NULL || IP == NULL) {
		return -1;
	}

	getnameinfo((struct sockaddr *) &(adresse -> sockAddr), adresse -> addrlen, 
					IP, 
					(socklen_t) tailleIP, 
					NULL, 
					0,
					NI_NUMERICHOST);
	return 0;
}

uint16_t AdresseInternet_getPort(const AdresseInternet *adresse) {
	if (adresse == NULL) {
		return 0;
	}

	uint16_t port;

	if (adresse -> sockAddr.ss_family == AF_INET) {
		port = ntohs(((struct sockaddr_in *) &adresse -> sockAddr) -> sin_port);
	} else if (adresse -> sockAddr.ss_family == AF_INET6) {
		port = ntohs(((struct sockaddr_in6 *) &adresse -> sockAddr) -> sin6_port);
	}

	return port;
}

int AdresseInternet_getDomain(const AdresseInternet *adresse) {
	if (adresse == NULL) {
		return -1;
	}

	return adresse -> sockAddr.ss_family;
}

/**
 * Construit une adresse internet à partir d'une structure sockaddr La
 * structure addresse doit être suffisamment grande pour pouvoir
 * accueillir l’adresse.  Valeur de retour : 0 en cas de succès, -1 en
 * cas d’échec.
 */
int sockaddr_to_AdresseInternet(const struct sockaddr *addr, AdresseInternet *adresse) {
	if (addr == NULL || adresse == NULL) {
		return -1;
	}

	//memcpy(&(adresse -> sockAddr), addr, sizeof(struct sockaddr_storage));

  	memcpy(((struct sockaddr_storage *) &adresse -> sockAddr), 
			((struct sockaddr_storage *) addr), 
			sizeof(struct sockaddr_storage));

	return 0;
}



/**
 * Construit une structure sockaddr à partir d'une adresse
 * internet. La structure addr doit être suffisamment grande pour
 * pouvoir accueillir l’adresse.  Valeur de retour : 0 en cas de
 * succès, -1 en cas d’échec.
 */
int AdresseInternet_to_sockaddr(const AdresseInternet *adresse, struct sockaddr *addr) {
	if (adresse == NULL || addr == NULL) {
		return -1;
	}

	memcpy(addr, ((struct sockaddr *) &(adresse -> sockAddr)), sizeof(struct sockaddr));

	return 0;
}


/**
 * compare deux adresse internet adresse1 et adresse2.  Valeur de
 * retour : rend 0 si les adresses sont différentes, 1 si elles sont
 * identiques (même IP et même port), et -1 en cas d’erreur.
 *
 */
int AdresseInternet_compare(const AdresseInternet *adresse1, const AdresseInternet *adresse2) {
	if (adresse1 == NULL || adresse2 == NULL) {
		return -1;
	}

	char ip1[_IP_MAX_SIZE];
	char ip2[_IP_MAX_SIZE];

	AdresseInternet_getIP(adresse1, ip1, sizeof(ip1));
	AdresseInternet_getIP(adresse2, ip2, sizeof(ip2));

	if (!strncmp(ip1, ip2, _IP_MAX_SIZE) && AdresseInternet_getPort(adresse1) == AdresseInternet_getPort(adresse2)) {
		return 1;
	}
	
	return 0;
}

/* copie une adresse internet dans une autre. Les variables doivent
   être allouées. */
int AdresseInternet_copy(AdresseInternet *adrdst, const AdresseInternet *adrsrc) {
	if (adrdst == NULL || adrsrc == NULL) {
		return -1;
	}

	memcpy(adrdst, adrsrc, sizeof(AdresseInternet));

	return 0;
}