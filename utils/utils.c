#include <errno.h>
#include "utils.h"

// Convertit une chaîne de caractères en uint16_t
// Renvoie 0 en cas de succès et -1 en cas d'erreur
int str_to_uint16(const char *str, uint16_t *res) {
  char *end;
  errno = 0;
  intmax_t val = strtoimax(str, &end, 10);
  
  if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0') {
    return -1;
  }

  *res = (uint16_t) val;
  
  return 0;
}
