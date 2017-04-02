#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../AdresseInternet/AdresseInternet.h"
#include "../SockUDP/SocketUDP.h"
#include "TFTP.h"
 
// Créé un paquet de type ACK
int tftp_make_ack(char *buffer, size_t *length, uint16_t block) {
	if (buffer == NULL || length == NULL) {
		return -1;
	}

	uint16_t *p = (uint16_t *) buffer;
	*p = htons(ACK);
	*(p+1) = htons(block);
	*length = 4;
	
    return 0;
}

// Créé un paquet de type RRQ contenant le nom du fichier 
int tftp_make_rrq(char *buffer, size_t *length, const char *fichier) {
	if (buffer == NULL || length == NULL || fichier == NULL) {
		return -1;
	}

	uint16_t *p = (uint16_t *) buffer;
	*p = htons(RRQ);
	strcpy(buffer + 2, fichier);
	strcpy(buffer + strlen(fichier) + 3, "octet");
	*length = 4 + strlen(fichier) + strlen("octet");
	
    return 0;
}

// Créé un paquet de type DATA contenant le numéro de block, les données ainsi que sa taille
int tftp_make_data(char *buffer, size_t *length, uint16_t block, const char *data, size_t n) {
	if (buffer == NULL || length == NULL || data == NULL) {
		return -1;
	}

	uint16_t *p = (uint16_t *) buffer;
	*p = htons(DATA);
	*(p+1) = htons(block);
	memcpy(buffer + 4, data, n);
	*length = 4 + n;	
	
    return 0;
}

// Créé un paquet de type ERROR
int tftp_make_error(char *buffer, size_t *length, uint16_t errorcode, const char *message) {
	if (buffer == NULL || length == NULL || message == NULL) {
		return -1;
	}

	uint16_t *p = (uint16_t *) buffer;
	*p = htons(ERROR);
	*(p+1) = htons(errorcode);
	strcpy(buffer + 4, message);
	*length = 4 + strlen(message) + 1;
	
    return 0;
}

// Fonction d'envoi du paquet d'erreur
void tftp_send_error(SocketUDP *socket, const AdresseInternet *dst, uint16_t code, const char *msg) {
	if (socket == NULL || dst == NULL || msg == NULL) {
        return;
    }

	char buffer[5 + 512];
	size_t length = 5 + 512;
	tftp_make_error(buffer, &length, code, msg);
	writeToSocketUDP(socket, dst, buffer, (int) strlen(buffer)); 
}

// Envoie un paquet RRQ et attend le début de la récéption des données, si le délai d'attente est dépassé la fonction renvoie 0
// En cas de succès la fonction renvoie 1, sinon elle renvoie -1
int tftp_send_RRQ_wait_DATA_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *fichier, AdresseInternet *connexion, char *reponse, size_t *replength){
	if (socket == NULL || dst == NULL || fichier == NULL || connexion == NULL) {
        return -1;
    }

	char buffer[1024];
	size_t taille = 1024;

	if (tftp_make_rrq(buffer, &taille, fichier) == -1) {
		return -1;
	}

	if (writeToSocketUDP(socket, dst, buffer, (int) taille) <= 0) {
		return -1;
	}

	int retour; 
    
	if ((retour = (int) recvFromSocketUDP(socket, reponse, (int) *replength, connexion, TIMEOUT_MAX)) == -1) {
		return -1;
	}

    // Si le délai d'attente est dépassé
	if (retour == 0 ) {
		return 0;
	}

	uint16_t *p = (uint16_t *) buffer;

	if (*p == htons(DATA)) {
        *replength = (size_t) retour;
    } else {
        tftp_send_error(socket, dst, (uint16_t) 4, "Mauvais type de paquet");
        return -1;
    }

	return 1;
}

int tftp_send_RRQ_wait_DATA(SocketUDP *socket, const AdresseInternet *dst, const char *fichier, AdresseInternet *connexion, char *reponse, size_t *replength) {
	if (socket == NULL || dst == NULL || fichier == NULL || connexion == NULL) {
        return -1;
    }

	int retour;
	for (int i = 0; i < NB_MAX_ENVOI; i++){
		if ((retour = tftp_send_RRQ_wait_DATA_with_timeout(socket, dst, fichier, connexion, reponse, replength)) == -1) {
			return -1;
		}

		if (retour == 1) {
			return 1;
		}
	}

	return 0;
}

int tftp_send_DATA_wait_ACK_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen) {
	if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }

    size_t bloc;
    char data[512];

    if (donneesDATA(paquet, &bloc, data) == -1) {
		return -1;
	}

    if (writeToSocketUDP(socket, dst, paquet, (int) paquetlen) == -1){
		return -1;
	}

	char buffer[1024];
	int taille = 1024;	
	int retour; 

	if ((retour = (int) recvFromSocketUDP(socket, buffer, taille, NULL, TIMEOUT_MAX)) == -1) {
		return -1;
	}

	size_t block;
	if (donneesACK(buffer, &block) == -1) {
		return -1;
	}

	if (retour == 0 || block < bloc) {
		return 0;
	}

	uint16_t *p = (uint16_t *) buffer;

	if (*p != htons(ACK) || block > bloc) {
		tftp_send_error(socket, dst, (uint16_t)4, "Erreur");
		return -1;
	}	

	return 1;
}

int tftp_send_DATA_wait_ACK(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen) {
	if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }

    int retour;
    for (int i = 0; i < NB_MAX_ENVOI; i++) {
		if ((retour = tftp_send_DATA_wait_ACK_with_timeout(socket, dst, paquet, paquetlen)) == -1) {
			return -1;
		}

		if (retour == 1) {
			return 1;
		}
	}
    return 0;
}

int tftp_send_ACK_wait_DATA_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen, char *retour, size_t *taille) {
    if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }

	if (writeToSocketUDP(socket, dst, paquet, (int) paquetlen) == -1) {
		return -1;
	}

    AdresseInternet *addr = malloc(sizeof(*addr));
    if (addr == NULL) {
        perror("malloc");
        return -1;
    }

	int rep; 
	if ((rep = (int) recvFromSocketUDP(socket, retour, (int) *taille, addr, TIMEOUT_MAX)) == -1) {
        free(addr);
		return -1;
	}

    free(addr);

	if (rep == 0){
		return 0;
	}

	uint16_t *p = (uint16_t *) retour;

	if (*p != htons(DATA)) {
		tftp_send_error(socket, dst, (uint16_t)4, "Erreur");
		return -1;
	} else {
		*taille = (size_t) rep;
	}

	return 1;	
}

int tftp_send_ACK_wait_DATA(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen, char *retour, size_t *taille) {
	if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }

    int rep = 0;
    for (int i = 0; i < NB_MAX_ENVOI; i++) {
		if ((rep = tftp_send_ACK_wait_DATA_with_timeout(socket, dst, paquet, paquetlen, retour, taille)) == -1) {
			return -1;
		}

		if (rep == 1){
			return 1;
		}
    }

    return 0;
}  

int tftp_send_last_ACK(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen) {
    if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }

	if (writeToSocketUDP(socket, dst, paquet, (int) paquetlen) == -1) {
		return -1;
	}

    return 0;
}

int donneesRRQ(const char *paquet, char *fichier, char *mode) {
	if (paquet == NULL || fichier == NULL || mode == NULL) {
        return -1;
    }

    uint16_t *p = (uint16_t *)paquet; 

    if (ntohs(*p) != RRQ) {
        return -1;
    }

    // Copie du nom du fichier
    memcpy(fichier, paquet + 2, 512);
    // Copie du mode de transfert
    memcpy(mode, paquet + 2 + strlen(paquet + 2) + 1, 512);
	return 0;
}

int donneesDATA(const char *paquet, size_t *bloc, char *data) {
    if (paquet == NULL || bloc == NULL || data == NULL) {
        return -1;
    }

    uint16_t *p = (uint16_t *)paquet;
    
    if (ntohs(*p) != DATA) {
        printf("Not data packet\nPacket is type: %"PRIu16 "\n", ntohs(*p));
        return -1;
    }

    *bloc = ntohs(*(p + 1));
    memcpy(data, paquet + 4, 512);
    
    return 0;
}

int donneesACK(const char *paquet, size_t *bloc) {
    if (paquet == NULL || bloc == NULL) {
        return -1;
    }

    uint16_t *p = (uint16_t *)paquet;
    
    if (ntohs(*p) != ACK) { 
        return -1;
    }
    
    *bloc = ntohs(*(p + 1));
    
    return 0;
}