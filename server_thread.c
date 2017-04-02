#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include "AdresseInternet/AdresseInternet.h"
#include "SockUDP/SocketUDP.h"
#include "TFTP/TFTP.h"
#include "defines.h"

typedef struct {
	SocketUDP *socket;
	char mode[MODE_LEN];
	char filepath[FILE_PATH_LEN];
	AdresseInternet *client_addr;
} server_thread_info;

static volatile sig_atomic_t quit;

void *server_thread(void *arg);

int main(void) {

	quit = false;

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

	AdresseInternet *client_addr = malloc(sizeof(*client_addr));
	if (client_addr == NULL) {
		perror("malloc");
		closeSocketUDP(sock);		
		return EXIT_FAILURE;
	}

	//while (!quit) {

		printf("Waiting for RRQ packet...\n");

		char buffer[BUFFER_SIZE];

		if (recvFromSocketUDP(sock, buffer, BUFFER_SIZE, client_addr, NO_TIMEOUT) == -1) {
			perror("recvFromSocketUDP");
			AdresseInternet_free(client_addr);
			closeSocketUDP(sock);
			return EXIT_FAILURE;
		}

		char filename[FILENAME_LEN];
		char mode[MODE_LEN];

		if (donneesRRQ(buffer, filename, mode) != -1) {
			printf("RRQ packet received\n");
			printf("File name: %s\nTransfer mode: %s\n", filename, mode);

			server_thread_info *th_info = malloc(sizeof(*th_info));
			if (th_info == NULL) {
				perror("malloc");
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				return EXIT_FAILURE;			
			}

			memset(th_info, 0, sizeof(*th_info));

			th_info -> socket = sock;
			strncpy(th_info -> filepath, FILES_FOLDER, FILE_PATH_LEN);
			strncat(th_info -> filepath, filename, FILE_PATH_LEN);
			strncpy(th_info -> mode, mode, MODE_LEN);
			th_info -> client_addr = malloc(sizeof(AdresseInternet));
			AdresseInternet_copy(th_info -> client_addr, client_addr);

			printf("filepath: %s\n", th_info -> filepath);

			pthread_t th;
	   
	    	int err;
	    	if ((err = pthread_create(&th, NULL, server_thread, (void *) th_info)) != 0) {
		    	fprintf(stderr, "Error: pthread_create (%s)\n", strerror(err));
				perror("malloc");
				AdresseInternet_free(client_addr);
				closeSocketUDP(sock);
				free(th_info);	    	
		       	exit(EXIT_FAILURE);
	    	}
	    }

	//}

	AdresseInternet_free(client_addr);
	closeSocketUDP(sock);

	return EXIT_SUCCESS;
}

void *server_thread(void *arg) {
	
	server_thread_info *th_info = (server_thread_info *) arg;

	char buffer[BUFFER_SIZE];
	size_t data_size;
	char data[BLK_SIZE];
	memset(data, 0, sizeof(data));
	uint16_t blockNb = 0;

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

		if (data_len < 512) {
			printf("Sending last DATA packet\n");
			done = true;
		}

		if (tftp_make_data(buffer, &data_size, ++blockNb, data, data_len) == -1){
			perror("tftp_make_data");
			AdresseInternet_free(th_info -> client_addr);
			closeSocketUDP(th_info -> socket);
			free(th_info);
			return NULL;
		}

		printf("Sending DATA block %d\n", (int) blockNb);

		if (tftp_send_DATA_wait_ACK(th_info -> socket, th_info -> client_addr, buffer, data_size) == 0){
			printf("Timeout exceeded\n");
			AdresseInternet_free(th_info -> client_addr);
			closeSocketUDP(th_info -> socket);
			free(th_info);
			return NULL;
		}

		printf("Receiving ACK packet %d.\n", blockNb);
	}

	if (fclose(file) != 0) {
		perror("fclose");
		AdresseInternet_free(th_info -> client_addr);
		closeSocketUDP(th_info -> socket);
		free(th_info);	
		return NULL;			
	}

	AdresseInternet_free(th_info -> client_addr);
	free(th_info);

	return NULL;
}