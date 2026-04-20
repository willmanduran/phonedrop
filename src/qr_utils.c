#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "qrcodegen.h"
#include "qr_utils.h"

void generate_terminal_qr(const char *text) {
    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl,
                                   qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                   qrcodegen_Mask_AUTO, true);

    if (ok) {
        int size = qrcodegen_getSize(qrcode);
        int border = 2;

        printf("\n");
        for (int y = -border; y < size + border; y++) {
            for (int x = -border; x < size + border; x++) {
                fputs((qrcodegen_getModule(qrcode, x, y) ? "██" : "  "), stdout);
            }
            fputs("\n", stdout);
        }
    } else {
        fprintf(stderr, "Error: No se pudo generar el QR.\n");
    }
}