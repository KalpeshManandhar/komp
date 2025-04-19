#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_ITER 1000

// Write a little-endian 4-byte int
void write_le_uint32(FILE *f, uint32_t val) {
    fputc(val & 0xFF, f);
    fputc((val >> 8) & 0xFF, f);
    fputc((val >> 16) & 0xFF, f);
    fputc((val >> 24) & 0xFF, f);
}

// Write a little-endian 2-byte int
void write_le_uint16(FILE *f, uint16_t val) {
    fputc(val & 0xFF, f);
    fputc((val >> 8) & 0xFF, f);
}

void write_bmp_header(FILE *f, int width, int height) {
    int row_size = (3 * width + 3) & ~3;
    int pixel_array_size = row_size * height;
    int file_size = 54 + pixel_array_size;

    // BMP Header
    fputc('B', f);
    fputc('M', f);
    write_le_uint32(f, file_size);
    write_le_uint32(f, 0); // reserved
    write_le_uint32(f, 54); // pixel data offset

    // DIB Header
    write_le_uint32(f, 40); // DIB header size
    write_le_uint32(f, width);
    write_le_uint32(f, height);
    write_le_uint16(f, 1); // planes
    write_le_uint16(f, 24); // bits per pixel
    write_le_uint32(f, 0); // no compression
    write_le_uint32(f, pixel_array_size);
    write_le_uint32(f, 2835); // print resolution X
    write_le_uint32(f, 2835); // print resolution Y
    write_le_uint32(f, 0); // palette colors
    write_le_uint32(f, 0); // important colors
}

int mandelbrot(double x0, double y0) {
    double x = 0, y = 0;
    int iter = 0;
    while (x*x + y*y <= 4.0 && iter < MAX_ITER) {
        double xtemp = x*x - y*y + x0;
        y = 2*x*y + y0;
        x = xtemp;
        iter++;
    }
    return iter;
}

int main() {
    FILE *f = fopen("mandelbrot.bmp", "wb");

    write_bmp_header(f, WIDTH, HEIGHT);

    int row_size = (3 * WIDTH + 3) & ~3;
    uint8_t *row = (uint8_t*) malloc(row_size);

    int j;
    for (j = HEIGHT - 1; j >= 0; --j) {
        int i;
        for (i = 0; i < WIDTH; ++i) {
            double x0 = (i - WIDTH / 2.0) * 4.0 / WIDTH;
            double y0 = (j - HEIGHT / 2.0) * 4.0 / WIDTH;

            int iter = mandelbrot(x0, y0);
            uint8_t r = (iter * 9) % 256;
            uint8_t g = (iter * 5) % 256;
            uint8_t b = (iter * 3) % 256;

            row[i * 3 + 0] = b;
            row[i * 3 + 1] = g;
            row[i * 3 + 2] = r;
        }

        // Padding
        for (i = 3 * WIDTH; i < row_size; ++i) {
            row[i] = 0;
        }

        fwrite(row, 1, row_size, f);
    }

    fclose(f);
    return 0;
}
