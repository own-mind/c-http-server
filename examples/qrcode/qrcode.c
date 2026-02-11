#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "qrcode.h"
#include "math.h"

#define M_SIZE 25
#define M_RATE 16

#define MASK_N1 3
#define MASK_N2 3
#define MASK_N3 40
#define MASK_N4 10

#define DATA_SIZE_L 34
#define DATA_SIZE_M 28
#define DATA_SIZE_Q 22
#define DATA_SIZE_H 16
#define ENCODING_SIZE 44

alpha EC_GEN_L[11] = { 0, 251, 67, 46, 61,118, 70, 64, 94, 32, 45 };
alpha EC_GEN_M[17] = { 0, 120,104,107,109,102,161, 76,  3, 91,191,
                           147,169,182,194,225,120 };
alpha EC_GEN_Q[23] = { 0,210,171,247,242, 93,230, 14,109,221, 53,
                           200, 74,  8,172, 98, 80,219,134,160,105,165,231 };
alpha EC_GEN_H[29] = { 0,168,223,200,104,224,234,108,180,110,190,
                           195,147,205, 27,232,201, 21, 43,245, 87,
                            42,195,212,119,242, 37,  9,123 };

byte DEC_TO_ALPH[256];
byte ALPH_TO_DEC[256];

char ALPHANUMS[45] = {
    '0','1','2','3','4','5','6','7','8','9',
    'A','B','C','D','E','F','G','H','I','J',
    'K','L','M','N','O','P','Q','R','S','T',
    'U','V','W','X','Y','Z',' ','$','%','*',
    '+','-','.','/',':'
};
int CHAR_TO_ALPHANUM[256];

__attribute__((constructor))
void generateUtilArrays() {
    unsigned int num = 1;
    for (int i = 1; i < 256; i++) {
        num = 2 * num;
        if (num > 255) {
            num ^= 285u;
        }

        DEC_TO_ALPH[num] = (byte) i;
        ALPH_TO_DEC[i] = num;

        CHAR_TO_ALPHANUM[i] = -1;   // Fail if encountered
    }
    ALPH_TO_DEC[0] = 1;

    for (int i = 0; i < 45; i++) {
        CHAR_TO_ALPHANUM[(int)ALPHANUMS[i]] = i;
    }
}

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
    { 0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0 },
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

byte FORMATTING_MASK[M_SIZE][M_SIZE] = {
    { 00,00,00,00,00,00,00,00,1 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,2 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,3 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,4 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,5 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,6 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,7 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 15,14,13,12,11,10,00,9 ,8 ,00,00,00,00,00,00,00,00,8 ,7 ,6 ,5 ,4 ,3 ,2 ,1 },
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,9 ,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,10,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,11,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,12,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,13,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,14,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
    { 00,00,00,00,00,00,00,00,15,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00},
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

        for (int j = 7; j >= 0; j--) {
            int bit = (word >> j) & 1u;

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

typedef struct {
    int data;
    int total;
} ModeBitSize;

ModeBitSize modeBitSize(Mode mode, int length) {
    static const int header = 4;
    int data; 

    switch (mode) {
    case NUMERIC:
            // 10 bits for every 3 digits
            data = (int)ceil(length / 3.0f) * 10;
            return (ModeBitSize) { data, data + 10 + header };
        case ALPHANUMERIC:
            data = (int)ceil(length / 2.0f) * 11;
            return (ModeBitSize) { data, data + 9 + header };
        case BYTE:
            data = length * 8;
            return (ModeBitSize) { data, data + 8 + header };
        case ECI: case KANJI: case STRUCTURED_APPEND:
            perror("Not supported QR mode");
            exit(1);
    }
    return (ModeBitSize) {0,0};
}

// Chooses best mode to encode the next data, accounting for mode setting cost
ModeGroup selectMode(char *data, int n) {
    Mode current = NUMERIC;  // Currently selected mode, CANNOT decrease in value 
    int startNumeric = 0;    // Number of consecutive numeric chars from start
    int numeric = 0;         // Number of consecutive numeric chars
    int startAlpha = 0;      // Number of consecutive alphanumetic chars from start
    int alpha = 0;           // Number of consecutive alphanumetic chars
    int length = 0;          // Number of consecutive bytes going into current mode

    int nextSize, currentSize;
    for (int i = 0; i < n; i++) {
        char c = data[i];
        length++;

        if (c >= '0' && c <= '9') {
            if (current == NUMERIC) startNumeric++;
            if (current == NUMERIC || current == ALPHANUMERIC) startAlpha++;
            numeric++;
            alpha++;
        } else if (CHAR_TO_ALPHANUM[(int)c] >= 0){
            if (current == ALPHANUMERIC || current == NUMERIC) startAlpha++;
            alpha++;
            numeric = 0;

            if (current == NUMERIC) current = ALPHANUMERIC;
        } else {
            alpha = 0;
            numeric = 0;
            current = BYTE;
        }

        if (current != NUMERIC && numeric > 0) {
            currentSize = modeBitSize(current, numeric).data;  // The amount of bits current encoding will take up
            nextSize = modeBitSize(NUMERIC, numeric).total;  // Headers + data in bits

            if (nextSize < currentSize) {
                length -= numeric;
                break;
            }
        }

        if (current != ALPHANUMERIC && alpha > 0) {
            currentSize = modeBitSize(current, alpha).data;  // The amount of bits current encoding will take up
            nextSize = modeBitSize(ALPHANUMERIC, alpha).total;  // Headers + data in bits

            if (nextSize < currentSize) {
                length -= alpha;
                break;
            }
        }
    }

    if (startNumeric > 0) {
        currentSize = modeBitSize(current, startNumeric).total;
        nextSize = modeBitSize(NUMERIC, startNumeric).total;
        if (nextSize < currentSize) {
            return (ModeGroup) { NUMERIC, startNumeric };
        }
    }

    if (startAlpha > 0) {
        currentSize = modeBitSize(current, startAlpha).total;
        nextSize = modeBitSize(ALPHANUMERIC, startAlpha).total;
        if (nextSize < currentSize) {
            return (ModeGroup) { ALPHANUMERIC, startAlpha };
        }
    }

    return (ModeGroup) { current, length };
}

int validateSize(int bi) {
    return ceil(bi / 8.0f) <= DATA_SIZE_L;
}

int encodeNumeric(byte *bitBuffer, int *bi, char *data, int n) {
    unsigned int triplet = 0u;
    int tripletDumped = 1;
    int i;
    for (i = 0; i < n; i++) {
        if (!validateSize(*bi + 10)) return 0;

        if (i > 0 && i % 3 == 0) {
            for (int j = 9; j >= 0; j--) {
                bitBuffer[(*bi)++] = (triplet >> j) & 1;
            }
            triplet = 0;
            tripletDumped = 1;
        }

        triplet *= 10u;
        triplet += (unsigned int) (data[i] - '0');
        tripletDumped = 0;
    }

    if (!tripletDumped) {
        if (!validateSize(*bi + 10)) return 0;

        // Remaining numbers should be flushed corresponding to their size
        int j = n % 3 == 1 ? 3
            : n % 3 == 2 ? 6
            : 9;

        for (; j >= 0; j--) {
            bitBuffer[(*bi)++] = (triplet >> j) & 1;
        }
    }

    return 1;
}

int encodeAlphanumeric(byte *bitBuffer, int *bi, char *data, int n) {
    for (int i = 0; i < n - 1; i += 2) {
        if (!validateSize(*bi + 11)) return 0;

        int val = CHAR_TO_ALPHANUM[(int)data[i]] * 45 + CHAR_TO_ALPHANUM[(int)data[i + 1]];
        for (int j = 10; j >= 0; j--) {
            bitBuffer[(*bi)++] = (val >> j) & 1;
        }
    }

    if (n & 1) {
        if (!validateSize(*bi + 6)) return 0;

        int val = CHAR_TO_ALPHANUM[(int)data[n - 1]];
        for (int j = 5; j >= 0; j--) {
            bitBuffer[(*bi)++] = (val >> j) & 1;
        }
    }

    return 1;
}

int encodeByte(byte *bitBuffer, int *bi, char *data, int n) {
    for (int i = 0; i < n; i++) {
        if(!validateSize(*bi + 8)) return 0;

        char c = data[i];
        for (int j = 7; j >= 0; j--) {
            bitBuffer[(*bi)++] = (c >> j) & 1;
        }
    }

    return 1;
}

int encodeData(byte *result, int rn, char *data, int n) {
    byte *bitBuffer = calloc(rn * 8, sizeof(byte));   // byte per bit array
    int bi = 0;     // Bit buffer index
    int idx = 0;    // Data index
    while (idx < n) {
        ModeGroup modeGroup = selectMode(data + idx, n - idx);

        for (int b = 3; b >= 0; --b) {
            bitBuffer[bi++] = (modeGroup.mode >> b) & 1;
        }

        int success; 
        if (modeGroup.mode == NUMERIC) {
            for (int b = 9; b >= 0; --b) {
                bitBuffer[bi++] = (modeGroup.length >> b) & 1;
            }

            success = encodeNumeric(bitBuffer, &bi, data + idx, modeGroup.length);
        } else if (modeGroup.mode == ALPHANUMERIC) {
            for (int b = 8; b >= 0; --b) {
                bitBuffer[bi++] = (modeGroup.length >> b) & 1;
            }

            success = encodeAlphanumeric(bitBuffer, &bi, data + idx, modeGroup.length);
        } else if (modeGroup.mode == BYTE) {
            for (int b = 7; b >= 0; --b) {
                bitBuffer[bi++] = (modeGroup.length >> b) & 1;
            }

            success = encodeByte(bitBuffer, &bi, data + idx, modeGroup.length);
        } else {
            return -1;
        }

        if (!success) {   // Typically means encoding function got too much data
            free(bitBuffer);
            return -1;
        }

        idx += modeGroup.length;
    }

    int terminatorBits = (int) fmin(DATA_SIZE_L * 8 - bi, 4);
    for (int i = 0; i < terminatorBits; i++) {
        bitBuffer[bi++] = 0;
    }

    // Flushing bit buffer to actual bit array
    int size = 0;
    unsigned char cb = 0;
    int bitCount = 0;
    for (int i = 0; i < bi; ++i) {
        cb = (cb << 1) | (bitBuffer[i] & 1);
        if (++bitCount == 8) {
            result[size++] = cb;
            cb = 0;
            bitCount = 0;
        }
    }
    if (bitCount) {
        cb <<= (8 - bitCount);
        result[size++] = cb;
    }

    free(bitBuffer);
    return size;
}

static inline alpha ec_itoa(int value) {
    return DEC_TO_ALPH[value];
}

static inline int ec_atoi(alpha value) {
    return ALPH_TO_DEC[value % 255];
}

int generateEC(byte *message, int mn, byte* writeTo, int ecwords, alpha *generator) {
    int *poly = calloc(mn + ecwords, sizeof(int));
    for (int s = 0; s < mn; s++) {
        poly[s] = (int) message[s];
    }

    int gn = ecwords + 1; // Length of generator poly
    for (int s = 0; s < mn; s++) {
        if (poly[s] == 0) continue;
        alpha lead = ec_itoa(poly[s]);

        for (int g = 0; g < gn; g++) {
            poly[s + g] ^= ec_atoi(lead + generator[g]); 
        }
    }

    for (int i = 0; i < ecwords; i++) {
        writeTo[i] = (byte) poly[mn + i];
    }

    free(poly);
    return ecwords;
}

int applyErrorCorrection(byte *encoding, int dataSize) {
    int ecwords;
    int level;
    alpha *generator;
    if (dataSize <= DATA_SIZE_H) {
        ecwords = ENCODING_SIZE - DATA_SIZE_H;
        level = 2;
        generator = EC_GEN_H;
    } else if (dataSize <= DATA_SIZE_Q) {
        ecwords = ENCODING_SIZE - DATA_SIZE_Q;
        level = 3;
        generator = EC_GEN_Q;
    } else if (dataSize <= DATA_SIZE_M) {
        ecwords = ENCODING_SIZE - DATA_SIZE_M;
        level = 0;
        generator = EC_GEN_M;
    } else {
        ecwords = ENCODING_SIZE - DATA_SIZE_L;
        level = 1;
        generator = EC_GEN_L;
    }

    int pad = ENCODING_SIZE - ecwords - dataSize;
    for (int i = 0; i < pad; i++) {
        if (i & 1) {
            encoding[dataSize + i] = 0b11101100;
        } else {
            encoding[dataSize + i] = 0b00010001;
        }
    }

    dataSize += pad;
    generateEC(encoding, dataSize, encoding + dataSize, ecwords, generator);

    return level;
}

byte **dupmat(byte **original) {
    byte **matrix = malloc(M_SIZE * sizeof(byte*));
    for (int i = 0; i < M_SIZE; i++) {
        matrix[i] = malloc(M_SIZE * sizeof(byte));
        memcpy(matrix[i], original[i], M_SIZE);
    }

    return matrix;
}

void freeMatrix(byte **matrix, int size) {
    for (int i = 0; i < size; i++) free(matrix[i]);
    free(matrix);
}

int scoreMask(byte** matrix) {
    int score = 0;

    // Scoring rows
    byte b;
    int consecutive;
    for (int y = 0; y < M_SIZE; y++) {
        consecutive = 1;
        b = matrix[y][0];

        for (int x = 1; x < M_SIZE; x++) {
            if (matrix[y][x] == b) {
                consecutive++;
            } else {
                if (consecutive >= 5) {
                    score += MASK_N1 + consecutive;
                }
                consecutive = 1;
            }

            b = matrix[y][x];
        }
    }

    // Scoring columns
    for (int x = 0; x < M_SIZE; x++) {
        consecutive = 1;
        b = matrix[0][x];

        for (int y = 1; y < M_SIZE; y++) {
            if (matrix[y][x] == b) {
                consecutive++;
            } else {
                if (consecutive >= 5) {
                    score += MASK_N1 + consecutive;
                }
                consecutive = 1;
            }

            b = matrix[y][x];
        }
    }

    // Scoring 2x2 blocks
    for (int y = 0; y < M_SIZE - 1; y++) {
        for (int x = 0; x < M_SIZE - 1; x++) {
            b = matrix[y][x];
            if (matrix[y + 1][x] == b && matrix[y][x + 1] == b && matrix[y + 1][x + 1] == b) {
                score += MASK_N2;
            }
        }
    }

    // Scoring fake finder patterns
    static const byte pattern[] = { 1,0,1,1,1,0,1 };
    static const byte lightArea[] = { 0,0,0,0 };
    // Rows
    for (int y = 0; y < M_SIZE; y++) {
        for (int x = 0; x < M_SIZE - 7; x++) {
            int match = 1;
            for (int i = 0; i < 7; i++) {
                if (pattern[i] != matrix[y][x + i]) {
                    match = 0;
                    break;
                }
            }

            if (!match) continue;

            if (x + 7 + 4 < M_SIZE && !memcmp(matrix[y] + x + 7, lightArea, 4)){
                score += MASK_N3;
            }

            if (x - 4 >= 0 && !memcmp(matrix[y] + x - 4, lightArea, 4)){
                score += MASK_N3;
            }
        }
    }
    // Columns
    for (int x = 0; x < M_SIZE; x++) {
        for (int y = 0; y < M_SIZE - 7; y++) {
            int match = 1;
            for (int i = 0; i < 7; i++) {
                if (pattern[i] != matrix[y + i][x]) {
                    match = 0;
                    break;
                }
            }

            if (!match) continue;

            if (y + 7 + 4 < M_SIZE){
                match = 1;
                for (int i = 0; i < 4; i++) {
                    if (matrix[y + 7 + i][x]) {
                        match = 0;
                        break;
                    }
                }

                if (match) {
                    score += MASK_N3;
                }
            }

            if (y - 4 >= 0){
                match = 1;
                for (int i = 0; i < 4; i++) {
                    if (matrix[y - 4 + i][x]) {
                        match = 0;
                        break;
                    }
                }

                if (match) {
                    score += MASK_N3;
                }
            }
        }
    }

    // Scoring dark-light ratio
    int darkCount = 0;
    for (int y = 0; y < M_SIZE; y++) {
        for (int x = 0; x < M_SIZE; x++) {
            darkCount += matrix[y][x] != 0;
        }
    }
    double ratio = darkCount / (double)(M_SIZE * M_SIZE);
    int k = (int) (fabs(ratio - 0.5d) / 0.05d);
    score += k * MASK_N4;

    return score;
}

int applyDataMasking(byte **matrix) {
    byte **masks[8];

    for (int i = 0; i < 8; i++) {
        masks[i] = dupmat(matrix);
    }

    for (int y = 0; y < M_SIZE; y++) {
        for (int x = 0; x < M_SIZE; x++) {
            if(!DATA_MASK[y][x]) continue;

            masks[0][y][x] ^= (y + x) % 2 == 0;
            masks[1][y][x] ^= y % 2 == 0;
            masks[2][y][x] ^= x % 3 == 0;
            masks[3][y][x] ^= (y + x) % 3 == 0;
            masks[4][y][x] ^= (y / 2 + x / 3) % 2 == 0;
            masks[5][y][x] ^= (y * x) % 2 + (y * x) % 3 == 0;
            masks[6][y][x] ^= ((y * x) % 2 + (y * x) % 3) % 2 == 0;
            masks[7][y][x] ^= ((y + x) % 2 + (y * x) % 3) % 2 == 0;
        }
    }

    int chosen = 0;
    int minScore = scoreMask(masks[0]);  // Choosing the one with lowest score (penalty)

    for (int i = 1; i < 8; i++) {
        int score = scoreMask(masks[i]);
        if (score < minScore) {
            chosen = i;
            minScore = score;
        }
    }

    chosen = 2;

    for (int y = 0; y < M_SIZE; y++) {
        for (int x = 0; x < M_SIZE; x++) {
            matrix[y][x] = masks[chosen][y][x];
        }
    }

    for (int i = 0; i < 8; i++) {
        freeMatrix(masks[i], M_SIZE);
    }

    return chosen;
}

void writeFormating(byte **matrix, int ecLevel, int dataMask) {
    byte bitmap[15];

    bitmap[0] = (ecLevel >> 1) & 1;
    bitmap[1] = (ecLevel >> 0) & 1;

    bitmap[2] = (dataMask >> 2) & 1;
    bitmap[3] = (dataMask >> 1) & 1;
    bitmap[4] = (dataMask >> 0) & 1;

    //Error correction
    static const byte gen[11] = { 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1 };
    byte ecpoly[15];
    for (int i = 0; i < 15; i++) {
        ecpoly[i] = i < 5 ? bitmap[i] : 0;
    }

    for (int i = 0; i < 5; i++) {
        if(ecpoly[i] == 0) continue;

        for (int g = 0; g < 11; g++) {
            ecpoly[i + g] ^= gen[g];
        }
    }

    memcpy(bitmap + 5, ecpoly + 5, 10);

    // Masking
    for (int i = 0; i < 15; i++) {
        bitmap[i] ^= (21522 >> (14 - i)) & 1;
    }

    // Writing to matrix
    for (int y = 0; y < M_SIZE; y++) {
        for (int x = 0; x < M_SIZE; x++) {
            if (FORMATTING_MASK[y][x] == 0) continue;

            matrix[y][x] = bitmap[15 - FORMATTING_MASK[y][x]];
        }
    }
}

QRCode *generateQR(char* data, int n) {
    byte **matrix = malloc(M_SIZE * sizeof(byte*));
    for (int i = 0; i < M_SIZE; i++) {
        matrix[i] = calloc(M_SIZE, sizeof(byte));
    }

    applyMaskS(matrix, STATIC_MASK);

    QRCode *qr = NULL;
    byte *encoding = calloc(ENCODING_SIZE, sizeof(byte));

    // Max data size up to L
    int wordsWritten = encodeData(encoding, DATA_SIZE_L, data, n);
    if (wordsWritten > 0) {
        int ecLevel = applyErrorCorrection(encoding, wordsWritten);

        packData(matrix, encoding, ENCODING_SIZE);

        int dataMask = applyDataMasking(matrix);

        writeFormating(matrix, ecLevel, dataMask);

        byte **upscaled = upscale(matrix, M_SIZE, M_SIZE, M_RATE);
        qr = toQRImage(upscaled, M_SIZE * M_RATE, M_SIZE * M_RATE);

        freeMatrix(upscaled, M_SIZE * M_RATE);
    }

    freeMatrix(matrix, M_SIZE);
    free(encoding);

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
