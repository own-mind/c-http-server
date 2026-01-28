#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "qrcode.h"

#define M_SIZE 25
#define M_RATE 16

#define DATA_SIZE 28
#define EC_SIZE 16
#define ENCODING_SIZE DATA_SIZE + EC_SIZE

QRCode *toQRImage(byte **matrix, int w, int h);
byte **upscale(byte **matrix, int w, int h, int rate);

byte STATIC_MASK[M_SIZE][M_SIZE] = {
    { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1 },
    { 1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1 },
    { 1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,1 },
    { 1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,1 },
    { 1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,1 },
    { 1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1 },
    { 1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0 },
    { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0 },
    { 1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0 },
    { 1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0 },
    { 1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
};

byte DATA_MASK[M_SIZE][M_SIZE] = {
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    { 0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
};

void applyMaskS(byte **matrix, byte mask[][M_SIZE]) {
    for (int y = 0; y < M_SIZE; y++) {
        for (int x = 0; x < M_SIZE; x++) {
            if(mask[y][x]) 
                matrix[y][x] = mask[y][x];
        }
    }
}

void applyMask(byte **matrix, byte **mask) {
    for (int y = 0; y < M_SIZE; y++) {
        for (int x = 0; x < M_SIZE; x++) {
            if(mask[y][x]) 
                matrix[y][x] = mask[y][x];
        }
    }
}

void packData(byte **matrix, byte *data, int n) {
    int px = M_SIZE - 1;
    int py = M_SIZE - 1;
    int upwards = 1;  // Direction flag
    int leftward = 0; // Place on the left or the right

    for (int i = 0; i < n; i++) {
        byte word = data[i];

        for (int j = 0; j < 8; j++) {   //TODO Maybe has to be reversed bit order
            int bit = word & 1u;
            word >>= 1;

            if (bit) matrix[py][px] = 1u;

            do {
                if (!leftward) {
                    px--;   // Going left
                    leftward = 1;
                } else {
                    px++;  // Going right
                    leftward = 0;
                    if (upwards) {
                        py--;

                        if (py < 0) {
                            upwards = 0;
                            px -= 2; // Moving left on the next column
                            py++;
                        }
                    } else {
                        py++;

                        if (py >= M_SIZE) {
                            upwards = 1;
                            px -= 2; // Moving left on the next column
                            py--;
                        }
                    }
                }

                if (px == 6) {  //Skipping zero-filled vertical 
                    px--;
                }
            } while (!DATA_MASK[py][px]);
        }
    }
}

QRCode *generateQR(char* data, int n) {
    byte **matrix = malloc(M_SIZE * sizeof(byte*));
    for (int i = 0; i < M_SIZE; i++) {
        matrix[i] = calloc(M_SIZE, sizeof(byte));
    }

    applyMaskS(matrix, STATIC_MASK);

    byte *encoding = calloc(ENCODING_SIZE, sizeof(byte));
    for (int i = 0; i < ENCODING_SIZE; i++) {
        encoding[i] = 255u;
    }

    packData(matrix, encoding, ENCODING_SIZE);
    free(encoding);

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
