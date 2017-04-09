#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include "AdresseInternet/AdresseInternet.h"
#include "SockUDP/SocketUDP.h"
#include "TFTP/TFTP.h"
#include "defines.h"

// Structure pour les threads
typedef struct {
	SocketUDP *socket;
	char mode[MODE_LEN];
	char filepath[FILE_PATH_LEN];
	AdresseInternet *client_addr;
} server_thread_info;

// Variable globale signalant l'arrêt du serveur
static volatile sig_atomic_t quit;

// Fonction du thread
void *server_thread(void *arg);

// Fonction de gestion des signaux
void signal_handler(int signum);

int main(void) {

	quit = false;

	// Construction de la gestion des signaux
	struct sigaction action;

	action.sa_handler = signal_handler;
	action.sa_flags = SA_NOCLDWAIT;
	if (sigfillset(&action.sa_mask) == -1) {
		perror("sigfillset");
		exit(EXIT_FAILURE);
	}

	if (sigaction(SIGINT, &action, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}


	// Initialisation de la socket
	SocketUDP *sock = malloc(sizeof(*sock));
	if (sock == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	if (initSocketUDP(sock) == -1) {
		perror("malloc");
	}

	if (attacherSocketUDP(sock, NULL, SERVER_PORT, LOOPBACK) == -1) {
		perror("attacherSocketUDP");
		closeSocketUDP(sock);
		return EXIT_FAILURE;
	}

	// Allocation de l'espace mémoire destiné à être rempli par l'addresse du client
	AdresseInternet *client_addr = malloc(sizeof(*client_addr));
	if (client_addr == NULL) {
		perror("malloc");
		closeSocketUDP(sock);		
		return EXIT_FAILURE;
	}

	// Boucle principale
	while (!quit) {

		printf("Waiting for RRQ packet...\n");

		char buffer[BUFFER_SIZE];
		memset(buffer, 0, sizeof(buffer));

		// Attente d'un paquet RRQ

		if (recvFromSocketUDP(sock, buffer, BUFFER_SIZE, client_addr, NO_TIMEOUT) == -1) {
			if (!quit) {
				perror("recvFromSocketUDP");
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				return EXIT_FAILURE;
			} else {
				break;
			}
		}

		char filename[FILENAME_LEN];
		char mode[MODE_LEN];

		if (donneesRRQ(buffer, filename, mode) != -1) {
			printf("RRQ packet received\n");
			printf("File name: %s\nTransfer mode: %s\n", filename, mode);

			// Initialisation de la structure contenant les variables utiles au thread

		    server_thread_info *th_info = malloc(sizeof(*th_info));
			if (th_info == NULL) {
				perror("malloc");
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				return EXIT_FAILURE;			
			}

			memset(th_info, 0, sizeof(*th_info));

			// Initialisation de la socket utilisée pendant le transfert du fichier

			th_info -> socket = malloc(sizeof(*sock));
			if (th_info -> socket == NULL) {
				perror("malloc");
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				free(th_info);	    	
				return EXIT_FAILURE;				
			}
			
			if (initSocketUDP(th_info -> socket) == -1) {
				perror("malloc");
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				free(th_info);	    	
				return EXIT_FAILURE;
			}

			// Si la socket est déjà utilisée alors elle est sûrement utilisée par un autre thread,
			// On attend donc que l'autre thread finisse son transfert
			errno = 0;
			int val = attacherSocketUDP(th_info -> socket, NULL, (uint16_t) (SERVER_TRANSFERT_PORT), LOOPBACK);
			if (val == -1) {
				if (errno == EADDRINUSE) {
					while (errno == EADDRINUSE) {
						errno = 0;
						val = attacherSocketUDP(th_info -> socket, NULL, (uint16_t) (SERVER_TRANSFERT_PORT), LOOPBACK);
						sleep(1);
					}

					if (val == -1) {
						perror("attacherSocketUDP");
						AdresseInternet_free(client_addr);
						closeSocketUDP(sock);
						free(th_info);	    	
						return EXIT_FAILURE;
					}
				} else {
					perror("attacherSocketUDP");
					AdresseInternet_free(client_addr);
					closeSocketUDP(sock);
					free(th_info);	    	
					return EXIT_FAILURE;
				}
			}

			// Copie des informations dans la structure du thread
			strncpy(th_info -> filepath, FILES_FOLDER, FILE_PATH_LEN);
			strncat(th_info -> filepath, filename, FILE_PATH_LEN);
			strncpy(th_info -> mode, mode, MODE_LEN);
			th_info -> client_addr = malloc(sizeof(AdresseInternet));

			AdresseInternet_copy(th_info -> client_addr, client_addr);

			pthread_t th;
	   
	   		// Lancement du thread
	    	int err;
	    	if ((err = pthread_create(&th, NULL, server_thread, (void *) th_info)) != 0) {
		    	fprintf(stderr, "Error: pthread_create (%s)\n", strerror(err));
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				free(th_info);	    	
		       	exit(EXIT_FAILURE);
	    	}

	    	// Le thread est placé en mode détaché afin qu'il libère automatiquement les ressources allouées

	    	if (pthread_detach(th) == -1) {
	    		perror("pthread_detach");
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				free(th_info);	    	
		       	exit(EXIT_FAILURE);	    		
	    	}
	    }

	}

	// Libération de la mémoire

	AdresseInternet_free(client_addr);
	closeSocketUDP(sock);

	printf("Server shutting down\n");

	return EXIT_SUCCESS;
}

void *server_thread(void *arg) {

	server_thread_info *th_info = (server_thread_info *) arg;

	char buffer[BUFFER_SIZE];
	size_t data_size;
	char data[BLK_SIZE];
	memset(data, 0, sizeof(data));
	uint16_t blockNb = 0;

	// Ouverture du fichier à transférer

	FILE *file = fopen(th_info -> filepath, "rb+");
	if (file == NULL) {
		perror("fopen");
		AdresseInternet_free(th_info -> client_addr);
		closeSocketUDP(th_info -> socket);
		free(th_info);			
		return NULL;		
	}

	bool done = false;

	while (!done) {
		size_t data_len = 0;

		data_len = fread(data, sizeof(char), BLK_SIZE, file);

		if (data_len < BLK_SIZE) {
			printf("Sending last DATA packet\n");
			done = true;
		}

		// Création du paquet de données

		if (tftp_make_data(buffer, &data_size, ++blockNb, data, data_len) == -1){
			perror("tftp_make_data");
			AdresseInternet_free(th_info -> client_addr);
			closeSocketUDP(th_info -> socket);
			free(th_info);
			return NULL;
		}

		printf("Sending DATA block %d\n", (int) blockNb);

		// Envoi du paquet de données et attente du paquet ACK
		if (tftp_send_DATA_wait_ACK(th_info -> socket, th_info -> client_addr, buffer, data_size) == 0){
			printf("Timeout exceeded\n");
			AdresseInternet_free(th_info -> client_addr);
			closeSocketUDP(th_info -> socket);
			free(th_info);
			return NULL;
		}

		printf("Receiving ACK packet %d.\n", blockNb);
	}

	// Fermeture du fichier
	if (fclose(file) != 0) {
		perror("fclose");
		AdresseInternet_free(th_info -> client_addr);
		closeSocketUDP(th_info -> socket);
		free(th_info);	
		return NULL;			
	}

	printf("File transfer complete\n");

	// Libération de la mémoire

	closeSocketUDP(th_info -> socket);	
	AdresseInternet_free(th_info -> client_addr);
	free(th_info);
	return NULL;
}

void signal_handler(int signum) {
	if (signum < 0) {
		fprintf(stderr, "Wrong signal number\n");
	}

	quit = true;
}
