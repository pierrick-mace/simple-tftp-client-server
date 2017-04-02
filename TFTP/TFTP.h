#define NB_MAX_ENVOI 10
#define TIMEOUT_MAX 10
#define BUFFER_SIZE 1024

enum {
	RRQ = 1,
	WRQ,
	DATA,
	ACK,
	ERROR
};

int tftp_make_ack(char *buffer, size_t *length, uint16_t block);

int tftp_make_rrq(char *buffer, size_t *length, const char *fichier);

int tftp_make_data(char *buffer, size_t *length, uint16_t block, const char *data, size_t n);

int tftp_make_error(char *buffer, size_t *length, uint16_t errorcode, const char *message);

void tftp_send_error(SocketUDP *socket, const AdresseInternet *dst,uint16_t code, const char *msg);

int tftp_send_RRQ_wait_DATA_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *fichier, AdresseInternet *connexion, char *reponse, size_t *replength);

int tftp_send_RRQ_wait_DATA(SocketUDP *socket, const AdresseInternet *dst, const char *fichier, AdresseInternet *connexion, char *reponse, size_t *replength);

int tftp_send_DATA_wait_ACK_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen);

int tftp_send_DATA_wait_ACK(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen);

int tftp_send_ACK_wait_DATA_with_timeout(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen, char *retour, size_t *taille);

int tftp_send_ACK_wait_DATA(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen, char *retour, size_t *taille);

int tftp_send_last_ACK(SocketUDP *socket, const AdresseInternet *dst, const char *paquet, size_t paquetlen);

int donneesRRQ(const char *paquet, char *fichier, char *mode);

int donneesDATA(const char *paquet, size_t *bloc, char *data);

int donneesACK(const char *paquet, size_t *bloc);
