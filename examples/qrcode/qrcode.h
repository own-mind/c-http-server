#ifndef __QRCODE_H
#define __QRCODE_H

typedef unsigned char byte;

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
QRCode *generateQR(char* data, int n);

void freeQR(QRCode *qr);

#endif
