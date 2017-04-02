#ifndef __SOCKET_UDP_H__
#define __SOCKET_UDP_H__

#include "SocketUDPType.h"

// Initialise la socket psocket préalablement allouée
int initSocketUDP(SocketUDP *psocket);

// Attache la socket sock à l'adresse address et au port indiqué, en prenant en compte les options de flags
int attacherSocketUDP(SocketUDP *sock, const char *address, uint16_t port, int flags);

// Renvoie 0 si la socket a une adresse locale, -1 sinon
int estAttachee(SocketUDP *socket);

// Remplie buffer avec le nom local de la socket. 
// Si le buffer est trop petit, le nom est tronqué (et le 0 final n’est pas écrit). Si besoin, socket est mis à jour.
// Renvoie le nombre d’octets écrits dans buffer, ou -1 en cas d’erreur.
int getLocalName(SocketUDP *socket, char *buffer, int taille);

// Remplie localIP avec l’IP locale de la SocketUDP. 
// Si le buffer est trop petit, le nom est tronqué (et le 0 final n’est pas écrit).
// Renvoie le nombre d’octets écrits dans buffer, ou -1 en cas d’erreur.
int getLocalIP(const SocketUDP *socket, char *localIP, int tailleIP);

// Renvoie le port de la socket
uint16_t getLocalPort(const SocketUDP *socket);

// Ecrit sur la socket sock vers adresse un bloc d’octets buffer de taille length et retourne la
// taille des données écrites ou -1 s’il y a erreur.
ssize_t writeToSocketUDP(SocketUDP *sock, const AdresseInternet *address, const char *buffer, int length);

// Lit sur socket les données envoyées par une machine d’adresse adresse. 
// La fonction est bloquante pendant timeout secondes. 
// Elle lit et place dans buffer un bloc d’octets de taille au plus length. 
// Renvoie la taille des données lues ou -1 s’il y a erreur.
ssize_t recvFromSocketUDP(SocketUDP *socket, char *buffer, int length, AdresseInternet *address, int timeout);

// Ferme la connexion et libère la SocketUDP.
// Renvoie 0 en cas de succès, -1 en cas d’erreur.
int closeSocketUDP(SocketUDP *socket);

#endif