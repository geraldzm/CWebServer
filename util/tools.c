#include <stdlib.h>
#include <errno.h>
#include "tools.h"
#include "logger.h"
#include "base64.h"
#include <openssl/sha.h>
#include <ctype.h>

int parseStrToUnsInt(char* str,  unsigned int * result) {

    char *endptr;

    errno = 0;    /* To distinguish success/failure after call */
    *result = strtoul(str, &endptr, 10);

    /* Check for various possible errors */
    if (errno != 0) {
        logError("error parsing str to long");
        return 0;
    }

    if (endptr == str) {
        logWarning("No digits were found to parse str to long");
        return 0;
    }

    /* If we got here, strtol() successfully parsed a number */
    return 1;
}


char* sha1AndBase64(char* msg) {
    // get sha1
    unsigned char sha1[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*) msg, strlen(msg), sha1);

    // to base64
    unsigned int b64s = b64e_size(SHA_DIGEST_LENGTH);
    unsigned char* b64 = malloc(b64s);
    memset(b64, 0, b64s);
    b64_encode(sha1, SHA_DIGEST_LENGTH, b64);

    return (char*) b64;
}


char* trim(char *str) {
    int i;
    int begin = 0;
    unsigned int end = strlen(str) - 1;

    while (isspace(str[begin])) begin++;

    while ((end >= begin) && isspace(str[end])) end--;

    char* trimmed = malloc(end - begin + 2);

    for (i = begin; i <= end; i++) {
        trimmed[i - begin] = str[i];
    }

    trimmed[i - begin] = '\0';

    return trimmed;
}


/* Converts a hex character to its integer value */
char from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
    static char hex[] = "0123456789abcdef";
    return hex[code & 15];
}


char *url_decode(char *str) {
    char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                pstr += 2;
            }
        } else if (*pstr == '+') {
            *pbuf++ = ' ';
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}
