#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdint.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h> 
#include "../AdresseInternet/AdresseInternet.h"
#include "../SockUDP/SocketUDP.h"
#include "TFTP.h"
 
int tftp_make_ack(char *buffer, size_t *length, uint16_t block){
	// Crée un paquet du type 4 1 
	if (buffer == NULL || length == NULL){
		return -1;
	}
	uint16_t *p = (uint16_t *) buffer;
	*p = htons(4);
	*(p+1) = htons(block);
	*length = 4;
	return 0;
}

int tftp_make_rrq(char *buffer, size_t *length, const char *fichier){
	// Crée un paquet du type 1 toto.txt octet 
	if (buffer == NULL || length == NULL || fichier == NULL){
		return -1;
	}
	uint16_t *p = (uint16_t *) buffer;
	*p = htons(1);
	strcpy(buffer + 2, fichier);
	strcpy(buffer + strlen(fichier) + 3, "octet");
	*length = 4 + strlen(fichier) + strlen("octet");
	return 0;
}

int tftp_make_data(char *buffer, size_t *length, uint16_t block, const char *data, size_t n){
	// Crée un paquet du type 3 1 
	if (buffer == NULL || length == NULL || data == NULL){
		return -1;
	}
	uint16_t *p = (uint16_t *) buffer;
	*p = htons(3);
	*(p+1) = htons(block);
	memcpy(buffer + 4, data, n);
	*length = 4 + n;	
	return 0;
}

int tftp_make_error(char *buffer, size_t *length, uint16_t errorcode, const char *message){
	// Crée un paquet du type 5 0 
	if (buffer == NULL || length == NULL || message == NULL){
		return -1;
	}
	uint16_t *p = (uint16_t *) buffer;
	*p = htons(5);
	*(p+1) = htons(errorcode);
	strcpy(buffer + 4, message);
	*length = 4 + strlen(message) + 1;
	return 0;
}

void tftp_send_error(SocketUDP *socket, const AdresseInternet *dst, uint16_t code, const char *msg){
	if (socket == NULL || dst == NULL || msg == NULL) {
        return;
    }
	char buffer[5 + 512];
	size_t length = 5 + 512;
	tftp_make_error(buffer, &length, code, msg);
	writeToSocketUDP(socket, dst, buffer, (int) strlen(buffer)); 
}

int tftp_send_RRQ_wait_DATA_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *fichier, AdresseInternet *connexion, char *reponse, size_t *replength){
	if (socket == NULL || dst == NULL || fichier == NULL || connexion == NULL) {
        return -1;
    }
	char buffer[1024];
	size_t taille = 1024;
	if (tftp_make_rrq(buffer, &taille, fichier) == -1){ // On crée le paquet rrq
		return -1;
	}
	if (writeToSocketUDP(socket, dst, buffer, (int) taille) <= 0){ // On l'envoie si ce n'est pas vide
		return -1;
	}

	int retour; 
    
	if ((retour = (int) recvFromSocketUDP(socket, reponse, (int) *replength, connexion, 10)) == -1){ // On recupère la réponse
		return -1;
	}
	if (retour == 0){ // Si le timeout est dépassé, on retourne 0
		return 0;
	}
	uint16_t *p = (uint16_t *) buffer;
	if (*p == htons(3)){
        *replength = (size_t) retour;
    } 
    else{
        tftp_send_error(socket, dst, (uint16_t) 4, "Mauvais type de paquet");
        return -1;
    }
	return 1;
}

int tftp_send_RRQ_wait_DATA(SocketUDP *socket, const AdresseInternet *dst, const char *fichier, AdresseInternet *connexion, char *reponse, size_t *replength){
	if (socket == NULL || dst == NULL || fichier == NULL || connexion == NULL) {
        return -1;
    }

	int retour;
	for (int i = 0; i < 10; i++){ // Maximum 10 envois
		if ((retour = tftp_send_RRQ_wait_DATA_with_timeout(socket, dst, fichier, connexion, reponse, replength)) == -1){
			return -1;
		};
		if (retour == 1){
			return 1;
		};
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
	if ((retour = (int) recvFromSocketUDP(socket, buffer, taille, NULL, 10)) == -1) {
		return -1;
	}

	size_t block;
	if (donneesACK(buffer, &block) == -1){
		return -1;
	}
	if (retour == 0 || block < bloc){ // Si le numéro ACK est inférieur au numéro DATA, on réessaie
		return 0;
	}
	uint16_t *p = (uint16_t *) buffer;
	if (*p != htons(4) || block > bloc) { // Si le numéro ACK est supérieur au numéro DATA, on réessaie
		tftp_send_error(socket, dst, (uint16_t)4, "Erreur");
		return -1;
	}	
	return 1;
}

int tftp_send_DATA_wait_ACK(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen){
	// On utilise la même méthode que précédemment : on boucle une fonction avec timeout
	if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }

    int retour;
    for (int i = 0; i < 10; i++){ // Maximum 10 envois
		if ((retour = tftp_send_DATA_wait_ACK_with_timeout(socket, dst, paquet, paquetlen)) == -1){
			return -1;
		}
		if (retour == 1){
			return 1;
		}
	}
    return 0;
}

int tftp_send_ACK_wait_DATA_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen, char *retour, size_t *taille){
    if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }
	if (writeToSocketUDP(socket, dst, paquet, (int) paquetlen) == -1){
		return -1;
	}

    AdresseInternet *addr = malloc(sizeof(*addr));
    if (addr == NULL) {
        perror("malloc");
        return -1;
    }

	int rep; 
	if ((rep = (int) recvFromSocketUDP(socket, retour, (int) *taille, addr, 10)) == -1) {
        free(addr);
		return -1;
	}

    free(addr);

	if (rep == 0){
		return 0;
	}
	uint16_t *p = (uint16_t *) retour;
	if (*p != htons(3)) {
		tftp_send_error(socket, dst, (uint16_t)4, "Erreur");
		return -1;
	}	
	else{
		*taille = (size_t) rep;
	}
	return 1;	
}

int tftp_send_ACK_wait_DATA(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen, char *retour, size_t *taille){
	if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }

    int rep = 0;
    for (int i = 0; i < 10; i++){ // Maximum 10 envois
		if ((rep = tftp_send_ACK_wait_DATA_with_timeout(socket, dst, paquet, paquetlen, retour, taille)) == -1){
			return -1;
		}
		if (rep == 1){
			return 1;
		}
    }
    return 0;
}  

int tftp_send_last_ACK(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen){
    if (socket == NULL || dst == NULL || paquet == NULL) {
        return -1;
    }
	if (writeToSocketUDP(socket, dst, paquet, (int) paquetlen) == -1){
		return -1;
	}

    return 0;
}

int donneesRRQ(const char *paquet, char *fichier, char *mode) {
	if (paquet == NULL || fichier == NULL || mode == NULL) {
        return -1;
    }
    uint16_t *p = (uint16_t *)paquet; 
    if (ntohs(*p) != 1) { // Si ce n'est pas un paquet RRQ, on arrête
        return -1;
    }
    memcpy(fichier, paquet + 2, 512);//on récupere le nom du fichier qui ce trouve apres les 2 byte sur le paquet RRQ
    //on récupere le mode de transfert qui ce trouve apres les 2 + strlen(paquet + 2) byte sur le paquet RRQ
    memcpy(mode, paquet + 2 + strlen(paquet + 2) + 1, 512);
	return 0;
}

int donneesDATA(const char *paquet, size_t *bloc, char *data) {
    if (paquet == NULL || bloc == NULL || data == NULL) {
        return -1;
    }
    uint16_t *p = (uint16_t *)paquet;
    if (ntohs(*p) != 3) { // Si ce n'est pas un paquet DATA, on arrête
        printf("Not data packet\n");
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
    if (ntohs(*p) != 4) { // Si ce n'est pas un paquet ACK, on arrête
        return -1;
    }
    *bloc = ntohs(*(p + 1));
    return 0;
}



int tftp_make_rrq_opt(char *buffer, size_t *length, const char *fichier, size_t noctets, size_t nblocks) {
    if (buffer == NULL || length == NULL || fichier == NULL) {
        fprintf(stderr, "[28]buffer == NULL || length == NULL || fichier == NULL\n");
        return -1;
    }

    uint16_t *p = (uint16_t *) buffer;
    *p = htons(1);
    char *c = buffer + 2; 
    strcpy(c, fichier);
    c += strlen(fichier);
    strcpy(c, "octet");
    c += strlen("octet") + 1;
    strcpy(c, "blksize");
    c += strlen("blksize") + 1;
    char s[10];
    sprintf(s, "%d", (int) nblocks);
    strcpy(c, s);
    c += strlen(s) + 1;
    memset(s, '\0', sizeof(s));
    strcpy(c, "windowsize");
    c += strlen("windowsize") + 1;
    sprintf(s, "%d", (int) noctets);
    strcpy(c, s);
    c += strlen(s) + 1;
    *length = strlen(c) + 2;
    return 0;
}


int tftp_make_oack(char * buffer, size_t * length,uint16_t block, size_t noctets, size_t nblocks) {
    if (buffer == NULL || length == NULL) {
        fprintf(stderr, "[31]buffer == NULL || length == NULL\n");
        return -1;
    }

    uint16_t *p = (uint16_t *)buffer;
    *p = htons(6);
    *(p + 1) = htons(block);
    char *c = buffer + 4;
    strcpy(c, "blksize");
    c += strlen("blksize") + 1;
    char s[10];
    sprintf(s, "%d", (int) nblocks);
    strcpy(c, s);
    c += strlen(s) + 1;
    memset(s, '\0', sizeof(s));
    strcpy(c, "windowsize");
    c += strlen("windowsize") + 1;
    sprintf(s, "%d", (int) noctets);
    strcpy(c, s);
    c += strlen(s) + 1;
    *length = strlen(c) + 4;

    return 0;
}


/**
 * Les boucles dans les fonctions suivantes ne s'arrêterons uniquement si les paquets
 * auront été au préalable initialisé avec des '\0' à l'aide d'un memset().
 * Dans le cas contraire elles bouclent jusqu'à un SEG FAULT
 */
int tftp_getInfoFromOACK(char *paquet, size_t *blockno, char *optvals[], int length) {
    if (paquet == NULL || blockno == NULL || optvals == NULL) {
        fprintf(stderr, "[36]paquet == NULL || blockno == NULL || optvals == NULL\n");
    }

    uint16_t *p = (uint16_t *)paquet;

    if (ntohs(*p) != 6) {
        // N'est pas un paquet OACK
        return -1;
    }

    *blockno = ntohs(*(p + 1));

    int i = 0; 
    int position = 2;
    while (strlen(paquet + position) != 0 && i < length) {
        strncpy(optvals[i++], paquet + position, 512);
        position += (int) strlen(paquet + position) + 1;
    }

    return 0;
}

int tftp_getInfoFromRRQ(char *paquet, char *filename, char *mode, char *optvals[], int length) {
    if (paquet == NULL || filename == NULL || mode == NULL || optvals == NULL) {
        fprintf(stderr, "[37paquet == NULL || filename == NULL || mode == NULL || optvals == NULL\n");
        return -1;
    }

    uint16_t *p = (uint16_t *)paquet; 

    if (ntohs(*p) != 1) {
        // N'est pas un paquet RRQ
        return -1;
    }

    strncpy(filename, paquet + 2, 512);
    strncpy(mode, paquet + 2 + (int) (strlen(paquet + 2)) + 1, 512);

    int i = 0;
    int position = 2 + (int) (strlen(paquet + 2)) + 1;
    while (strlen(paquet + position) != 0 && i < length) {
        strncpy(optvals[i++], paquet + position, 512);
        //printf("optvals[i++]=%s\n", paquet + position);
        position += (int) strlen(paquet + position) + 1;
    }

    return 0;
}
