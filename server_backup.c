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
#include "AdresseInternet/AdresseInternet.h"
#include "SockUDP/SocketUDP.h"
#include "TFTP/TFTP.h"
#include "defines.h"

int main(void) {

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

	printf("Waiting for RRQ packet...\n");

	char buffer[BUFFER_SIZE];

	AdresseInternet *client_addr = malloc(sizeof(*client_addr));
	if (client_addr == NULL) {
		perror("malloc");
		closeSocketUDP(sock);		
		return EXIT_FAILURE;
	}

	if (recvFromSocketUDP(sock, buffer, BUFFER_SIZE, client_addr, TIMEOUT_LEN) == -1) {
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
	}

	char filepath[FILE_PATH_LEN];

	strncpy(filepath, FILES_FOLDER, FILE_PATH_LEN);
	strncat(filepath, filename, FILE_PATH_LEN);

	size_t data_size;
	char data[BLK_SIZE];
	memset(data, 0, sizeof(data));
	uint16_t blockNb = 0;

	FILE *file = fopen(filepath, "rb+");
	if (file == NULL) {
		perror("fopen");
		AdresseInternet_free(client_addr);
		closeSocketUDP(sock);
		return EXIT_FAILURE;		
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
			AdresseInternet_free(client_addr);
			closeSocketUDP(sock);			
			return EXIT_FAILURE;
		}

		printf("Sending DATA block %d\n", (int) blockNb);

		if (tftp_send_DATA_wait_ACK(sock, client_addr, buffer, data_size) == 0){
			printf("Timeout exceeded\n");
			AdresseInternet_free(client_addr);
			closeSocketUDP(sock);
			return EXIT_FAILURE;
		}

		printf("Receiving ACK packet %d.\n", blockNb);
	}

	if (fclose(file) != 0) {
		perror("fclose");
		AdresseInternet_free(client_addr);
		closeSocketUDP(sock);
		return EXIT_FAILURE;			
	}


	AdresseInternet_free(client_addr);
	closeSocketUDP(sock);

	return EXIT_SUCCESS;
}