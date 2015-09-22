#ifndef WRK_BASE64_H
#define WRK_BASE64_H

#include <stddef.h>

/**
 * Encode the specified buffer to base64.
 *
 * @return pointer to encoded buffer in base64. Receiver must free() this
 *	pointer.
 */
char* base64_encode(const char* buff, size_t len);

#endif /* WRK_BASE64_H */
