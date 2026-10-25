/*
 * NV2A (Xbox) texture swizzling
 *
 * Copyright (c) 2026 nxdk-gles11 contributors
 *
 * The Z-order (Morton) bit interleaving and the mask-increment technique used
 * here originate from the MIT licensed Nouveau/Mesa nv30 texture code:
 *   Copyright 2007-2010 The Nouveau Project
 *   Copyright 2012 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

#include "swizzle.h"

/*
 * Swizzled textures store pixels in Z-order (Morton order) instead of
 * row-major order, which improves texture cache locality on the NV2A.
 * The swizzled offset of a pixel is the bit interleaving of its texture
 * coordinates, starting with the least significant bit of each axis:
 *
 *     x0 y0 z0 x1 y1 z1 x2 y2 z2 ...
 *
 * Width, height and depth are expected to be powers of two (the GLES driver
 * only swizzles power-of-two textures). The functions below also accept other
 * sizes and match the historical QEMU/Nouveau behaviour for those.
 *
 * Instead of moving one pixel at a time, the fast path exploits the structure
 * of the curve: four consecutive swizzled pixels always form one 2x2 block of
 * the source image (x0 selects the column, y0 the row within the block).
 * Stepping through the image in 2x2 blocks therefore only needs one
 * mask-increment per four pixels, and each row of a block is a contiguous
 * 2*bytes_per_pixel copy. This matters on the Xbox because nxdk builds with
 * -fno-builtin, so a call to memcpy() is not inlined while __builtin_memcpy()
 * with a constant size is.
 *
 * See also: https://en.wikipedia.org/wiki/Z-order_curve
 */

/*
 * Create masks representing the interleaving of each linear texture
 * dimension (x, y, z). For a 2D 8x32 texture (3 bits for x, 5 bits for y):
 *
 *     mask_x: 00010101
 *     mask_y: 11101010
 *     mask_z: 00000000
 *
 * for "Z": yyyxyxyx
 */
static void generate_swizzle_masks(
    unsigned int width, unsigned int height, unsigned int depth, uint32_t *mask_x, uint32_t *mask_y, uint32_t *mask_z)
{
    uint32_t x = 0, y = 0, z = 0;
    uint32_t bit = 1;
    uint32_t mask_bit = 1;
    bool done;
    do {
        done = true;
        if (bit < width) {
            x |= mask_bit;
            mask_bit <<= 1;
            done = false;
        }
        if (bit < height) {
            y |= mask_bit;
            mask_bit <<= 1;
            done = false;
        }
        if (bit < depth) {
            z |= mask_bit;
            mask_bit <<= 1;
            done = false;
        }
        bit <<= 1;
    } while (!done);
    assert((x ^ y ^ z) == (mask_bit - 1)); /* masks are mutually exclusive */
    *mask_x = x;
    *mask_y = y;
    *mask_z = z;
}

/*
 * Fast 2D path, one instance per common pixel size.
 *
 * A 2x2 source block maps to four consecutive pixels in the swizzled buffer
 * (in the order top-left, top-right, bottom-left, bottom-right). At block
 * granularity the same Z-order rule applies, but the lowest mask bit of each
 * axis is now fixed by the pixel position inside the block, so the block
 * offsets advance with the mask reduced by its lowest set bit.
 */
#define DEFINE_SWIZZLE_2D(BPP)                                                                                         \
    static void swizzle_2d_##BPP(                                                                                      \
        const uint8_t *src, unsigned int width, unsigned int height, uint8_t *dst, unsigned int src_pitch)             \
    {                                                                                                                  \
        uint32_t mask_x, mask_y, mask_z;                                                                               \
        uint32_t off_y = 0;                                                                                            \
        generate_swizzle_masks(width, height, 1, &mask_x, &mask_y, &mask_z);                                           \
        const uint32_t step_x = mask_x & (mask_x - 1u);                                                                \
        const uint32_t step_y = mask_y & (mask_y - 1u);                                                                \
        (void)mask_z;                                                                                                  \
                                                                                                                       \
        for (unsigned int y = 0; y < height; y += 2) {                                                                 \
            const uint8_t *src0 = src + (size_t)y * src_pitch;                                                         \
            const uint8_t *src1 = src0 + src_pitch;                                                                    \
            uint32_t off_x = 0;                                                                                        \
            for (unsigned int x = 0; x < width; x += 2) {                                                              \
                uint8_t *dst_px = dst + (size_t)(off_y + off_x) * (BPP);                                               \
                __builtin_memcpy(dst_px, src0, 2 * (BPP));                                                             \
                __builtin_memcpy(dst_px + 2 * (BPP), src1, 2 * (BPP));                                                 \
                src0 += 2 * (BPP);                                                                                     \
                src1 += 2 * (BPP);                                                                                     \
                off_x = (off_x - step_x) & step_x;                                                                     \
            }                                                                                                          \
            off_y = (off_y - step_y) & step_y;                                                                         \
        }                                                                                                              \
    }

#define DEFINE_UNSWIZZLE_2D(BPP)                                                                                       \
    static void unswizzle_2d_##BPP(                                                                                    \
        const uint8_t *src, unsigned int width, unsigned int height, uint8_t *dst, unsigned int dst_pitch)             \
    {                                                                                                                  \
        uint32_t mask_x, mask_y, mask_z;                                                                               \
        uint32_t off_y = 0;                                                                                            \
        generate_swizzle_masks(width, height, 1, &mask_x, &mask_y, &mask_z);                                           \
        const uint32_t step_x = mask_x & (mask_x - 1u);                                                                \
        const uint32_t step_y = mask_y & (mask_y - 1u);                                                                \
        (void)mask_z;                                                                                                  \
                                                                                                                       \
        for (unsigned int y = 0; y < height; y += 2) {                                                                 \
            uint8_t *dst0 = dst + (size_t)y * dst_pitch;                                                               \
            uint32_t off_x = 0;                                                                                        \
            for (unsigned int x = 0; x < width; x += 2) {                                                              \
                const uint8_t *src_px = src + (size_t)(off_y + off_x) * (BPP);                                         \
                __builtin_memcpy(dst0, src_px, 2 * (BPP));                                                             \
                __builtin_memcpy(dst0 + dst_pitch, src_px + 2 * (BPP), 2 * (BPP));                                     \
                dst0 += 2 * (BPP);                                                                                     \
                off_x = (off_x - step_x) & step_x;                                                                     \
            }                                                                                                          \
            off_y = (off_y - step_y) & step_y;                                                                         \
        }                                                                                                              \
    }

DEFINE_SWIZZLE_2D(1)
DEFINE_SWIZZLE_2D(2)
DEFINE_SWIZZLE_2D(3)
DEFINE_UNSWIZZLE_2D(1)
DEFINE_UNSWIZZLE_2D(2)
DEFINE_UNSWIZZLE_2D(3)

#if defined(__SSE__)
/*
 * 32bpp (the most common texture format) moves one 16 byte block with a
 * single SSE store: the two 8 byte source rows are loaded into the low and
 * high halves of an XMM register. This halves the number of memory
 * operations compared to two 8 byte copies, which matters on the Pentium III
 * where a 16 byte store is split into two 8 byte stores anyway.
 */
static void swizzle_2d_4(
    const uint8_t *src, unsigned int width, unsigned int height, uint8_t *dst, unsigned int src_pitch)
{
    uint32_t mask_x, mask_y, mask_z;
    uint32_t off_y = 0;
    generate_swizzle_masks(width, height, 1, &mask_x, &mask_y, &mask_z);
    const uint32_t step_x = mask_x & (mask_x - 1u);
    const uint32_t step_y = mask_y & (mask_y - 1u);
    (void)mask_z;

    for (unsigned int y = 0; y < height; y += 2) {
        const uint8_t *src0 = src + (size_t)y * src_pitch;
        const uint8_t *src1 = src0 + src_pitch;
        uint32_t off_x = 0;
        for (unsigned int x = 0; x < width; x += 2) {
            uint8_t *dst_px = dst + (size_t)(off_y + off_x) * 4;
            __m128 block = _mm_loadl_pi(_mm_setzero_ps(), (const __m64 *)src0);
            block = _mm_loadh_pi(block, (const __m64 *)src1);
            _mm_storeu_ps((float *)dst_px, block);
            src0 += 8;
            src1 += 8;
            off_x = (off_x - step_x) & step_x;
        }
        off_y = (off_y - step_y) & step_y;
    }
}

static void unswizzle_2d_4(
    const uint8_t *src, unsigned int width, unsigned int height, uint8_t *dst, unsigned int dst_pitch)
{
    uint32_t mask_x, mask_y, mask_z;
    uint32_t off_y = 0;
    generate_swizzle_masks(width, height, 1, &mask_x, &mask_y, &mask_z);
    const uint32_t step_x = mask_x & (mask_x - 1u);
    const uint32_t step_y = mask_y & (mask_y - 1u);
    (void)mask_z;

    for (unsigned int y = 0; y < height; y += 2) {
        uint8_t *dst0 = dst + (size_t)y * dst_pitch;
        uint32_t off_x = 0;
        for (unsigned int x = 0; x < width; x += 2) {
            const uint8_t *src_px = src + (size_t)(off_y + off_x) * 4;
            const __m128 block = _mm_loadu_ps((const float *)src_px);
            _mm_storel_pi((__m64 *)dst0, block);
            _mm_storeh_pi((__m64 *)(dst0 + dst_pitch), block);
            dst0 += 8;
            off_x = (off_x - step_x) & step_x;
        }
        off_y = (off_y - step_y) & step_y;
    }
}
#else
DEFINE_SWIZZLE_2D(4)
DEFINE_UNSWIZZLE_2D(4)
#endif

/*
 * Generic path: one pixel at a time. Used for 3D boxes, odd dimensions and
 * unusual pixel sizes.
 */
static void swizzle_generic(const uint8_t *src,
                            unsigned int width,
                            unsigned int height,
                            unsigned int depth,
                            uint8_t *dst,
                            unsigned int row_pitch,
                            unsigned int slice_pitch,
                            unsigned int bytes_per_pixel)
{
    uint32_t mask_x, mask_y, mask_z;
    uint32_t off_z = 0;

    generate_swizzle_masks(width, height, depth, &mask_x, &mask_y, &mask_z);

    for (unsigned int z = 0; z < depth; z++) {
        const uint8_t *src_slice = src + (size_t)z * slice_pitch;
        uint32_t off_y = 0;
        for (unsigned int y = 0; y < height; y++) {
            const uint8_t *src_px = src_slice + (size_t)y * row_pitch;
            uint32_t off_x = 0;
            for (unsigned int x = 0; x < width; x++) {
                uint8_t *dst_px = dst + ((size_t)off_z + off_y + off_x) * bytes_per_pixel;
                __builtin_memcpy(dst_px, src_px, bytes_per_pixel);
                src_px += bytes_per_pixel;
                off_x = (off_x - mask_x) & mask_x;
            }
            off_y = (off_y - mask_y) & mask_y;
        }
        off_z = (off_z - mask_z) & mask_z;
    }
}

static void unswizzle_generic(const uint8_t *src,
                              unsigned int width,
                              unsigned int height,
                              unsigned int depth,
                              uint8_t *dst,
                              unsigned int row_pitch,
                              unsigned int slice_pitch,
                              unsigned int bytes_per_pixel)
{
    uint32_t mask_x, mask_y, mask_z;
    uint32_t off_z = 0;

    generate_swizzle_masks(width, height, depth, &mask_x, &mask_y, &mask_z);

    for (unsigned int z = 0; z < depth; z++) {
        uint8_t *dst_slice = dst + (size_t)z * slice_pitch;
        uint32_t off_y = 0;
        for (unsigned int y = 0; y < height; y++) {
            uint8_t *dst_px = dst_slice + (size_t)y * row_pitch;
            uint32_t off_x = 0;
            for (unsigned int x = 0; x < width; x++) {
                const uint8_t *src_px = src + ((size_t)off_z + off_y + off_x) * bytes_per_pixel;
                __builtin_memcpy(dst_px, src_px, bytes_per_pixel);
                dst_px += bytes_per_pixel;
                off_x = (off_x - mask_x) & mask_x;
            }
            off_y = (off_y - mask_y) & mask_y;
        }
        off_z = (off_z - mask_z) & mask_z;
    }
}

void swizzle_box(const uint8_t *src_buf,
                 unsigned int width,
                 unsigned int height,
                 unsigned int depth,
                 uint8_t *dst_buf,
                 unsigned int row_pitch,
                 unsigned int slice_pitch,
                 unsigned int bytes_per_pixel)
{
    if (depth == 1 && width >= 2 && height >= 2 && ((width | height) & 1u) == 0) {
        switch (bytes_per_pixel) {
            case 1:
                swizzle_2d_1(src_buf, width, height, dst_buf, row_pitch);
                return;
            case 2:
                swizzle_2d_2(src_buf, width, height, dst_buf, row_pitch);
                return;
            case 3:
                swizzle_2d_3(src_buf, width, height, dst_buf, row_pitch);
                return;
            case 4:
                swizzle_2d_4(src_buf, width, height, dst_buf, row_pitch);
                return;
            default:
                break;
        }
    }
    swizzle_generic(src_buf, width, height, depth, dst_buf, row_pitch, slice_pitch, bytes_per_pixel);
}

void unswizzle_box(const uint8_t *src_buf,
                   unsigned int width,
                   unsigned int height,
                   unsigned int depth,
                   uint8_t *dst_buf,
                   unsigned int row_pitch,
                   unsigned int slice_pitch,
                   unsigned int bytes_per_pixel)
{
    if (depth == 1 && width >= 2 && height >= 2 && ((width | height) & 1u) == 0) {
        switch (bytes_per_pixel) {
            case 1:
                unswizzle_2d_1(src_buf, width, height, dst_buf, row_pitch);
                return;
            case 2:
                unswizzle_2d_2(src_buf, width, height, dst_buf, row_pitch);
                return;
            case 3:
                unswizzle_2d_3(src_buf, width, height, dst_buf, row_pitch);
                return;
            case 4:
                unswizzle_2d_4(src_buf, width, height, dst_buf, row_pitch);
                return;
            default:
                break;
        }
    }
    unswizzle_generic(src_buf, width, height, depth, dst_buf, row_pitch, slice_pitch, bytes_per_pixel);
}
