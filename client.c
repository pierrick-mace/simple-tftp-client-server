#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "AdresseInternet/AdresseInternet.h"
#include "SockUDP/SocketUDP.h"
#include "TFTP/TFTP.h"
#include "utils/utils.h"
#include "defines.h"

int main(int argc, char *argv[]) {

	if (argc < 3) {
		printf("Usage: client file port\n");
		return EXIT_FAILURE;
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

	uint16_t port;
	char filename[FILENAME_LEN];

	str_to_uint16(argv[2], &port);

	strcpy(filename, argv[1]);

	if (attacherSocketUDP(sock, NULL, port, LOOPBACK) == -1) {
		perror("attacherSocketUDP");
		closeSocketUDP(sock);
		return EXIT_FAILURE;
	}

	AdresseInternet *server = AdresseInternet_new("127.0.0.1", SERVER_PORT);
	if (server == NULL) {
		perror("AdresseInternet_new");
		closeSocketUDP(sock);
		return EXIT_FAILURE;
	}

	AdresseInternet *tmp = malloc(sizeof(*tmp));
	if (tmp == NULL) {
		perror("malloc");
		closeSocketUDP(sock);
		return EXIT_FAILURE;
	}

	char buffer[BUFFER_SIZE];
	char response[BLK_SIZE + 4];
	size_t response_len = sizeof(response);

	printf("Sending RRQ packet to server...\n");

	if (tftp_send_RRQ_wait_DATA(sock, server, filename, tmp, response, &response_len) == 0) {
    	printf("Timeout exceeded\n");
    	closeSocketUDP(sock);
    	AdresseInternet_free(tmp);
		return EXIT_FAILURE;
	}


	AdresseInternet_copy(server, tmp);

	size_t blockNb;
	size_t expectedBlockNb = 1;

	char data[BLK_SIZE];

	bool done = false;

	size_t data_len;

	FILE *file = fopen(filename, "wb+");
	if (file == NULL) {
		perror("fopen");
		closeSocketUDP(sock);
		AdresseInternet_free(tmp);
		return EXIT_FAILURE;
	}

	while (!done) {
		if (donneesDATA(response, &blockNb, data) == -1){
			perror("donneesDATA");
			closeSocketUDP(sock);
			AdresseInternet_free(tmp);
			return EXIT_FAILURE;
		}

		if (blockNb == expectedBlockNb) {
			printf("Receiving DATA packet\ndata: \n%s\n", data);
			expectedBlockNb++;

			size_t size = fwrite(data, sizeof(char), (response_len - 4), file);
			if (size < 512) {
				printf("Sending last packet\n");
				done = true;
			}

			printf("Sending ACK to server\n");

			memset(response, 0, sizeof(response));
			memset(data, 0, sizeof(data));

			if (tftp_make_ack(buffer, &data_len, (uint16_t) blockNb) == -1) {
				perror("tftp_make_ack");
				exit(EXIT_FAILURE);
			}
		}

		if (done) {
			tftp_send_last_ACK(sock, server, buffer, data_len);
		} else {
			if (tftp_send_ACK_wait_DATA(sock, server, buffer, data_len, response, &response_len) == 0){
				printf("Timeout exceeded\n");
				closeSocketUDP(sock);
				AdresseInternet_free(tmp);
				return EXIT_FAILURE;
			}
		}
	}

 	printf("File transfer complete\n");	

	if (fclose(file) != 0) {
		perror("fclose");
		closeSocketUDP(sock);
		AdresseInternet_free(tmp);
		AdresseInternet_free(server);		
		return EXIT_FAILURE;
	}

	closeSocketUDP(sock);
	AdresseInternet_free(tmp);
	AdresseInternet_free(server);

	return EXIT_SUCCESS;
}
