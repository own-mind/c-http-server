#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "qrcode.h"

#define M_SIZE 25
#define M_RATE 16

QRCode *toQRImage(byte **matrix, int w, int h);
byte **upscale(byte **matrix, int w, int h, int rate);

QRCode *generateQR(char* data, int n) {
    byte **matrix = malloc(M_SIZE * sizeof(byte*));
    for (int i = 0; i < M_SIZE; i++) {
        matrix[i] = calloc(M_SIZE, sizeof(byte));
        matrix[i][i] = 1u;
    }

    byte **upscaled = upscale(matrix, M_SIZE, M_SIZE, M_RATE);
    QRCode *qr = toQRImage(upscaled, M_SIZE * M_RATE, M_SIZE * M_RATE);

    for (int i = 0; i < M_SIZE; i++) free(matrix[i]);
    free(matrix);

    return qr;
}

byte **upscale(byte **matrix, int w, int h, int rate) {
    int uw = rate * w;
    int uh = rate * h;
    byte **result = malloc(uh * sizeof(byte*));
    for (int i = 0; i < uh; i++) {
        result[i] = calloc(uw, sizeof(byte));
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            byte val = matrix[y][x];

            for (int i = 0; i < rate; i++) {
                for (int j = 0; j < rate; j++) {
                    result[y * rate + i][x * rate + j] = val;
                }
            }
        }
    }

    return result;
}

QRCode *toQRImage(byte **matrix, int w, int h) {
    unsigned int headersSize =
        14          // Headers
        + 40        // Info headers
        + 4 * 2;    // Colors (black and white)
    int rowSize = ((w + 31) / 32) * 4;
    unsigned int totalSize = headersSize + rowSize * h; // Data (no compression)
    unsigned char *result = calloc(totalSize, sizeof(byte));

    // Signature
    result[0] = 'B';
    result[1] = 'M';

    // File size
    *((int*)(result + 2)) = totalSize;

    // 4 bytes skipped
    // Off bits
    *((int*)(result + 10)) = headersSize;

    // Header size (40 bits)
    *((int*)(result + 14)) = 40u;

    // Width 
    *((int*)(result + 18)) = w;

    // Height 
    *((int*)(result + 22)) = -h;

    // Planes (always 1)
    *((short*)(result + 26)) = 1u;

    // Bit count (monochorome -> 1)
    *((short*)(result + 28)) = 1u;

    // Compression (none -> 0)
    *((int*)(result + 30)) = 0u;

    // Size image (0 because uncompressed)
    *((int*)(result + 34)) = 0u;

    // Pixels per meter X 
    *((int*)(result + 38)) = w;

    // Pixels per meter Y 
    *((int*)(result + 42)) = h;

    // Colors used 
    *((int*)(result + 46)) = 2u;

    // Important colors (all)
    *((int*)(result + 50)) = 0u;

    // White
    result[54] = 255;
    result[55] = 255;
    result[56] = 255;
    // 0 for last byte

    // Black is all 0, so we skip 4 bytes

    unsigned char *data = result + headersSize;

    for (int y = 0; y < h; y++) {
        unsigned char cb = 0; // Collecting byte
        int ci = 0;
        int ri = 0;
        for (int x = 0; x < w; x++) {
            cb = (cb << 1) | (matrix[y][x] ? 1 : 0);
            ci++;
            if (ci == 8) {
                data[ri++] = cb;
                cb = 0;
                ci = 0;
            }
        }
        if (ci > 0) {
            cb <<= (8 - ci);
            data[ri++] = cb;
        }

        data += rowSize;
    }

    QRCode *qr = calloc(1, sizeof(QRCode));
    qr->data = result;
    qr->len = totalSize;

    return qr;
}

void freeQR(QRCode *qr) {
    free(qr->data);
    free(qr);
}
