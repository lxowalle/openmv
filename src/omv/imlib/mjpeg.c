/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2023 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2023 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * A simple MJPEG encoder.
 */
#include "imlib.h"
#if defined(IMLIB_ENABLE_IMAGE_FILE_IO)

#include "file_utils.h"

#define SIZE_OFFSET        (1 * 4)
#define MICROS_OFFSET      (8 * 4)
#define FRAMES_OFFSET      (12 * 4)
#define RATE_0_OFFSET      (19 * 4)
#define LENGTH_0_OFFSET    (21 * 4)
#define RATE_1_OFFSET      (33 * 4)
#define LENGTH_1_OFFSET    (35 * 4)
#define MOVI_OFFSET        (54 * 4)

void mjpeg_open(FIL *fp, int width, int height) {
    file_write(fp, "RIFF", 4); // FOURCC fcc; - 0
    file_write_long(fp, 0); // DWORD cb; size - updated on close - 1
    file_write(fp, "AVI ", 4); // FOURCC fcc; - 2

    file_write(fp, "LIST", 4); // FOURCC fcc; - 3
    file_write_long(fp, 192); // DWORD cb; - 4
    file_write(fp, "hdrl", 4); // FOURCC fcc; - 5

    file_write(fp, "avih", 4); // FOURCC fcc; - 6
    file_write_long(fp, 56); // DWORD cb; - 7
    file_write_long(fp, 0); // DWORD dwMicroSecPerFrame; micros - updated on close - 8
    file_write_long(fp, 0); // DWORD dwMaxBytesPerSec; updated on close - 9
    file_write_long(fp, 4); // DWORD dwPaddingGranularity; - 10
    file_write_long(fp, 0); // DWORD dwFlags; - 11
    file_write_long(fp, 0); // DWORD dwTotalFrames; frames - updated on close - 12
    file_write_long(fp, 0); // DWORD dwInitialFrames; - 13
    file_write_long(fp, 1); // DWORD dwStreams; - 14
    file_write_long(fp, 0); // DWORD dwSuggestedBufferSize; - 15
    file_write_long(fp, width); // DWORD dwWidth; - 16
    file_write_long(fp, height); // DWORD dwHeight; - 17
    file_write_long(fp, 1000); // DWORD dwScale; - 18
    file_write_long(fp, 0); // DWORD dwRate; rate - updated on close - 19
    file_write_long(fp, 0); // DWORD dwStart; - 20
    file_write_long(fp, 0); // DWORD dwLength; length - updated on close - 21

    file_write(fp, "LIST", 4); // FOURCC fcc; - 22
    file_write_long(fp, 116); // DWORD cb; - 23
    file_write(fp, "strl", 4); // FOURCC fcc; - 24

    file_write(fp, "strh", 4); // FOURCC fcc; - 25
    file_write_long(fp, 56); // DWORD cb; - 26
    file_write(fp, "vids", 4); // FOURCC fccType; - 27
    file_write(fp, "MJPG", 4); // FOURCC fccHandler; - 28
    file_write_long(fp, 0); // DWORD dwFlags; - 29
    file_write_short(fp, 0); // WORD wPriority; - 30
    file_write_short(fp, 0); // WORD wLanguage; - 30.5
    file_write_long(fp, 0); // DWORD dwInitialFrames; - 31
    file_write_long(fp, 1000); // DWORD dwScale; - 32
    file_write_long(fp, 0); // DWORD dwRate; rate - updated on close - 33
    file_write_long(fp, 0); // DWORD dwStart; - 34
    file_write_long(fp, 0); // DWORD dwLength; length - updated on close - 35
    file_write_long(fp, 0); // DWORD dwSuggestedBufferSize; - 36
    file_write_long(fp, 10000); // DWORD dwQuality; - 37
    file_write_long(fp, 0); // DWORD dwSampleSize; - 38
    file_write_short(fp, 0); // short int left; - 39
    file_write_short(fp, 0); // short int top; - 39.5
    file_write_short(fp, 0); // short int right; - 40
    file_write_short(fp, 0); // short int bottom; - 40.5

    file_write(fp, "strf", 4); // FOURCC fcc; - 41
    file_write_long(fp, 40); // DWORD cb; - 42
    file_write_long(fp, 40); // DWORD biSize; - 43
    file_write_long(fp, width); // DWORD biWidth; - 44
    file_write_long(fp, height); // DWORD biHeight; - 45
    file_write_short(fp, 1); // WORD biPlanes; - 46
    file_write_short(fp, 24); // WORD biBitCount; - 46.5
    file_write(fp, "MJPG", 4); // DWORD biCompression; - 47
    file_write_long(fp, 0); // DWORD biSizeImage; - 48
    file_write_long(fp, 0); // DWORD biXPelsPerMeter; - 49
    file_write_long(fp, 0); // DWORD biYPelsPerMeter; - 50
    file_write_long(fp, 0); // DWORD biClrUsed; - 51
    file_write_long(fp, 0); // DWORD biClrImportant; - 52

    file_write(fp, "LIST", 4); // FOURCC fcc; - 53
    file_write_long(fp, 0); // DWORD cb; movi - updated on close - 54
    file_write(fp, "movi", 4); // FOURCC fcc; - 55
}

void mjpeg_write(FIL *fp, int width, int height, uint32_t *frames, uint32_t *bytes,
                 image_t *img, int quality, rectangle_t *roi, int rgb_channel, int alpha,
                 const uint16_t *color_palette, const uint8_t *alpha_palette, image_hint_t hint) {
    float xscale = width / ((float) roi->w);
    float yscale = height / ((float) roi->h);
    // MAX == KeepAspectRationByExpanding - MIN == KeepAspectRatio
    float scale = IM_MIN(xscale, yscale);

    image_t dst_img = {
        .w = width,
        .h = height,
        .pixfmt = PIXFORMAT_JPEG,
        .size = 0,
        .data = NULL
    };

    bool simple = (xscale == 1) &&
                  (yscale == 1) &&
                  (roi->x == 0) &&
                  (roi->y == 0) &&
                  (roi->w == img->w) &&
                  (roi->h == img->h) &&
                  (rgb_channel == -1) &&
                  (alpha == 256) &&
                  (color_palette == NULL) &&
                  (alpha_palette == NULL);

    fb_alloc_mark();

    if ((dst_img.pixfmt != img->pixfmt) || (!simple)) {
        image_t temp;
        memcpy(&temp, img, sizeof(image_t));

        if (img->is_compressed || (!simple)) {
            temp.w = dst_img.w;
            temp.h = dst_img.h;
            temp.pixfmt = PIXFORMAT_RGB565; // TODO PIXFORMAT_ARGB8888
            temp.size = 0;
            temp.data = fb_alloc(image_size(&temp), FB_ALLOC_NO_HINT);

            int center_x = fast_floorf((width - (roi->w * scale)) / 2);
            int center_y = fast_floorf((height - (roi->h * scale)) / 2);

            point_t p0, p1;
            imlib_draw_image_get_bounds(&temp, img, center_x, center_y, scale, scale, roi,
                                        alpha, alpha_palette, hint, &p0, &p1);
            bool black = p0.x == -1;

            if (black) {
                // zero the whole image
                memset(temp.data, 0, temp.w * temp.h * sizeof(uint16_t));
            } else {
                // Zero the top rows
                if (p0.y) {
                    memset(temp.data, 0, temp.w * p0.y * sizeof(uint16_t));
                }

                if (p0.x) {
                    for (int i = p0.y; i < p1.y; i++) {
                        // Zero left
                        memset(IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&temp, i), 0, p0.x * sizeof(uint16_t));
                    }
                }

                imlib_draw_image(&temp, img, center_x, center_y, scale, scale, roi,
                                 rgb_channel, alpha, color_palette, alpha_palette,
                                 (hint & (~IMAGE_HINT_CENTER)) | IMAGE_HINT_BLACK_BACKGROUND,
                                 NULL, NULL);

                if (temp.w - p1.x) {
                    for (int i = p0.y; i < p1.y; i++) {
                        // Zero right
                        memset(IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&temp, i) + p1.x,
                               0, (temp.w - p1.x) * sizeof(uint16_t));
                    }
                }

                // Zero the bottom rows
                if (temp.h - p1.y) {
                    memset(IMAGE_COMPUTE_RGB565_PIXEL_ROW_PTR(&temp, p1.y),
                           0, temp.w * (temp.h - p1.y) * sizeof(uint16_t));
                }
            }
        }

        // When jpeg_compress needs more memory than in currently allocated it
        // will try to realloc. MP will detect that the pointer is outside of
        // the heap and return NULL which will cause an out of memory error.
        jpeg_compress(&temp, &dst_img, quality, true);
    } else {
        dst_img.size = img->size;
        dst_img.data = img->data;
    }

    uint32_t size_padded = (((dst_img.size + 3) / 4) * 4);
    file_write(fp, "00dc", 4); // FOURCC fcc;
    file_write_long(fp, size_padded); // DWORD cb;
    file_write(fp, dst_img.data, size_padded); // reading past okay

    *frames += 1;
    *bytes += size_padded;

    fb_alloc_free_till_mark();
}

void mjpeg_sync(FIL *fp, uint32_t *frames, uint32_t *bytes, float fps) {
    uint32_t position = f_tell(fp);
    // Needed
    file_seek(fp, SIZE_OFFSET);
    file_write_long(fp, 216 + (*frames * 8) + *bytes);
    // Needed
    file_seek(fp, MICROS_OFFSET);
    file_write_long(fp, (!fast_roundf(fps)) ? 0 :
                    fast_roundf(1000000 / fps));
    file_write_long(fp, (!(*frames)) ? 0 :
                    fast_roundf((((*frames * 8) + *bytes) * fps) / *frames));
    // Needed
    file_seek(fp, FRAMES_OFFSET);
    file_write_long(fp, *frames);
    // Probably not needed but writing it just in case.
    file_seek(fp, RATE_0_OFFSET);
    file_write_long(fp, fast_roundf(fps * 1000));
    // Probably not needed but writing it just in case.
    file_seek(fp, LENGTH_0_OFFSET);
    file_write_long(fp, (!fast_roundf(fps)) ? 0 :
                    fast_roundf((*frames * 1000) / fps));
    // Probably not needed but writing it just in case.
    file_seek(fp, RATE_1_OFFSET);
    file_write_long(fp, fast_roundf(fps * 1000));
    // Probably not needed but writing it just in case.
    file_seek(fp, LENGTH_1_OFFSET);
    file_write_long(fp, (!fast_roundf(fps)) ? 0 :
                    fast_roundf((*frames * 1000) / fps));
    // Needed
    file_seek(fp, MOVI_OFFSET);
    file_write_long(fp, 4 + (*frames * 8) + *bytes);
    file_sync(fp);
    file_seek(fp, position);
}

void mjpeg_close(FIL *fp, uint32_t *frames, uint32_t *bytes, float fps) {
    mjpeg_sync(fp, frames, bytes, fps);
    file_close(fp);
}

#endif // IMLIB_ENABLE_IMAGE_FILE_IO
