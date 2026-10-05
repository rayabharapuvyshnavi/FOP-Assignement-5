#include "loader.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/** Returns p1 with each channel multiplied by scalar. */
struct pixel mul(struct pixel p1, float scalar) {
    return (struct pixel){r: p1.r * scalar, g: p1.g * scalar, b: p1.b * scalar};
}
/** Returns the channel-wise sum of p1 and p2. */
struct pixel add(struct pixel p1, struct pixel p2) {
    return (struct pixel){r: p1.r + p2.r, g: p1.g + p2.g, b: p1.b + p2.b};
}

/**
 * Applies a square kernel to an image (cross-correlation).
 *
 * Produces a new image where each output pixel is the weighted sum of
 * the ksize x ksize neighborhood centered on the corresponding input
 * pixel, multiplied by normalize. The kernel is applied as-is (not
 * flipped), so this is technically cross-correlation; the result is
 * identical to convolution for symmetric kernels.
 *
 * The input img is padded so that kernel operations that fall outside of the 
 * original image are multiplied by a black pixel (zero padding).
 *
 * img        Source image. Not modified.
 * kernel     Kernel weights in row-major order, containing ksize * ksize elements.
 * ksize      Width and height of the kernel. Should be odd
 * normalize  Scale factor applied to each weighted sum
 *                       (e.g., 1.0f / 9 for a 3x3 box blur).
 *
 * Returns a pointer to a newly allocated image with the same dimensions as img.
 *
 */
struct image* apply_kernel(struct image* img, int* kernel, int ksize, float normalize) {
    if (img == NULL || kernel == NULL || ksize <= 0 || (ksize % 2) == 0) {
        return NULL;
    }

    int radius = ksize / 2;
    struct image* out = malloc(sizeof(struct image));
    if (out == NULL) {
        return NULL;
    }

    out->width = img->width;
    out->height = img->height;
    out->pixels = calloc((size_t)img->width * (size_t)img->height, sizeof(struct pixel));
    if (out->pixels == NULL) {
        free(out);
        return NULL;
    }

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int sum_r = 0;
            int sum_g = 0;
            int sum_b = 0;

            for (int ky = 0; ky < ksize; ky++) {
                for (int kx = 0; kx < ksize; kx++) {
                    int img_x = x + kx - radius;
                    int img_y = y + ky - radius;
                    struct pixel source = {0, 0, 0};

                    if (img_x >= 0 && img_x < img->width && img_y >= 0 && img_y < img->height) {
                        source = img->pixels[img_x + img_y * img->width];
                    }

                    int weight = kernel[ky * ksize + kx];
                    sum_r += source.r * weight;
                    sum_g += source.g * weight;
                    sum_b += source.b * weight;
                }
            }

            struct pixel result = {
                .r = (int)lrintf((float)sum_r * normalize),
                .g = (int)lrintf((float)sum_g * normalize),
                .b = (int)lrintf((float)sum_b * normalize)
            };

            out->pixels[x + y * img->width] = result;
        }
    }

    return out;
}

