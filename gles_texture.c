#include "gles_private.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#define STBIR_MALLOC(size, user_data) GLI_MALLOC(size)
#define STBIR_FREE(ptr, user_data)    GLI_FREE(ptr)
#include <stb_image_resize.h>

#include <swizzle.h>

texture_object_t *gliFindTextureObject(GLuint name, texture_object_t **prev)
{
    gli_context_t *context = gliGetContext();
    texture_object_t *g_textures_head = context->texture_environment.texture_objects;
    if (prev) {
        *prev = NULL;
    }

    if (name == 0) {
        return NULL;
    }

    for (texture_object_t *it = g_textures_head; it != NULL; it = it->next) {
        if (it->texture_name == name) {
            return it;
        }
        if (prev) {
            *prev = it;
        }
    }
    return NULL;
}

GL_API void GL_APIENTRY glActiveTexture(GLenum texture)
{
    gli_context_t *context = gliGetContext();
    if (texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + GLI_MAX_TEXTURE_UNITS) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->texture_environment.server_active_texture = texture;
}

GL_API void GL_APIENTRY glClientActiveTexture(GLenum texture)
{
    gli_context_t *context = gliGetContext();
    if (texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + GLI_MAX_TEXTURE_UNITS) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->vertex_array_data.client_active_texture = texture;
}

GL_API void GL_APIENTRY glGenTextures(GLsizei n, GLuint *textures)
{
    if (textures == NULL) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; i++) {
        while (1) {
            if (next == 0) {
                next++;
            }
            if (gliFindTextureObject(next, NULL) == NULL) {
                break;
            }
            next++;
        }
        textures[i] = next++;
    }
}

GL_API void GL_APIENTRY glBindTexture(GLenum target, GLuint texture)
{
    gli_context_t *context = gliGetContext();

    if (target != GL_TEXTURE_2D) { // GL_TEXTURE_CUBE_MAP_OES maybe?
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // The texture object is bound to the active texture texture_unit
    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];

    // If the texture name is zero just unbind any texture currently bound to the target
    if (texture == 0) {
        texture_unit->texture_binding_2d = 0;
        texture_unit->bound_texture_object = &texture_unit->unbound_texture_object;
        texture_unit->bound_texture_object->texture_object_dirty = GL_TRUE;
        context->transformation_state.texture_matrix_dirty[texture_index] = GL_TRUE;
        return;
    }

    // First check if the object already exists, if so just bind it to the texture_unit and we are done
    texture_object_t *texture_object = gliFindTextureObject(texture, NULL);
    if (texture_object != NULL) {
        texture_unit->texture_binding_2d = texture;
        texture_unit->bound_texture_object = texture_object;
        texture_unit->bound_texture_object->texture_object_dirty = GL_TRUE;
        context->transformation_state.texture_matrix_dirty[texture_index] = GL_TRUE;
        return;
    }

    // It's a new object, so we create it
    texture_object = GLI_MALLOC(sizeof(texture_object_t));
    if (texture_object == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    // Initialize the texture object
    // In the initial state, the value assigned to TEXTURE MIN FILTER is NEAREST MIPMAP LINEAR, and
    // the value for TEXTURE MAG FILTER is LINEAR. s and t wrap modes are both set
    // to REPEAT. The value of GENERATE MIPMAP is false
    gli_memset(texture_object, 0, sizeof(texture_object_t));
    texture_object->texture_name = texture;
    texture_object->texture_object_dirty = GL_TRUE;
    texture_object->min_filter = GL_NEAREST_MIPMAP_LINEAR;
    texture_object->mag_filter = GL_LINEAR;
    texture_object->wrap_s = GL_REPEAT;
    texture_object->wrap_t = GL_REPEAT;
    texture_object->generate_mipmap = GL_FALSE;

    // Bind the object to the texture_unit
    texture_unit->texture_binding_2d = texture;
    texture_unit->bound_texture_object = texture_object;
    context->transformation_state.texture_matrix_dirty[texture_index] = GL_TRUE;

    // Add the object to the context's list
    texture_object->next = context->texture_environment.texture_objects;
    context->texture_environment.texture_objects = texture_object;
}

GL_API void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures)
{
    gli_context_t *context = gliGetContext();
    if (!textures) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    for (GLint i = 0; i < n; i++) {
        GLuint name = textures[i];
        texture_object_t *prev = NULL; // Track previous for linked list removal
        texture_object_t *texture_object = gliFindTextureObject(name, &prev);
        if (texture_object) {
            // Remove from the context's list
            if (prev) {
                prev->next = texture_object->next;
            } else {
                context->texture_environment.texture_objects = texture_object->next;
            }

            // If a texture that is currently bound is deleted, the binding reverts to 0
            for (int u = 0; u < GLI_MAX_TEXTURE_UNITS; ++u) {
                texture_unit_t *texture_unit = &context->texture_environment.texture_units[u];
                if (texture_unit->texture_binding_2d == name) {
                    texture_unit->texture_binding_2d = 0;
                    texture_unit->bound_texture_object = &texture_unit->unbound_texture_object;
                    texture_unit->bound_texture_object->texture_object_dirty = GL_TRUE;
                }
            }

            // If a texture is deleted while attached to a framebuffer, it must be detached
            for (framebuffer_object_t *it = context->framebuffer_objects; it != NULL; it = it->next) {
                if (it->color.type == GL_TEXTURE_2D && it->color.name == name) {
                    it->color.type = GL_NONE_OES;
                    it->color.name = 0;
                    it->color.texture = NULL;
                }
            }

            xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
            if (xgu_texture) {
                if (xgu_texture->data) {
                    MmFreeContiguousMemory(xgu_texture->data);
                }
                GLI_FREE(xgu_texture);
            }
            GLI_FREE(texture_object);
        }
    }
}

GL_API GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
    gli_context_t *context = gliGetContext();
    texture_object_t *texture_object = gliFindTextureObject(texture, NULL);
    if (texture == 0 || texture_object == NULL) {
        return GL_FALSE;
    }
    return GL_TRUE;
}

static inline XguTexFormatColor gliNvTexFormatToSwizzled(XguTexFormatColor format)
{
    switch (format) {
        case XGU_TEXTURE_FORMAT_A8:
            return XGU_TEXTURE_FORMAT_A8_SWIZZLED;
        case XGU_TEXTURE_FORMAT_Y8:
            return XGU_TEXTURE_FORMAT_Y8_SWIZZLED;
        case XGU_TEXTURE_FORMAT_A8Y8:
        case XGU_TEXTURE_FORMAT_AY8:
            return XGU_TEXTURE_FORMAT_A8Y8_SWIZZLED;
        case XGU_TEXTURE_FORMAT_A8R8G8B8:
            return XGU_TEXTURE_FORMAT_A8R8G8B8_SWIZZLED;
        case XGU_TEXTURE_FORMAT_X8R8G8B8:
            return XGU_TEXTURE_FORMAT_X8R8G8B8_SWIZZLED;
        case XGU_TEXTURE_FORMAT_R5G6B5:
            return XGU_TEXTURE_FORMAT_R5G6B5_SWIZZLED;
        case XGU_TEXTURE_FORMAT_A4R4G4B4:
            return XGU_TEXTURE_FORMAT_A4R4G4B4_SWIZZLED;
        case XGU_TEXTURE_FORMAT_A1R5G5B5:
            return XGU_TEXTURE_FORMAT_A1R5G5B5_SWIZZLED;
        case XGU_TEXTURE_FORMAT_X1R5G5B5:
            return XGU_TEXTURE_FORMAT_X1R5G5B5_SWIZZLED;
        default:
            return format;
    }
}

static void gliResampleImage(const void *src,
                             int in_w,
                             int in_h,
                             int src_pitch,
                             void *dst,
                             int out_w,
                             int out_h,
                             int dst_pitch,
                             int bytes_per_pixel,
                             GLenum type)
{
    if (in_w == out_w && in_h == out_h) {
        for (int y = 0; y < out_h; y++) {
            gli_memcpy((uint8_t *)dst + y * dst_pitch, (const uint8_t *)src + y * src_pitch, out_w * bytes_per_pixel);
        }
        return;
    }

    if (type == GL_UNSIGNED_SHORT_5_6_5 || type == GL_UNSIGNED_SHORT_4_4_4_4 || type == GL_UNSIGNED_SHORT_5_5_5_1) {
        uint32_t *src_rgba = GLI_MALLOC(in_w * in_h * sizeof(uint32_t));
        uint32_t *dst_rgba = GLI_MALLOC(out_w * out_h * sizeof(uint32_t));
        if (!src_rgba || !dst_rgba) {
            if (src_rgba) {
                GLI_FREE(src_rgba);
            }
            if (dst_rgba) {
                GLI_FREE(dst_rgba);
            }
            return;
        }

        for (int y = 0; y < in_h; y++) {
            const uint16_t *src_row = (const uint16_t *)((const uint8_t *)src + y * src_pitch);
            uint32_t *dst_row = src_rgba + y * in_w;
            for (int x = 0; x < in_w; x++) {
                uint16_t p = src_row[x];
                uint32_t r = 0, g = 0, b = 0, a = 255;
                if (type == GL_UNSIGNED_SHORT_5_6_5) {
                    r = ((p >> 11) & 0x1F) * 255 / 31;
                    g = ((p >> 5) & 0x3F) * 255 / 63;
                    b = (p & 0x1F) * 255 / 31;
                } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
                    r = ((p >> 12) & 0xF) * 255 / 15;
                    g = ((p >> 8) & 0xF) * 255 / 15;
                    b = ((p >> 4) & 0xF) * 255 / 15;
                    a = (p & 0xF) * 255 / 15;
                } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
                    r = ((p >> 11) & 0x1F) * 255 / 31;
                    g = ((p >> 6) & 0x1F) * 255 / 31;
                    b = ((p >> 1) & 0x1F) * 255 / 31;
                    a = (p & 1) ? 255 : 0;
                }
                dst_row[x] = (r) | (g << 8) | (b << 16) | (a << 24);
            }
        }

        stbir_resize_uint8((const unsigned char *)src_rgba,
                           in_w,
                           in_h,
                           in_w * 4,
                           (unsigned char *)dst_rgba,
                           out_w,
                           out_h,
                           out_w * 4,
                           4);

        for (int y = 0; y < out_h; y++) {
            const uint32_t *src_row = dst_rgba + y * out_w;
            uint16_t *dst_row = (uint16_t *)((uint8_t *)dst + y * dst_pitch);
            for (int x = 0; x < out_w; x++) {
                uint32_t p = src_row[x];
                uint32_t r = p & 0xFF;
                uint32_t g = (p >> 8) & 0xFF;
                uint32_t b = (p >> 16) & 0xFF;
                uint32_t a = (p >> 24) & 0xFF;
                uint16_t packed = 0;
                if (type == GL_UNSIGNED_SHORT_5_6_5) {
                    packed = ((r * 31 / 255) << 11) | ((g * 63 / 255) << 5) | (b * 31 / 255);
                } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
                    packed = ((r * 15 / 255) << 12) | ((g * 15 / 255) << 8) | ((b * 15 / 255) << 4) | (a * 15 / 255);
                } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
                    packed =
                        ((r * 31 / 255) << 11) | ((g * 31 / 255) << 6) | ((b * 31 / 255) << 1) | ((a >= 128) ? 1 : 0);
                }
                dst_row[x] = packed;
            }
        }

        GLI_FREE(src_rgba);
        GLI_FREE(dst_rgba);
        return;
    }

    stbir_resize_uint8((const unsigned char *)src,
                       in_w,
                       in_h,
                       src_pitch,
                       (unsigned char *)dst,
                       out_w,
                       out_h,
                       dst_pitch,
                       bytes_per_pixel);
}

static void gliPromoteTextureToSwizzled(texture_object_t *texture_object)
{
    if (!texture_object || !texture_object->texture_2d) {
        return;
    }
    xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
    if (xgu_texture->swizzled) {
        return; // Already swizzled
    }

    GLuint container_w = npot2pot(xgu_texture->tex_width);
    GLuint container_h = npot2pot(xgu_texture->tex_height);
    container_w = GLI_MAX(8, container_w);
    container_h = GLI_MAX(8, container_h);

    GLuint bytes_per_pixel = xgu_texture->bytes_per_pixel;
    GLuint pitch = container_w * bytes_per_pixel;
    GLuint alloc_size = pitch * container_h;

    if (texture_object->generate_mipmap) {
        GLuint required_size;
        uint8_t required_levels;
        gliCalcMipmapChain(container_w, container_h, bytes_per_pixel, &required_size, &required_levels);
        alloc_size = required_size;
        xgu_texture->mipmap_levels = required_levels;
    }

    GLubyte *new_data =
        MmAllocateContiguousMemoryEx(alloc_size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!new_data) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }
    gli_memset(new_data, 0, alloc_size);

    if (xgu_texture->data != NULL) {
        GLubyte *resampled = GLI_MALLOC(pitch * container_h);
        if (!resampled) {
            MmFreeContiguousMemory(new_data);
            gliSetError(GL_OUT_OF_MEMORY);
            return;
        }
        gliResampleImage(xgu_texture->data,
                         xgu_texture->tex_width,
                         xgu_texture->tex_height,
                         xgu_texture->pitch,
                         resampled,
                         container_w,
                         container_h,
                         pitch,
                         bytes_per_pixel,
                         texture_object->type);
        swizzle_rect(resampled, container_w, container_h, new_data, pitch, bytes_per_pixel);
        GLI_FREE(resampled);
        MmFreeContiguousMemory(xgu_texture->data);
    }

    xgu_texture->data = new_data;
    xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(new_data);
    xgu_texture->data_size = alloc_size;
    xgu_texture->data_width = container_w;
    xgu_texture->data_height = container_h;
    xgu_texture->pitch = pitch;
    xgu_texture->swizzled = 1;
    xgu_texture->format = gliNvTexFormatToSwizzled(xgu_texture->format);
    xgu_texture->u_scale = 1.0f;
    xgu_texture->v_scale = 1.0f;

    if (texture_object->generate_mipmap) {
        gliGenSwizzledMipmaps(xgu_texture);
    }

    texture_object->texture_object_dirty = GL_TRUE;

    // Mark texture matrix dirty for any unit that has this texture bound
    gli_context_t *context = gliGetContext();
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        if (context->texture_environment.texture_units[i].bound_texture_object == texture_object) {
            context->transformation_state.texture_matrix_dirty[i] = GL_TRUE;
        }
    }
}

GL_API void GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    gli_context_t *context = gliGetContext();
    if (target != GL_TEXTURE_2D) { // GL_TEXTURE_CUBE_MAP_OES maybe?
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    texture_object_t *texture_object = texture_unit->bound_texture_object;

    if (texture_object == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            if (params[0] != GL_NEAREST && params[0] != GL_LINEAR && params[0] != GL_NEAREST_MIPMAP_NEAREST &&
                params[0] != GL_LINEAR_MIPMAP_NEAREST && params[0] != GL_NEAREST_MIPMAP_LINEAR &&
                params[0] != GL_LINEAR_MIPMAP_LINEAR) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            texture_object->min_filter = params[0];
            break;
        case GL_TEXTURE_MAG_FILTER:
            if (params[0] != GL_NEAREST && params[0] != GL_LINEAR) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            texture_object->mag_filter = params[0];
            break;
        case GL_TEXTURE_WRAP_S:
            if (params[0] != GL_CLAMP_TO_EDGE && params[0] != GL_REPEAT && params[0] != GL_MIRRORED_REPEAT_OES) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            texture_object->wrap_s = params[0];
            if (params[0] != GL_CLAMP_TO_EDGE) {
                gliPromoteTextureToSwizzled(texture_object);
            }
            break;
        case GL_TEXTURE_WRAP_T:
            if (params[0] != GL_CLAMP_TO_EDGE && params[0] != GL_REPEAT && params[0] != GL_MIRRORED_REPEAT_OES) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            texture_object->wrap_t = params[0];
            if (params[0] != GL_CLAMP_TO_EDGE) {
                gliPromoteTextureToSwizzled(texture_object);
            }
            break;
        case GL_GENERATE_MIPMAP:
            texture_object->generate_mipmap = (params[0]) ? GL_TRUE : GL_FALSE;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
    texture_object->texture_object_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        case GL_GENERATE_MIPMAP:
            glTexParameterf(target, pname, params[0]);
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glTexParameterxv(GLenum target, GLenum pname, const GLfixed *params)
{
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        case GL_GENERATE_MIPMAP:
            glTexParameterx(target, pname, params[0]);
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    glTexParameteriv(target, pname, &param);
}

GL_API void GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    // Nothing in GLES 1.1 uses float parameters, so just cast to int
    glTexParameteri(target, pname, (GLint)param);
}

GL_API void GL_APIENTRY glTexParameterx(GLenum target, GLenum pname, GLfixed param)
{
    glTexParameterf(target, pname, gliFixedtoFloat(param));
}

GL_API void GL_APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    gli_context_t *context = gliGetContext();

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];

    switch (pname) {
        case GL_TEXTURE_ENV_MODE:
            switch (params[0]) {
                case GL_MODULATE:
                case GL_DECAL:
                case GL_BLEND:
                case GL_REPLACE:
                case GL_ADD:
                case GL_COMBINE:
                    break;
                default:
                    gliSetError(GL_INVALID_ENUM);
                    return;
            }
            texture_unit->tex_env_mode = (GLenum)params[0];
            break;
        case GL_TEXTURE_ENV_COLOR:
            texture_unit->tex_env_color[0] = INT_TO_FLOAT(params[0]);
            texture_unit->tex_env_color[1] = INT_TO_FLOAT(params[1]);
            texture_unit->tex_env_color[2] = INT_TO_FLOAT(params[2]);
            texture_unit->tex_env_color[3] = INT_TO_FLOAT(params[3]);
            break;
        case GL_COMBINE_RGB:
            switch (params[0]) {
                case GL_REPLACE:
                case GL_MODULATE:
                case GL_ADD:
                case GL_ADD_SIGNED:
                case GL_INTERPOLATE:
                case GL_SUBTRACT:
                case GL_DOT3_RGB:
                case GL_DOT3_RGBA:
                    break;
                default:
                    gliSetError(GL_INVALID_ENUM);
                    return;
            }
            texture_unit->combine_rgb_function = (GLenum)params[0];
            break;
        case GL_COMBINE_ALPHA:
            switch (params[0]) {
                case GL_REPLACE:
                case GL_MODULATE:
                case GL_ADD:
                case GL_ADD_SIGNED:
                case GL_INTERPOLATE:
                case GL_SUBTRACT:
                    break;
                default:
                    gliSetError(GL_INVALID_ENUM);
                    return;
            }
            texture_unit->combine_alpha_function = (GLenum)params[0];
            break;

        case GL_SRC0_RGB:
        case GL_SRC1_RGB:
        case GL_SRC2_RGB: {
            const GLint index = pname - GL_SRC0_RGB;
            switch (params[0]) {
                case GL_TEXTURE:
                case GL_CONSTANT:
                case GL_PRIMARY_COLOR:
                case GL_PREVIOUS:
                    break;
                default:
                    gliSetError(GL_INVALID_ENUM);
                    return;
            }
            texture_unit->combine_rgb_source[index] = (GLenum)params[0];
        } break;

        case GL_SRC0_ALPHA:
        case GL_SRC1_ALPHA:
        case GL_SRC2_ALPHA: {
            const GLint index = pname - GL_SRC0_ALPHA;
            switch (params[0]) {
                case GL_TEXTURE:
                case GL_CONSTANT:
                case GL_PRIMARY_COLOR:
                case GL_PREVIOUS:
                    break;
                default:
                    gliSetError(GL_INVALID_ENUM);
                    return;
            }
            texture_unit->combine_alpha_source[index] = (GLenum)params[0];
        } break;

        case GL_OPERAND0_RGB:
        case GL_OPERAND1_RGB:
        case GL_OPERAND2_RGB: {
            const GLint index = pname - GL_OPERAND0_RGB;
            switch (params[0]) {
                case GL_SRC_COLOR:
                case GL_ONE_MINUS_SRC_COLOR:
                case GL_SRC_ALPHA:
                case GL_ONE_MINUS_SRC_ALPHA:
                    break;
                default:
                    gliSetError(GL_INVALID_ENUM);
                    return;
            }
            texture_unit->combine_rgb_operand[index] = (GLenum)params[0];
        } break;

        case GL_OPERAND0_ALPHA:
        case GL_OPERAND1_ALPHA:
        case GL_OPERAND2_ALPHA: {
            const GLint index = pname - GL_OPERAND0_ALPHA;
            switch (params[0]) {
                case GL_SRC_ALPHA:
                case GL_ONE_MINUS_SRC_ALPHA:
                    break;
                default:
                    gliSetError(GL_INVALID_ENUM);
                    return;
            }
            texture_unit->combine_alpha_operand[index] = (GLenum)params[0];
        } break;

        case GL_RGB_SCALE:
            if (params[0] != 1 && params[0] != 2 && params[0] != 4) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            texture_unit->rgb_scale = (GLfloat)params[0];
            break;

        case GL_ALPHA_SCALE:
            if (params[0] != 1 && params[0] != 2 && params[0] != 4) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            texture_unit->alpha_scale = (GLfloat)params[0];
            break;

        case GL_COORD_REPLACE_OES:
            texture_unit->coord_replace_oes_enabled = (params[0]) ? GL_TRUE : GL_FALSE;
            break;

        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
    texture_unit->texture_unit_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];

    switch (pname) {
        case GL_TEXTURE_ENV_COLOR:
            for (GLint i = 0; i < 4; i++) {
                texture_unit->tex_env_color[i] = params[i];
            }
            texture_unit->texture_unit_dirty = GL_TRUE;
            return;
        default:
            break;
    }

    GLint iparam = (GLint)params[0];
    glTexEnviv(target, pname, &iparam);
}

GL_API void GL_APIENTRY glTexEnvxv(GLenum target, GLenum pname, const GLfixed *params)
{
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (pname == GL_TEXTURE_ENV_COLOR) {
        GLfloat fparams[4];
        for (GLint i = 0; i < 4; i++) {
            fparams[i] = gliFixedtoFloat(params[i]);
        }
        glTexEnvfv(target, pname, fparams);
        return;
    }
    GLfloat paramf = gliFixedtoFloat(params[0]);
    glTexEnvfv(target, pname, &paramf);
}

GL_API void GL_APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    glTexEnviv(target, pname, &param);
}

GL_API void GL_APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    if (pname == GL_TEXTURE_ENV_COLOR) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    GLint iparam = (GLint)param;
    glTexEnviv(target, pname, &iparam);
}

GL_API void GL_APIENTRY glTexEnvx(GLenum target, GLenum pname, GLfixed param)
{
    if (pname == GL_TEXTURE_ENV_COLOR) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    GLfloat paramf = gliFixedtoFloat(param);
    glTexEnvfv(target, pname, &paramf);
}

GL_API void GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
    gli_context_t *context = gliGetContext();
    if (target != GL_TEXTURE_2D) { // GL_TEXTURE_CUBE_MAP_OES maybe?
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    texture_object_t *texture_object = texture_unit->bound_texture_object;

    assert(texture_object != NULL);

    switch (pname) {
        case GL_TEXTURE_MIN_FILTER:
            params[0] = (GLint)texture_object->min_filter;
            break;
        case GL_TEXTURE_MAG_FILTER:
            params[0] = (GLint)texture_object->mag_filter;
            break;
        case GL_TEXTURE_WRAP_S:
            params[0] = (GLint)texture_object->wrap_s;
            break;
        case GL_TEXTURE_WRAP_T:
            params[0] = (GLint)texture_object->wrap_t;
            break;
        case GL_GENERATE_MIPMAP:
            params[0] = texture_object->generate_mipmap;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLint ival;
    glGetTexParameteriv(target, pname, &ival);
    params[0] = (GLfloat)ival;
}

GL_API void GL_APIENTRY glGetTexParameterxv(GLenum target, GLenum pname, GLfixed *params)
{
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    // GLES 1.1 has no parameters that require multiple elements. We just get the single value, convert and return.
    GLfloat paramf;
    glGetTexParameterfv(target, pname, &paramf);
    *params = gliFloattoFixed(paramf);
}

GL_API void GL_APIENTRY glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
    gli_context_t *context = gliGetContext();
    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];

    switch (pname) {
        case GL_TEXTURE_ENV_MODE:
            params[0] = (GLint)texture_unit->tex_env_mode;
            break;
        case GL_TEXTURE_ENV_COLOR:
            for (GLint i = 0; i < 4; i++) {
                params[i] = FLOAT_TO_INT(texture_unit->tex_env_color[i]);
            }
            break;
        case GL_COMBINE_RGB:
            params[0] = (GLint)texture_unit->combine_rgb_function;
            break;
        case GL_COMBINE_ALPHA:
            params[0] = (GLint)texture_unit->combine_alpha_function;
            break;
        case GL_SRC0_RGB:
        case GL_SRC1_RGB:
        case GL_SRC2_RGB: {
            const GLint index = pname - GL_SRC0_RGB;
            params[0] = (GLint)texture_unit->combine_rgb_source[index];
        } break;
        case GL_SRC0_ALPHA:
        case GL_SRC1_ALPHA:
        case GL_SRC2_ALPHA: {
            const GLint index = pname - GL_SRC0_ALPHA;
            params[0] = (GLint)texture_unit->combine_alpha_source[index];
        } break;
        case GL_OPERAND0_RGB:
        case GL_OPERAND1_RGB:
        case GL_OPERAND2_RGB: {
            const GLint index = pname - GL_OPERAND0_RGB;
            params[0] = (GLint)texture_unit->combine_rgb_operand[index];
        } break;
        case GL_OPERAND0_ALPHA:
        case GL_OPERAND1_ALPHA:
        case GL_OPERAND2_ALPHA: {
            const GLint index = pname - GL_OPERAND0_ALPHA;
            params[0] = (GLint)texture_unit->combine_alpha_operand[index];
        } break;
        case GL_RGB_SCALE:
            params[0] = (GLint)(texture_unit->rgb_scale);
            break;
        case GL_ALPHA_SCALE:
            params[0] = (GLint)(texture_unit->alpha_scale);
            break;
        case GL_COORD_REPLACE_OES:
            params[0] = texture_unit->coord_replace_oes_enabled ? 1 : 0;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
    gli_context_t *context = gliGetContext();
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (target != GL_TEXTURE_ENV && target != GL_POINT_SPRITE_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // Handle special case of TEXURE_ENV_COLOR first
    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    if (pname == GL_TEXTURE_ENV_COLOR) {
        for (GLint i = 0; i < 4; i++) {
            params[i] = texture_unit->tex_env_color[i];
        }
        return;
    }

    GLint iparam;
    glGetTexEnviv(target, pname, &iparam);
    *params = (GLfloat)iparam;
}

GL_API void GL_APIENTRY glGetTexEnvxv(GLenum target, GLenum pname, GLfixed *params)
{
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (pname == GL_TEXTURE_ENV_COLOR) {
        GLfloat fparams[4];
        glGetTexEnvfv(target, pname, fparams);
        for (GLint i = 0; i < 4; i++) {
            params[i] = gliFloattoFixed(fparams[i]);
        }
        return;
    }
    GLfloat paramf;
    glGetTexEnvfv(target, pname, &paramf);
    *params = gliFloattoFixed(paramf);
}

static inline void convert_to_bgra(const uint8_t *restrict src, uint8_t *restrict dst, size_t pixel_count, int src_bpp)
{
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[0] = src[2];                         // B
        dst[1] = src[1];                         // G
        dst[2] = src[0];                         // R
        dst[3] = (src_bpp == 4) ? src[3] : 0xFF; // A

        src += src_bpp;
        dst += 4;
    }
}

GL_API void GL_APIENTRY glTexImage2D(GLenum target,
                                     GLint level,
                                     GLint internalformat,
                                     GLsizei width,
                                     GLsizei height,
                                     GLint border,
                                     GLenum format,
                                     GLenum type,
                                     const void *pixels)
{
    gli_context_t *context = gliGetContext();

    if (target != GL_TEXTURE_2D) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (format != GL_ALPHA && format != GL_RGB && format != GL_RGBA && format != GL_LUMINANCE &&
        format != GL_LUMINANCE_ALPHA) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT_5_6_5 && type != GL_UNSIGNED_SHORT_4_4_4_4 &&
        type != GL_UNSIGNED_SHORT_5_5_5_1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (level < 0 || width < 0 || height < 0 || border != 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (width > GLI_MAX_TEXTURE_SIZE || height > GLI_MAX_TEXTURE_SIZE) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (internalformat != format) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (type == GL_UNSIGNED_SHORT_5_6_5 && (format != GL_RGB)) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if ((type == GL_UNSIGNED_SHORT_4_4_4_4 || type == GL_UNSIGNED_SHORT_5_5_5_1) && (format != GL_RGBA)) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    // Get the currently bound texture object
    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    texture_object_t *texture_object = texture_unit->bound_texture_object;

    if (level > 0) {
        if (texture_object->texture_name == 0 || texture_object->texture_2d == NULL) {
            gliSetError(GL_INVALID_OPERATION); // Can't define mipmap without base level on this hardware
            return;
        }

        xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
        if (!xgu_texture->swizzled) {
            gliSetError(GL_INVALID_OPERATION); // Mipmaps require POT swizzled textures
            return;
        }

        // Ensure the level matches the expected dimensions based on level 0
        GLuint expected_w = xgu_texture->data_width;
        GLuint expected_h = xgu_texture->data_height;
        for (GLint i = 0; i < level; i++) {
            if (expected_w > 1) {
                expected_w /= 2;
            }
            if (expected_h > 1) {
                expected_h /= 2;
            }
        }

        if ((GLuint)width != expected_w || (GLuint)height != expected_h) {
            gliSetError(GL_INVALID_VALUE);
            return;
        }

        // Calculate offset to the mipmap level
        GLuint required_size;
        uint8_t required_levels;
        gliCalcMipmapChain(xgu_texture->data_width,
                           xgu_texture->data_height,
                           xgu_texture->bytes_per_pixel,
                           &required_size,
                           &required_levels);

        if (level >= required_levels) {
            gliSetError(GL_INVALID_VALUE);
            return;
        }

        // Check if we need to reallocate the texture to fit mipmaps
        if (xgu_texture->data_size < required_size) {
            GLubyte *new_data =
                MmAllocateContiguousMemoryEx(required_size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
            if (!new_data) {
                gliSetError(GL_OUT_OF_MEMORY);
                return;
            }

            gli_memset(new_data, 0, required_size);
            GLuint base_size = xgu_texture->data_width * xgu_texture->data_height * xgu_texture->bytes_per_pixel;
            gli_memcpy(new_data, xgu_texture->data, base_size);

            MmFreeContiguousMemory(xgu_texture->data);
            xgu_texture->data = new_data;
            xgu_texture->data_size = required_size;
            xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);
        }

        if (xgu_texture->mipmap_levels < (uint8_t)(level + 1)) {
            xgu_texture->mipmap_levels = (uint8_t)(level + 1);
        }

        // Calculate offset for the current level
        GLuint level_offset = 0;
        GLuint current_w = xgu_texture->data_width;
        GLuint current_h = xgu_texture->data_height;
        for (GLint i = 0; i < level; i++) {
            level_offset += current_w * current_h * xgu_texture->bytes_per_pixel;
            if (current_w > 1) {
                current_w /= 2;
            }
            if (current_h > 1) {
                current_h /= 2;
            }
        }

        if (pixels != NULL) {
            size_t input_bpp = (type == GL_UNSIGNED_BYTE) ? ((format == GL_RGB)               ? 3
                                                             : (format == GL_RGBA)            ? 4
                                                             : (format == GL_LUMINANCE_ALPHA) ? 2
                                                                                              : 1)
                                                          : 2;
            const GLint alignment = context->pixel_store.unpack_alignment;
            const size_t user_src_pitch = (((size_t)width * input_bpp) + (alignment - 1)) & ~(size_t)(alignment - 1);
            const GLubyte *src_pixels = (const GLubyte *)pixels;
            size_t effective_pitch = user_src_pitch;

            if (type == GL_UNSIGNED_BYTE && (format == GL_RGB || format == GL_RGBA)) {
                const size_t conv_pitch = (size_t)width * 4;
                GLubyte *converted_pixels = GLI_MALLOC(conv_pitch * height);
                if (!converted_pixels) {
                    gliSetError(GL_OUT_OF_MEMORY);
                    return;
                }
                for (GLuint y = 0; y < (GLuint)height; y++) {
                    convert_to_bgra(src_pixels + y * user_src_pitch,
                                    converted_pixels + y * conv_pitch,
                                    width,
                                    (format == GL_RGBA) ? 4 : 3);
                }
                src_pixels = converted_pixels;
                effective_pitch = conv_pitch;
            }

            if (xgu_texture->swizzled) {
                swizzle_rect(src_pixels,
                             current_w,
                             current_h,
                             xgu_texture->data + level_offset,
                             effective_pitch,
                             xgu_texture->bytes_per_pixel);
            } else {
                for (GLuint y = 0; y < (GLuint)height; y++) {
                    gli_memcpy(xgu_texture->data + level_offset + y * xgu_texture->pitch,
                               src_pixels + y * effective_pitch,
                               width * xgu_texture->bytes_per_pixel);
                }
            }

            if (src_pixels != (const GLubyte *)pixels) {
                GLI_FREE((void *)src_pixels);
            }
        }

        texture_object->texture_object_dirty = GL_TRUE;
        return;
    }

    // level == 0
    xgu_texture_t *xgu_texture = GLI_MALLOC(sizeof(xgu_texture_t));
    if (xgu_texture == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }
    gli_memset(xgu_texture, 0, sizeof(xgu_texture_t));

    xgu_texture->swizzled = 0;

    // If already POT, or if non-clamp wrap mode is configured, use swizzled texture in POT container
    if ((width == npot2pot(width) && height == npot2pot(height)) ||
        (texture_object->wrap_s != GL_CLAMP_TO_EDGE || texture_object->wrap_t != GL_CLAMP_TO_EDGE)) {
        xgu_texture->swizzled = 1;
    }

    GLuint bytes_per_pixel = 0;
    XguTexFormatColor xgu_format = gliEnumToNvTexFormat(format, type, &bytes_per_pixel, xgu_texture->swizzled);
    assert(xgu_format != -1);
    assert(bytes_per_pixel != 0);

    xgu_texture->tex_width = width;
    xgu_texture->tex_height = height;
    xgu_texture->bytes_per_pixel = bytes_per_pixel;
    xgu_texture->mipmap_levels = 1;

    // Swizzled textures are resampled to fill the POT container completely with normalized coords [0, 1]
    if (xgu_texture->swizzled) {
        xgu_texture->data_width = npot2pot(width);
        xgu_texture->data_height = npot2pot(height);
        xgu_texture->data_width = GLI_MAX(8, xgu_texture->data_width);
        xgu_texture->data_height = GLI_MAX(8, xgu_texture->data_height);
        xgu_texture->u_scale = 1.0f;
        xgu_texture->v_scale = 1.0f;
        xgu_texture->pitch = xgu_texture->data_width * bytes_per_pixel;
    } else {
        xgu_texture->data_width = width;
        xgu_texture->data_height = height;
        xgu_texture->u_scale = (GLfloat)width;
        xgu_texture->v_scale = (GLfloat)height;
        // NV2A hardware requires pitched surfaces (like FBO attachments) to be 64-byte aligned
        xgu_texture->pitch = (xgu_texture->data_width * bytes_per_pixel + 63) & ~63;
    }

    // Allocate space for mipmaps immediately if GL_GENERATE_MIPMAP is enabled and it's swizzled
    GLuint alloc_size = xgu_texture->pitch * xgu_texture->data_height;
    if (xgu_texture->swizzled && texture_object->generate_mipmap) {
        GLuint required_size;
        uint8_t required_levels;
        gliCalcMipmapChain(xgu_texture->data_width,
                           xgu_texture->data_height,
                           xgu_texture->bytes_per_pixel,
                           &required_size,
                           &required_levels);
        alloc_size = required_size;
        xgu_texture->mipmap_levels = required_levels;
    }

    xgu_texture->format = xgu_format;
    xgu_texture->data_size = alloc_size;
    xgu_texture->data =
        MmAllocateContiguousMemoryEx(alloc_size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (xgu_texture->data == NULL) {
        GLI_FREE(xgu_texture);
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }
    gli_memset(xgu_texture->data, 0, alloc_size);

    xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);

    if (pixels != NULL) {
        size_t input_bpp = (type == GL_UNSIGNED_BYTE) ? ((format == GL_RGB)               ? 3
                                                         : (format == GL_RGBA)            ? 4
                                                         : (format == GL_LUMINANCE_ALPHA) ? 2
                                                                                          : 1)
                                                      : 2;
        const GLint alignment = context->pixel_store.unpack_alignment;
        const size_t user_src_pitch = (((size_t)width * input_bpp) + (alignment - 1)) & ~(size_t)(alignment - 1);
        const GLubyte *src_pixels = (const GLubyte *)pixels;
        GLubyte *dst_pixels = xgu_texture->data;
        size_t effective_pitch = user_src_pitch;

        if (type == GL_UNSIGNED_BYTE && (format == GL_RGB || format == GL_RGBA)) {
            const size_t conv_pitch = (size_t)width * 4;
            GLubyte *converted_pixels = GLI_MALLOC(conv_pitch * height);
            if (!converted_pixels) {
                gliSetError(GL_OUT_OF_MEMORY);
                return;
            }
            for (GLuint y = 0; y < (GLuint)height; y++) {
                convert_to_bgra(src_pixels + y * user_src_pitch,
                                converted_pixels + y * conv_pitch,
                                width,
                                (format == GL_RGBA) ? 4 : 3);
            }
            src_pixels = converted_pixels;
            effective_pitch = conv_pitch;
        }

        if (xgu_texture->swizzled) {
            if (width == (GLsizei)xgu_texture->data_width && height == (GLsizei)xgu_texture->data_height) {
                swizzle_rect(src_pixels,
                             xgu_texture->data_width,
                             xgu_texture->data_height,
                             dst_pixels,
                             effective_pitch,
                             xgu_texture->bytes_per_pixel);
            } else {
                GLubyte *resampled = GLI_MALLOC(xgu_texture->pitch * xgu_texture->data_height);
                if (resampled) {
                    gliResampleImage(src_pixels,
                                     width,
                                     height,
                                     effective_pitch,
                                     resampled,
                                     xgu_texture->data_width,
                                     xgu_texture->data_height,
                                     xgu_texture->pitch,
                                     xgu_texture->bytes_per_pixel,
                                     type);
                    swizzle_rect(resampled,
                                 xgu_texture->data_width,
                                 xgu_texture->data_height,
                                 dst_pixels,
                                 xgu_texture->pitch,
                                 xgu_texture->bytes_per_pixel);
                    GLI_FREE(resampled);
                }
            }
        } else {
            for (GLuint y = 0; y < (GLuint)height; y++) {
                gli_memcpy(
                    dst_pixels + y * xgu_texture->pitch, src_pixels + y * effective_pitch, width * bytes_per_pixel);
            }
        }

        if (src_pixels != (const GLubyte *)pixels) {
            GLI_FREE((void *)src_pixels);
        }
    }

    if (texture_object->texture_2d != NULL) {
        xgu_texture_t *old_tex = (xgu_texture_t *)texture_object->texture_2d;
        if (old_tex->data) {
            MmFreeContiguousMemory(old_tex->data);
        }
        GLI_FREE(old_tex);
    }

    texture_object->texture_2d = xgu_texture;
    texture_object->internalformat = internalformat;
    texture_object->type = type;

    if (xgu_texture->swizzled && texture_object->generate_mipmap) {
        gliGenSwizzledMipmaps(xgu_texture);
    }

    texture_object->texture_object_dirty = GL_TRUE;

    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        if (context->texture_environment.texture_units[i].bound_texture_object == texture_object) {
            context->transformation_state.texture_matrix_dirty[i] = GL_TRUE;
        }
    }
}

GL_API void GL_APIENTRY glTexSubImage2D(GLenum target,
                                        GLint level,
                                        GLint xoff,
                                        GLint yoff,
                                        GLsizei w,
                                        GLsizei h,
                                        GLenum format,
                                        GLenum type,
                                        const void *pixels)
{
    if (target != GL_TEXTURE_2D) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (format != GL_ALPHA && format != GL_RGB && format != GL_RGBA && format != GL_LUMINANCE &&
        format != GL_LUMINANCE_ALPHA) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT_5_6_5 && type != GL_UNSIGNED_SHORT_4_4_4_4 &&
        type != GL_UNSIGNED_SHORT_5_5_5_1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (type == GL_UNSIGNED_SHORT_5_6_5 && format != GL_RGB) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if ((type == GL_UNSIGNED_SHORT_4_4_4_4 || type == GL_UNSIGNED_SHORT_5_5_5_1) && format != GL_RGBA) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (level < 0 || w < 0 || h < 0 || xoff < 0 || yoff < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (!pixels || w == 0 || h == 0) {
        return;
    }

    gli_context_t *context = gliGetContext();
    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    texture_object_t *texture_object = texture_unit->bound_texture_object;

    if (!texture_object || texture_object->texture_name == 0 || !texture_object->texture_2d) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (format != texture_object->internalformat || type != texture_object->type) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;

    if (level >= xgu_texture->mipmap_levels) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint current_w = xgu_texture->data_width;
    GLuint current_h = xgu_texture->data_height;
    GLuint level_offset = 0;
    for (GLint i = 0; i < level; i++) {
        level_offset += current_w * current_h * xgu_texture->bytes_per_pixel;
        if (current_w > 1) {
            current_w /= 2;
        }
        if (current_h > 1) {
            current_h /= 2;
        }
    }

    if (xoff + w > (GLint)current_w || yoff + h > (GLint)current_h) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    size_t input_bpp = (type == GL_UNSIGNED_BYTE) ? ((format == GL_RGB)               ? 3
                                                     : (format == GL_RGBA)            ? 4
                                                     : (format == GL_LUMINANCE_ALPHA) ? 2
                                                                                      : 1)
                                                  : 2;
    const GLint alignment = context->pixel_store.unpack_alignment;
    const size_t user_src_pitch = (((size_t)w * input_bpp) + (alignment - 1)) & ~(size_t)(alignment - 1);

    const GLubyte *src_pixels = (const GLubyte *)pixels;
    size_t effective_pitch = user_src_pitch;

    if (type == GL_UNSIGNED_BYTE && (format == GL_RGB || format == GL_RGBA)) {
        const size_t conv_pitch = (size_t)w * 4;
        GLubyte *converted_pixels = GLI_MALLOC(conv_pitch * h);
        if (!converted_pixels) {
            gliSetError(GL_OUT_OF_MEMORY);
            return;
        }
        for (GLsizei y = 0; y < h; y++) {
            convert_to_bgra(
                src_pixels + y * user_src_pitch, converted_pixels + y * conv_pitch, w, (format == GL_RGBA) ? 4 : 3);
        }
        src_pixels = converted_pixels;
        effective_pitch = conv_pitch;
    }

    if (xgu_texture->swizzled) {
        swizzle_subrect(src_pixels,
                        current_w,
                        current_h,
                        xoff,
                        yoff,
                        w,
                        h,
                        xgu_texture->data + level_offset,
                        effective_pitch,
                        xgu_texture->bytes_per_pixel);
    } else {
        for (GLsizei y = 0; y < h; y++) {
            const GLubyte *src_row = src_pixels + y * effective_pitch;
            GLubyte *dst_row = xgu_texture->data + level_offset + (yoff + y) * xgu_texture->pitch +
                               xoff * xgu_texture->bytes_per_pixel;
            gli_memcpy(dst_row, src_row, w * xgu_texture->bytes_per_pixel);
        }
    }

    if (src_pixels != (const GLubyte *)pixels) {
        GLI_FREE((void *)src_pixels);
    }

    texture_object->texture_object_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glCompressedTexImage2D(GLenum target,
                                               GLint level,
                                               GLenum internalformat,
                                               GLsizei width,
                                               GLsizei height,
                                               GLint border,
                                               GLsizei imageSize,
                                               const void *data)
{
    assert(0 && "glCompressedTexImage2D not implemented yet");
    (void)target;
    (void)level;
    (void)internalformat;
    (void)width;
    (void)height;
    (void)border;
    (void)imageSize;
    (void)data;
}

GL_API void GL_APIENTRY glCompressedTexSubImage2D(GLenum target,
                                                  GLint level,
                                                  GLint xoff,
                                                  GLint yoff,
                                                  GLsizei w,
                                                  GLsizei h,
                                                  GLenum format,
                                                  GLsizei imageSize,
                                                  const void *data)
{
    assert(0 && "glCompressedTexSubImage2D not implemented yet");
    (void)target;
    (void)level;
    (void)xoff;
    (void)yoff;
    (void)w;
    (void)h;
    (void)format;
    (void)imageSize;
    (void)data;
}

GL_API void GL_APIENTRY glCopyTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    assert(0 && "glCopyTexImage2D not implemented yet");
    (void)target;
    (void)level;
    (void)internalformat;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)border;
}

GL_API void GL_APIENTRY
glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h)
{
    assert(0 && "glCopyTexSubImage2D not implemented yet");
    (void)target;
    (void)level;
    (void)xoff;
    (void)yoff;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
}

void gliTextureFlush(void)
{
    gli_context_t *context = gliGetContext();

    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];
        texture_object_t *texture_object = texture_unit->bound_texture_object;
        xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;

        if (!texture_object->texture_object_dirty && !texture_unit->texture_unit_dirty) {
            continue;
        }

        // If the texture unit is disabled or there is no texture bound, skip
        if (!texture_unit->texture_2d_enabled || !xgu_texture ||
            texture_object == &texture_unit->unbound_texture_object) {
            uint32_t *pb = pb_begin();
            pb = xgu_set_texture_control0(pb, i, false, 0, 0);
            pb = xgu_set_texture_matrix_enable(pb, i, false);
            pb_end(pb);

            texture_unit->texture_unit_dirty = GL_TRUE;
            continue;
        }

        // Promote linear texture to swizzled if non-clamp wrap mode is requested
        if (!xgu_texture->swizzled &&
            (texture_object->wrap_s != GL_CLAMP_TO_EDGE || texture_object->wrap_t != GL_CLAMP_TO_EDGE)) {
            gliPromoteTextureToSwizzled(texture_object);
            xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
        }

        // Update the texture metadata if needed
        XguTextureAddress u = gliEnumToNvAddressMode(texture_object->wrap_s);
        XguTextureAddress v = gliEnumToNvAddressMode(texture_object->wrap_t);
        const XguTextureAddress p = XGU_CLAMP_TO_EDGE;

        // Linear textures are limited to edge clamp
        if (!xgu_texture->swizzled) {
            u = XGU_CLAMP_TO_EDGE;
            v = XGU_CLAMP_TO_EDGE;
        }
        const XguTexFilter min_filter = gliEnumToNvTexFilter(texture_object->min_filter);
        const XguTexFilter mag_filter = gliEnumToNvTexFilter(texture_object->mag_filter);

        uint32_t *pb = pb_begin();
        pb = xgu_set_texture_offset(pb, i, xgu_texture->data_physical_address);
        pb = xgu_set_texture_format(pb,
                                    i,
                                    2,
                                    false,
                                    XGU_SOURCE_COLOR,
                                    2,
                                    xgu_texture->format,
                                    xgu_texture->mipmap_levels,
                                    __builtin_ctz(xgu_texture->data_width),  // Only used for swizzled
                                    __builtin_ctz(xgu_texture->data_height), // Only used for swizzled
                                    0);

        uint16_t max_lod = (xgu_texture->mipmap_levels > 0) ? (xgu_texture->mipmap_levels - 1) << 8 : 0;
        pb = xgu_set_texture_control0(pb, i, true, 0, max_lod);
        if (!xgu_texture->swizzled) {
            pb = xgu_set_texture_control1(pb, i, xgu_texture->pitch);                                // NPOT PITCH
            pb = xgu_set_texture_image_rect(pb, i, xgu_texture->tex_width, xgu_texture->tex_height); // NPOT SIZE
        }
        pb = xgu_set_texture_filter(
            pb, i, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, min_filter, mag_filter, false, false, false, false);
        pb = xgu_set_texture_address(pb, i, u, false, v, false, p, false, false);
        pb_end(pb);
    }

    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];
        if (texture_unit->bound_texture_object) {
            texture_unit->bound_texture_object->texture_object_dirty = GL_FALSE;
        }
    }

    combiner_set_texture_env();

    // After combiners have evaluated texture_unit_dirty, clear the flag
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        context->texture_environment.texture_units[i].texture_unit_dirty = GL_FALSE;
    }
}
