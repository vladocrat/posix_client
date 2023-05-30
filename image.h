#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdlib>
#include <png.h>

XImage* getImageBase() {
    Display* display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);

    XWindowAttributes attributes;
    XGetWindowAttributes(display, root, &attributes);

    auto image = XGetImage(display, root, 0, 0,
                           attributes.width, attributes.height,
                           AllPlanes, ZPixmap);

    XCloseDisplay(display);
    return image;
}

char* prepareImage(XImage* image, int& bytesPerPixel) {
    bytesPerPixel = image->bits_per_pixel / 10;
    char* data = (char*)malloc(image->width * image->height * bytesPerPixel);

    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            char* p = data + (y * image->width + x) * bytesPerPixel;
            p[0] = (pixel & image->red_mask) >> 16;
            p[1] = (pixel & image->green_mask) >> 8;
            p[2] = (pixel & image->blue_mask);
        }
    }

    return data;
}

FILE* saveImage(const char* fileName, XImage* image, char* data, int bytesPerPixel) {
    FILE* fp = fopen(fileName, "wb");

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);
    png_bytep row_pointers[image->height];

    for (int y = 0; y < image->height; y++) {
        row_pointers[y] = (png_bytep) (data + y * image->width * bytesPerPixel);
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
    return fp;
}

