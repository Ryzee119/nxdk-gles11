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

#ifndef HW_XBOX_NV2A_PGRAPH_SWIZZLE_H
#define HW_XBOX_NV2A_PGRAPH_SWIZZLE_H

#include <stdint.h>

/*
 * Convert a linear texture box to the NV2A swizzled (Z-ordered) layout.
 *
 * The NV2A addresses swizzled textures by Z-order over the texture
 * dimensions: the low bits of the texture coordinates are interleaved
 * (x at bit 0, y at bit 1, z at bit 2, x at bit 3, ...).
 *
 * row_pitch       linear row stride in bytes (source for swizzle_box,
 *                 destination for unswizzle_box)
 * slice_pitch     linear slice stride in bytes, only used when depth > 1
 * bytes_per_pixel pixel size in bytes
 */
void swizzle_box(const uint8_t *src_buf,
                 unsigned int width,
                 unsigned int height,
                 unsigned int depth,
                 uint8_t *dst_buf,
                 unsigned int row_pitch,
                 unsigned int slice_pitch,
                 unsigned int bytes_per_pixel);

void unswizzle_box(const uint8_t *src_buf,
                   unsigned int width,
                   unsigned int height,
                   unsigned int depth,
                   uint8_t *dst_buf,
                   unsigned int row_pitch,
                   unsigned int slice_pitch,
                   unsigned int bytes_per_pixel);

static inline void unswizzle_rect(const uint8_t *src_buf,
                                  unsigned int width,
                                  unsigned int height,
                                  uint8_t *dst_buf,
                                  unsigned int pitch,
                                  unsigned int bytes_per_pixel)
{
    unswizzle_box(src_buf, width, height, 1, dst_buf, pitch, 0, bytes_per_pixel);
}

static inline void swizzle_rect(const uint8_t *src_buf,
                                unsigned int width,
                                unsigned int height,
                                uint8_t *dst_buf,
                                unsigned int pitch,
                                unsigned int bytes_per_pixel)
{
    swizzle_box(src_buf, width, height, 1, dst_buf, pitch, 0, bytes_per_pixel);
}

#endif
