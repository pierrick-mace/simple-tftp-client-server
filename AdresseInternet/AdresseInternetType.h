#ifndef _ADRESSEINTERNETTYPE_H_
#define _ADRESSEINTERNETTYPE_H_

#include <sys/socket.h>
#include <arpa/inet.h>

#define _DNS_NAME_MAX_SIZE 256
#define _SERVICE_NAME_MAX_SIZE 20
#define _IP_MAX_SIZE INET6_ADDRSTRLEN

typedef struct {
  socklen_t addrlen;
  struct sockaddr_storage sockAddr;
  char nom[_DNS_NAME_MAX_SIZE];
  char service[_SERVICE_NAME_MAX_SIZE];
} _adresseInternet_struct;

typedef _adresseInternet_struct AdresseInternet;

#endif // ADRESSEINTERNETTYPE_H_
