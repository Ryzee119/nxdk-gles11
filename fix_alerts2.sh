#!/bin/bash
sed -i 's/for (GLsizei y = 0; y < height; y++) {/for (GLsizei row = 0; row < height; row++) {/' gles_texture.c
sed -i 's/memcpy(dst_pixels + y \* xgu_texture->pitch, src_pixels + y \* src_pitch, width \* bytes_per_pixel);/memcpy(dst_pixels + row \* xgu_texture->pitch, src_pixels + row \* src_pitch, width \* bytes_per_pixel);/' gles_texture.c
