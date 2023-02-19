//
//#define SHA_DIGEST_LENGTH 20
//
//unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md)
//{
//    static unsigned char m[SHA_DIGEST_LENGTH];
//
//    if (md == NULL) md = m;
//    return EVP_Q_digest(NULL, "SHA1", NULL, d, n, md, NULL) ? md : NULL;
//}