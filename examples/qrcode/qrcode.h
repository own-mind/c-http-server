#ifndef __QRCODE_H
#define __QRCODE_H

typedef unsigned char byte;

typedef struct {
    byte *data;
    int len;
} QRCode;

QRCode *generateQR(char* data, int n);

void freeQR(QRCode *qr);

#endif
