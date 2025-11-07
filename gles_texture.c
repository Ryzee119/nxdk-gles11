// FIXME GL_POINT_SPRITE_OES
#include "gles_private.h"

static texture_object_t *find_texture_object(GLuint name, texture_object_t **prev)
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
        // Surely we will never wrap around, but skip zero if we so
        // Fixme, wrap around could cause duplicate names
        if (next == 0) {
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
        return;
    }

    // First check if the object already exists, if so just bind it to the texture_unit and we are done
    texture_object_t *texture_object = find_texture_object(texture, NULL);
    if (texture_object != NULL) {
        texture_unit->texture_binding_2d = texture;
        texture_unit->bound_texture_object = texture_object;
        texture_unit->bound_texture_object->texture_object_dirty = GL_TRUE;
        return;
    }

    // It's a new object, so we create it
    texture_object = GLI_MALLOC(sizeof(texture_object_t));
    if (texture_object == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    // Initialize the texture object
    memset(texture_object, 0, sizeof(texture_object_t));
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
        texture_object_t *texture_object = find_texture_object(name, &prev);
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

            // FIXME Also delete any texture data associated with the object
            GLI_FREE(texture_object);
        }
    }
}

GL_API GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
    gli_context_t *context = gliGetContext();
    texture_object_t *texture_object = find_texture_object(texture, NULL);
    if (texture == 0 || texture_object == NULL) {
        return GL_FALSE;
    }
    return GL_TRUE;
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
            if (params[0] != GL_CLAMP_TO_EDGE && params[0] != GL_REPEAT) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            texture_object->wrap_s = params[0];
            break;
        case GL_TEXTURE_WRAP_T:
            if (params[0] != GL_CLAMP_TO_EDGE && params[0] != GL_REPEAT) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            texture_object->wrap_t = params[0];
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
            texture_unit->coord_replace_oes = (params[0]) ? GL_TRUE : GL_FALSE;
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
            params[0] = texture_unit->coord_replace_oes ? 1 : 0;
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

void convert_rgb_expand_swizzle(const uint8_t *src,
                                uint8_t *dst,
                                int width,
                                int height,
                                int src_pitch,
                                int dst_pitch,
                                int r_dst,
                                int g_dst,
                                int b_dst,
                                int a_dst,
                                uint8_t a_value)
{
    for (int y = 0; y < height; y++) {
        const uint8_t *s = src + y * src_pitch;
        uint8_t *d = dst + y * dst_pitch;

        for (int x = 0; x < width; x++) {
            uint8_t r = s[0];
            uint8_t g = s[1];
            uint8_t b = s[2];

            d[r_dst] = r;
            d[g_dst] = g;
            d[b_dst] = b;
            d[a_dst] = a_value;

            s += 3;
            d += 4;
        }
    }
}

static inline void rgb_to_rgba_opaque(const uint8_t *restrict src, uint8_t *restrict dst, size_t pixel_count)
{
    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t r = src[0];
        uint8_t g = src[1];
        uint8_t b = src[2];

        dst[0] = r;
        dst[1] = g;
        dst[2] = b;
        dst[3] = 0xFF;

        src += 3;
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
    // FIXME mipmap levels
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

    xgu_texture_t *xgu_texture = GLI_MALLOC(sizeof(xgu_texture_t));
    if (xgu_texture == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    GLuint bytes_per_pixel = 0;
    XguTexFormatColor xgu_format = _gl_enum_to_xgu_tex_format(format, type, &bytes_per_pixel);
    assert(xgu_format != 0);
    assert(bytes_per_pixel != 0);

    xgu_texture->data_width = npot2pot(width);
    xgu_texture->data_height = npot2pot(height);
    xgu_texture->tex_width = width;
    xgu_texture->tex_height = height;
    xgu_texture->bytes_per_pixel = bytes_per_pixel;
    xgu_texture->pitch = xgu_texture->data_width * bytes_per_pixel;
    xgu_texture->u_scale = (float)xgu_texture->tex_width / (float)xgu_texture->data_width;
    xgu_texture->v_scale = (float)xgu_texture->tex_height / (float)xgu_texture->data_height;
    xgu_texture->format = xgu_format;
    xgu_texture->data = MmAllocateContiguousMemoryEx(
        xgu_texture->pitch * xgu_texture->data_height, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (xgu_texture->data == NULL) {
        GLI_FREE(xgu_texture);
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);

    if (pixels != NULL) {

        const GLint alignment = context->pixel_store.unpack_alignment;
        const size_t src_pitch =
            (((size_t)width * (size_t)bytes_per_pixel) + (alignment - 1)) & ~(size_t)(alignment - 1);

        if (format == GL_RGB && type == GL_UNSIGNED_BYTE) {
            GLubyte *src_pixels = (GLubyte *)pixels;
            GLubyte *dst_pixels = GLI_MALLOC(src_pitch * height);
            if (dst_pixels == NULL) {
                MmFreeContiguousMemory(xgu_texture->data);
                GLI_FREE(xgu_texture);
                gliSetError(GL_OUT_OF_MEMORY);
                return;
            }
            rgb_to_rgba_opaque(src_pixels, dst_pixels, width * height);
            swizzle_rect(dst_pixels,
                         xgu_texture->tex_width,
                         xgu_texture->tex_height,
                         xgu_texture->data,
                         src_pitch,
                         xgu_texture->bytes_per_pixel);
            GLI_FREE(dst_pixels);
        } else {
            swizzle_rect(pixels,
                         xgu_texture->tex_width,
                         xgu_texture->tex_height,
                         xgu_texture->data,
                         src_pitch,
                         xgu_texture->bytes_per_pixel);
        }
    }

    // Bind the texture to the currently bound texture object
    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    texture_object_t *texture_object = texture_unit->bound_texture_object;
    texture_object->texture_2d = xgu_texture;
    texture_object->texture_object_dirty = GL_TRUE;
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
    assert(0 && "glTexSubImage2D not implemented yet");
    (void)target;
    (void)level;
    (void)xoff;
    (void)yoff;
    (void)w;
    (void)h;
    (void)format;
    (void)type;
    (void)pixels;
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

    // Update texture bindings
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];
        texture_object_t *texture_object = texture_unit->bound_texture_object;

        if (!texture_object->texture_object_dirty) {
            continue;
        }

        if (!texture_unit->texture_2d_enabled) {
            uint32_t *pb = pb_begin();
            pb = xgu_set_texture_control0(pb, i, 0, 0, 0);
            pb_end(pb);
            continue;
        }

        xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
        assert(xgu_texture != NULL);

        XguTextureAddress u = _gl_wrap_to_xgu_address_mode(texture_object->wrap_s);
        XguTextureAddress v = _gl_wrap_to_xgu_address_mode(texture_object->wrap_t);
        XguTexFilter min_filter = _gl_filter_to_xgu_tex_filter(texture_object->min_filter);
        XguTexFilter mag_filter = _gl_filter_to_xgu_tex_filter(texture_object->mag_filter);

        uint32_t *pb = pb_begin();
        pb = xgu_set_texture_offset(pb, i, xgu_texture->data_physical_address);
        pb = xgu_set_texture_format(pb,
                                    i,
                                    2,
                                    false,
                                    XGU_SOURCE_COLOR,
                                    2,
                                    xgu_texture->format,
                                    1,
                                    __builtin_ctz(xgu_texture->data_width),
                                    __builtin_ctz(xgu_texture->data_height),
                                    0);
        pb = xgu_set_texture_control0(pb, i, true, 0, 0);
        pb = xgu_set_texture_control1(pb, i, xgu_texture->pitch);
        pb = xgu_set_texture_image_rect(pb, i, xgu_texture->tex_width, xgu_texture->tex_height);
        pb = xgu_set_texture_filter(
            pb, i, 0, XGU_TEXTURE_CONVOLUTION_GAUSSIAN, min_filter, mag_filter, false, false, false, false);
        pb = xgu_set_texture_address(pb, i, u, (u == XGU_WRAP), v, (v == XGU_WRAP), XGU_CLAMP_TO_EDGE, false, false);
        pb_end(pb);
    }

    combiner_set_texture_env();
}
