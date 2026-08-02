#include "gles_private.h"
#include <swizzle.h>

void gliCalcMipmapChain(GLuint width, GLuint height, GLuint bytes_per_pixel, GLuint *out_size, uint8_t *out_levels)
{
    GLuint size = 0;
    uint8_t levels = 0;
    GLuint w = width;
    GLuint h = height;

    while (w > 0 && h > 0) {
        GLuint pitch = w * bytes_per_pixel;
        size += pitch * h;
        levels++;
        if (w == 1 && h == 1) {
            break;
        }
        if (w > 1) {
            w /= 2;
        }
        if (h > 1) {
            h /= 2;
        }
    }

    if (out_size) {
        *out_size = size;
    }
    if (out_levels) {
        *out_levels = levels;
    }
}

// Simple box filter to generate the next mipmap level
static void generate_next_mipmap_level(const GLubyte *src, GLuint src_w, GLuint src_h, GLuint bpp, GLubyte *dst)
{
    GLuint dst_w = src_w > 1 ? src_w / 2 : 1;
    GLuint dst_h = src_h > 1 ? src_h / 2 : 1;

    for (GLuint y = 0; y < dst_h; y++) {
        for (GLuint x = 0; x < dst_w; x++) {
            for (GLuint c = 0; c < bpp; c++) {
                GLuint sum = 0;
                GLuint count = 0;

                // Sample 2x2 box
                for (GLuint sy = 0; sy < 2; sy++) {
                    for (GLuint sx = 0; sx < 2; sx++) {
                        GLuint px = x * 2 + sx;
                        GLuint py = y * 2 + sy;
                        if (px < src_w && py < src_h) {
                            sum += src[(py * src_w + px) * bpp + c];
                            count++;
                        }
                    }
                }

                dst[(y * dst_w + x) * bpp + c] = (GLubyte)(sum / count);
            }
        }
    }
}

// Swizzled texture auto-generator
void gliGenSwizzledMipmaps(xgu_texture_t *xgu_texture)
{
    if (!xgu_texture->swizzled) {
        return;
    }

    GLuint src_w = xgu_texture->data_width;
    GLuint src_h = xgu_texture->data_height;
    GLuint bpp = xgu_texture->bytes_per_pixel;

    // Allocate a temporary buffer for the unswizzled base level
    GLubyte *unswizzled_base = GLI_MALLOC(src_w * src_h * bpp);
    if (!unswizzled_base) {
        return;
    }

    unswizzle_rect(xgu_texture->data, src_w, src_h, unswizzled_base, src_w * bpp, bpp);

    GLubyte *current_src = unswizzled_base;
    GLuint current_w = src_w;
    GLuint current_h = src_h;

    // Pointer to where the next swizzled level should go
    GLubyte *dst_swizzled = xgu_texture->data + (src_w * src_h * bpp);

    while (current_w > 1 || current_h > 1) {
        GLuint next_w = current_w > 1 ? current_w / 2 : 1;
        GLuint next_h = current_h > 1 ? current_h / 2 : 1;

        GLubyte *unswizzled_next = GLI_MALLOC(next_w * next_h * bpp);
        if (!unswizzled_next) {
            GLI_FREE(unswizzled_base);
            return;
        }

        generate_next_mipmap_level(current_src, current_w, current_h, bpp, unswizzled_next);

        // Swizzle directly into the xgu_texture->data buffer
        swizzle_rect(unswizzled_next, next_w, next_h, dst_swizzled, next_w * bpp, bpp);

        dst_swizzled += (next_w * next_h * bpp);

        if (current_src != unswizzled_base) {
            GLI_FREE(current_src);
        }

        current_src = unswizzled_next;
        current_w = next_w;
        current_h = next_h;
    }

    if (current_src != unswizzled_base) {
        GLI_FREE(current_src);
    }
    GLI_FREE(unswizzled_base);
}

GL_API void GL_APIENTRY glGenerateMipmapOES(GLenum target)
{
    gli_context_t *context = gliGetContext();

    if (target != GL_TEXTURE_2D) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    texture_object_t *texture_object = texture_unit->bound_texture_object;

    if (texture_object->texture_name == 0 || texture_object->texture_2d == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;

    // Mipmaps require swizzled textures (which in turn requires power-of-two)
    if (!xgu_texture->swizzled) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    // Check if we need to reallocate memory for the mipmap chain
    GLuint required_size;
    uint8_t required_levels;
    gliCalcMipmapChain(xgu_texture->data_width,
                       xgu_texture->data_height,
                       xgu_texture->bytes_per_pixel,
                       &required_size,
                       &required_levels);

    if (xgu_texture->data_size < required_size) {
        GLubyte *new_data =
            MmAllocateContiguousMemoryEx(required_size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
        if (!new_data) {
            gliSetError(GL_OUT_OF_MEMORY);
            return;
        }

        // Copy base level
        GLuint base_size = xgu_texture->data_width * xgu_texture->data_height * xgu_texture->bytes_per_pixel;
        gli_memcpy(new_data, xgu_texture->data, base_size);

        MmFreeContiguousMemory(xgu_texture->data);
        xgu_texture->data = new_data;
        xgu_texture->data_size = required_size;
        xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);
    }

    xgu_texture->mipmap_levels = required_levels;

    gliGenSwizzledMipmaps(xgu_texture);

    texture_unit->texture_unit_dirty = GL_TRUE;
}
