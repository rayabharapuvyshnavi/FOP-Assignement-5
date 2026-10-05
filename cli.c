#include "kernel.h"
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

int generate_pagefault() {
    struct image* src = malloc(sizeof(struct image));
    if (src == NULL) {
        return -1;
    }

    src->width = 4096;
    src->height = 4096;
    src->pixels = calloc((size_t)src->width * (size_t)src->height, sizeof(struct pixel));
    if (src->pixels == NULL) {
        free(src);
        return -1;
    }

    for (int i = 0; i < src->width * src->height; i++) {
        src->pixels[i].r = (i * 17) % 256;
        src->pixels[i].g = (i * 29) % 256;
        src->pixels[i].b = (i * 31) % 256;
    }

    saveimage_mmap("fault.bin", src);

    struct image* mapped = malloc(sizeof(struct image));
    if (mapped == NULL) {
        free(src->pixels);
        free(src);
        return -1;
    }

    mapped->width = src->width;
    mapped->height = src->height;
    int rc = loadimage_mmap("fault.bin", mapped);
    if (rc != 0) {
        free(mapped);
        free(src->pixels);
        free(src);
        return -1;
    }

    volatile unsigned long long sum = 0;
    for (int i = 0; i < mapped->width * mapped->height; i++) {
        sum += mapped->pixels[i].r + mapped->pixels[i].g + mapped->pixels[i].b;
    }

    size_t mapped_size = (size_t)sizeof(struct image) + (size_t)mapped->width * (size_t)mapped->height * sizeof(struct pixel);
    munmap((void*)((char*)mapped->pixels - sizeof(struct image)), mapped_size);
    free(mapped);
    free(src->pixels);
    free(src);
    remove("fault.bin");
    return (int)sum;
}

int main(int argc, char** argv){
    if(argc != 6) {
        printf("Incorrect number of arguments. Expected: ./cli <MODE=kernel|mmap|convert|uconvert|fault> <input_image> <width> <height> <output_image_path>\n");
        return -1;
    }

    char* mode = argv[1];
    char* filepath = argv[2];
    int width = atoi(argv[3]);
    int height = atoi(argv[4]);
    char* outpath = argv[5];
    int kernel[3][3] = {{1,1,1},{1,1,1},{1,1,1}};

    if (strcmp(mode, "fault") == 0) {
        generate_pagefault();
        return 0;
    }

    struct image* image = calloc(1, sizeof(struct image));
    if (image == NULL) {
        return -1;
    }

    image->width = width;
    image->height = height;

    if (strcmp(mode, "kernel") == 0) {
        if (loadimage(filepath, image) != 0) {
            free(image);
            return -1;
        }

        struct image* out = apply_kernel(image, (int*)kernel, 3, 1.0f / 9.0f);
        if (out == NULL) {
            free(image->pixels);
            free(image);
            return -1;
        }

        saveimage(outpath, out);

        free(image->pixels);
        free(image);
        free(out->pixels);
        free(out);
        return 0;
    }

    if (strcmp(mode, "convert") == 0) {
        if (loadimage(filepath, image) != 0) {
            free(image);
            return -1;
        }

        int rc = saveimage_mmap(outpath, image);
        free(image->pixels);
        free(image);
        return rc == 0 ? 0 : -1;
    }

    if (strcmp(mode, "uconvert") == 0) {
        if (loadimage_mmap(filepath, image) != 0) {
            free(image);
            return -1;
        }

        int rc = saveimage(outpath, image);
        size_t mapped_size = (size_t)sizeof(struct image) + (size_t)image->width * (size_t)image->height * sizeof(struct pixel);
        munmap((void*)((char*)image->pixels - sizeof(struct image)), mapped_size);
        free(image);
        return rc == 0 ? 0 : -1;
    }

    if (strcmp(mode, "mmap") == 0) {
        if (loadimage_mmap(filepath, image) != 0) {
            free(image);
            return -1;
        }

        struct image* out = apply_kernel(image, (int*)kernel, 3, 1.0f / 9.0f);
        if (out == NULL) {
            size_t mapped_size = (size_t)sizeof(struct image) + (size_t)image->width * (size_t)image->height * sizeof(struct pixel);
            munmap((void*)((char*)image->pixels - sizeof(struct image)), mapped_size);
            free(image);
            return -1;
        }

        int rc = saveimage_mmap(outpath, out);
        size_t mapped_size = (size_t)sizeof(struct image) + (size_t)image->width * (size_t)image->height * sizeof(struct pixel);
        munmap((void*)((char*)image->pixels - sizeof(struct image)), mapped_size);
        free(image);
        free(out->pixels);
        free(out);
        return rc == 0 ? 0 : -1;
    }

    printf("Unknown mode: %s\n", mode);
    free(image);
    return -1;
}
