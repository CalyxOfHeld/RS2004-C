#include "config/flo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void flo_unpack(FloConfig* config, Jagfile* jag) {
    size_t len = 0;
    uint8_t* data = jagfile_read(jag, "flo.dat", NULL, &len, NULL);
    if (!data) {
        config->count = 0;
        config->instances = NULL;
        return;
    }

    Packet pkt = packet_wrap(data, len);

    config->count = g2(&pkt);
    config->instances = (FloType*)calloc(config->count, sizeof(FloType));

    for (int i = 0; i < config->count; i++) {
        FloType* flo = &config->instances[i];
        flo->texture = -1;
        flo->occlude = true;
        flo_decode(flo, &pkt);
    }

    free(data);
}

void flo_decode(FloType* flo, Packet* pkt) {
    while (1) {
        int code = g1(pkt);
        if (code == 0) {
            return;
        }

        if (code == 1) {
            flo->rgb = g3(pkt);
            flo_set_color(flo, flo->rgb);
        } else if (code == 2) {
            flo->texture = g1(pkt);
        } else if (code == 3) {
            flo->overlay = true;
        } else if (code == 5) {
            flo->occlude = false;
        } else if (code == 6) {
            gjstr(pkt, flo->name, sizeof(flo->name));
        } else {
            fprintf(stderr, "Error unrecognised flo config code: %d\n", code);
        }
    }
}

void flo_set_color(FloType* flo, int rgb) {
    double r = (double)((rgb >> 16) & 0xFF) / 256.0;
    double g = (double)((rgb >> 8) & 0xFF) / 256.0;
    double b = (double)(rgb & 0xFF) / 256.0;

    double min = r;
    if (g < min) min = g;
    if (b < min) min = b;

    double max = r;
    if (g > max) max = g;
    if (b > max) max = b;

    double h = 0.0;
    double s = 0.0;
    double l = (min + max) / 2.0;

    if (min != max) {
        if (l < 0.5) {
            s = (max - min) / (max + min);
        } else {
            s = (max - min) / (2.0 - max - min);
        }

        if (r == max) {
            h = (g - b) / (max - min);
        } else if (g == max) {
            h = (b - r) / (max - min) + 2.0;
        } else if (b == max) {
            h = (r - g) / (max - min) + 4.0;
        }
    }

    h /= 6.0;
    flo->hue = (int)(h * 256.0);
    flo->saturation = (int)(s * 256.0);
    flo->lightness = (int)(l * 256.0);

    if (flo->saturation < 0) flo->saturation = 0;
    else if (flo->saturation > 255) flo->saturation = 255;

    if (flo->lightness < 0) flo->lightness = 0;
    else if (flo->lightness > 255) flo->lightness = 255;

    if (l > 0.5) {
        flo->luminance = (int)((1.0 - l) * s * 512.0);
    } else {
        flo->luminance = (int)(l * s * 512.0);
    }

    if (flo->luminance < 1) flo->luminance = 1;
    flo->chroma = (int)(h * (double)flo->luminance);

    int var19 = flo->hue + (int)(((double)rand() / (double)RAND_MAX) * 16.0) - 8;
    if (var19 < 0) var19 = 0;
    else if (var19 > 255) var19 = 255;

    int var20 = flo->saturation + (int)(((double)rand() / (double)RAND_MAX) * 48.0) - 24;
    if (var20 < 0) var20 = 0;
    else if (var20 > 255) var20 = 255;

    int var21 = flo->lightness + (int)(((double)rand() / (double)RAND_MAX) * 48.0) - 24;
    if (var21 < 0) var21 = 0;
    else if (var21 > 255) var21 = 255;

    flo->hsl = flo_hsl24to16(var19, var20, var21);
}

int flo_hsl24to16(int h, int s, int l) {
    if (l > 179) s /= 2;
    if (l > 192) s /= 2;
    if (l > 217) s /= 2;
    if (l > 243) s /= 2;
    return ((h / 4) << 10) + ((s / 32) << 7) + (l / 2);
}

void flo_free(FloConfig* config) {
    if (config->instances) {
        free(config->instances);
        config->instances = NULL;
    }
    config->count = 0;
}
