#ifndef __QRCODE_H
#define __QRCODE_H

typedef unsigned char byte;
// Represents integer number X as exponent of 2
typedef int alpha;

extern alpha EC_GEN_L[11];
extern alpha EC_GEN_M[17];
extern alpha EC_GEN_Q[23];
extern alpha EC_GEN_H[29];

typedef struct {
    byte *data;
    int len;
} QRCode;

typedef enum {
    ECI = 7u,        // Unsupported
    NUMERIC = 1u,     
    ALPHANUMERIC = 2u,
    BYTE = 4u,
    KANJI = 8u,       // Unsupported
    STRUCTURED_APPEND = 3u // Unsupported
} Mode;

typedef struct {
    Mode mode;
    int length;
} ModeGroup;


ModeGroup selectMode(char *data, int n);
int encodeNumeric(byte *bitBuffer, int *bi, char *data, int n);
int generateEC(byte *message, int mn, byte* writeTo, int ecwords, alpha *generator);
QRCode *generateQR(char* data, int n);
void packData(byte **matrix, byte *data, int n);
void freeMatrix(byte **matrix, int n);

void freeQR(QRCode *qr);

#endif
