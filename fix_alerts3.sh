#!/bin/bash
sed -i 's/src_pixels + y \* src_pitch/src_pixels + row \* src_pitch/' gles_texture.c
sed -i 's/yoff + y/yoff + row/' gles_texture.c
