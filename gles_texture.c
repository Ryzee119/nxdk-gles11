#define GL_GLEXT_PROTOTYPES
#include "gles_private.h"
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

    // Fast path: if this texture is already bound to this texture unit, avoid O(N) list traversal
    if (texture_unit->texture_binding_2d == texture && texture_unit->bound_texture_object != NULL) {
        return;
    }

    const GLboolean prev_has_tex =
        (texture_unit->bound_texture_object != &texture_unit->unbound_texture_object &&
         texture_unit->bound_texture_object != NULL && texture_unit->bound_texture_object->texture_2d != NULL);
    const xgu_texture_t *prev_xgu =
        prev_has_tex ? (const xgu_texture_t *)texture_unit->bound_texture_object->texture_2d : NULL;
    const GLfloat prev_u_scale = prev_xgu ? prev_xgu->u_scale : 1.0f;
    const GLfloat prev_v_scale = prev_xgu ? prev_xgu->v_scale : 1.0f;
    // The combiners route texture alpha differently for formats without alpha (see combiner_set_texture_env)
    const GLboolean prev_has_alpha =
        prev_has_tex && gliTextureFormatHasAlpha(texture_unit->bound_texture_object->internalformat);

    // If the texture name is zero just unbind any texture currently bound to the target
    if (texture == 0) {
        texture_unit->texture_binding_2d = 0;
        texture_unit->bound_texture_object = &texture_unit->unbound_texture_object;
        texture_unit->bound_texture_object->texture_object_dirty = GL_TRUE;
        if (prev_has_tex) {
            context->texture_environment.combiner_dirty = GL_TRUE;
            context->transformation_state.texture_matrix_dirty[texture_index] = GL_TRUE;
        }
        return;
    }

    // First check if the object already exists, if so just bind it to the texture_unit and we are done
    texture_object_t *texture_object = gliFindTextureObject(texture, NULL);
    if (texture_object != NULL) {
        texture_unit->texture_binding_2d = texture;
        texture_unit->bound_texture_object = texture_object;
        texture_unit->bound_texture_object->texture_object_dirty = GL_TRUE;

        const GLboolean has_tex = (texture_object->texture_2d != NULL);
        const GLboolean has_alpha = has_tex && gliTextureFormatHasAlpha(texture_object->internalformat);
        if (has_tex != prev_has_tex || has_alpha != prev_has_alpha) {
            context->texture_environment.combiner_dirty = GL_TRUE;
        }

        const xgu_texture_t *cur_xgu = has_tex ? (const xgu_texture_t *)texture_object->texture_2d : NULL;
        const GLfloat cur_u_scale = cur_xgu ? cur_xgu->u_scale : 1.0f;
        const GLfloat cur_v_scale = cur_xgu ? cur_xgu->v_scale : 1.0f;
        if (!prev_has_tex || cur_u_scale != prev_u_scale || cur_v_scale != prev_v_scale) {
            context->transformation_state.texture_matrix_dirty[texture_index] = GL_TRUE;
        }
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
    if (prev_has_tex) {
        context->texture_environment.combiner_dirty = GL_TRUE;
    }
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
            break;
        case GL_TEXTURE_WRAP_T:
            if (params[0] != GL_CLAMP_TO_EDGE && params[0] != GL_REPEAT && params[0] != GL_MIRRORED_REPEAT_OES) {
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
            texture_unit->coord_replace_oes_enabled = (params[0]) ? GL_TRUE : GL_FALSE;
            break;

        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
    texture_unit->texture_unit_dirty = GL_TRUE;
    context->texture_environment.combiner_dirty = GL_TRUE;
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
            context->texture_environment.combiner_dirty = GL_TRUE;
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

static size_t texture_swizzled_pixel_offset(GLuint width, GLuint height, GLuint x, GLuint y)
{
    GLuint offset = 0;
    GLuint source_bit = 1;
    GLuint destination_bit = 1;

    while (source_bit < width || source_bit < height) {
        if (source_bit < width) {
            if (x & source_bit) {
                offset |= destination_bit;
            }
            destination_bit <<= 1;
        }
        if (source_bit < height) {
            if (y & source_bit) {
                offset |= destination_bit;
            }
            destination_bit <<= 1;
        }
        source_bit <<= 1;
    }

    return offset;
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
        if (!xgu_texture->swizzled || internalformat != texture_object->internalformat) {
            gliSetError(GL_INVALID_OPERATION); // Mipmaps require POT swizzled textures and matching internalformat
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

            GLuint base_size = xgu_texture->data_width * xgu_texture->data_height * xgu_texture->bytes_per_pixel;
            gli_memcpy(new_data, xgu_texture->data, base_size);

            MmFreeContiguousMemory(xgu_texture->data);
            xgu_texture->data = new_data;
            xgu_texture->data_size = required_size;
            xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);
            xgu_texture->mipmap_levels = required_levels;
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
            const GLint alignment = context->pixel_store.unpack_alignment;
            const size_t src_pitch =
                (((size_t)width * (size_t)xgu_texture->bytes_per_pixel) + (alignment - 1)) & ~(size_t)(alignment - 1);

            const GLubyte *src_pixels = (GLubyte *)pixels;
            if (type == GL_UNSIGNED_BYTE && (format == GL_RGB || format == GL_RGBA)) {
                GLubyte *converted_pixels = GLI_MALLOC(src_pitch * height);
                if (converted_pixels == NULL) {
                    gliSetError(GL_OUT_OF_MEMORY);
                    return;
                }
                convert_to_bgra(src_pixels, converted_pixels, width * height, (format == GL_RGBA) ? 4 : 3);
                src_pixels = converted_pixels;
            }

            swizzle_rect(
                src_pixels, width, height, xgu_texture->data + level_offset, src_pitch, xgu_texture->bytes_per_pixel);

            if (src_pixels != (GLubyte *)pixels) {
                GLI_FREE((void *)src_pixels);
            }
        }

        texture_object->texture_object_dirty = GL_TRUE;
        return;
    }

    // level == 0
    if (width == 0 || height == 0) {
        // A zero-sized image is legal and leaves the texture without image data (incomplete).
        // Handled here because the storage code below (npot2pot, ctz) is undefined for 0.
        if (texture_object->texture_2d != NULL) {
            xgu_texture_t *old_tex = (xgu_texture_t *)texture_object->texture_2d;
            if (old_tex->data) {
                MmFreeContiguousMemory(old_tex->data);
            }
            GLI_FREE(old_tex);
        }
        texture_object->texture_2d = NULL;
        texture_object->internalformat = internalformat;
        texture_object->texture_object_dirty = GL_TRUE;
        context->texture_environment.combiner_dirty = GL_TRUE;
        return;
    }

    xgu_texture_t *xgu_texture = GLI_MALLOC(sizeof(xgu_texture_t));
    if (xgu_texture == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }
    gli_memset(xgu_texture, 0, sizeof(xgu_texture_t));

    xgu_texture->swizzled = 0;

    // If the provided width and height are already powers of two, we swizzle the texture to get better performance
    if (width == npot2pot(width) && height == npot2pot(height)) {
        xgu_texture->swizzled = 1;
    }

    GLuint bytes_per_pixel = 0;
    XguTexFormatColor xgu_format = gliEnumToNvTexFormat(format, type, &bytes_per_pixel, xgu_texture->swizzled);
    assert(xgu_format != -1);
    assert(bytes_per_pixel != 0);

    // Swizzled textures must be a power of 2 and texture coordinates normalized to [0, 1]
    if (xgu_texture->swizzled) {
        xgu_texture->data_width = npot2pot(width);
        xgu_texture->data_height = npot2pot(height);
        xgu_texture->u_scale = 1.0f;
        xgu_texture->v_scale = 1.0f;
    } else {
        xgu_texture->data_width = width;
        xgu_texture->data_height = height;
        xgu_texture->u_scale = (GLfloat)xgu_texture->data_width;
        xgu_texture->v_scale = (GLfloat)xgu_texture->data_height;
    }
    xgu_texture->tex_width = width;
    xgu_texture->tex_height = height;
    xgu_texture->bytes_per_pixel = bytes_per_pixel;
    xgu_texture->mipmap_levels = 1;

    // Width must be atleast 8 pixels
    xgu_texture->data_width = GLI_MAX(8, xgu_texture->data_width);
    xgu_texture->data_height = GLI_MAX(8, xgu_texture->data_height);

    if (xgu_texture->swizzled) {
        xgu_texture->pitch = xgu_texture->data_width * bytes_per_pixel;
    } else {
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

    xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);

    if (pixels != NULL) {
        const GLint alignment = context->pixel_store.unpack_alignment;
        const size_t src_pitch =
            (((size_t)width * (size_t)bytes_per_pixel) + (alignment - 1)) & ~(size_t)(alignment - 1);

        const GLubyte *src_pixels = (GLubyte *)pixels;
        GLubyte *dst_pixels = xgu_texture->data;

        if (type == GL_UNSIGNED_BYTE && (format == GL_RGB || format == GL_RGBA)) {
            GLubyte *converted_pixels = GLI_MALLOC(src_pitch * height);
            if (converted_pixels == NULL) {
                gliSetError(GL_OUT_OF_MEMORY);
                MmFreeContiguousMemory(xgu_texture->data);
                GLI_FREE(xgu_texture);
                return;
            }
            convert_to_bgra(src_pixels, converted_pixels, width * height, (format == GL_RGBA) ? 4 : 3);
            src_pixels = converted_pixels;
        }

        if (xgu_texture->swizzled) {
            swizzle_rect(src_pixels,
                         xgu_texture->tex_width,
                         xgu_texture->tex_height,
                         dst_pixels,
                         src_pitch,
                         xgu_texture->bytes_per_pixel);
        } else {
            for (GLuint y = 0; y < (GLuint)height; y++) {
                gli_memcpy(dst_pixels + y * xgu_texture->pitch, src_pixels + y * src_pitch, width * bytes_per_pixel);
            }
        }

        if (src_pixels != (GLubyte *)pixels) {
            GLI_FREE((void *)src_pixels);
        }
    } else {
        gli_memset(xgu_texture->data, 0, xgu_texture->pitch * xgu_texture->data_height);
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
    // The combiner's active stages depend on whether the texture has image data
    context->texture_environment.combiner_dirty = GL_TRUE;
    // u_scale/v_scale may have changed (POT swizzled vs NPOT linear), and they are folded into the texture matrix
    context->transformation_state.texture_matrix_dirty[texture_index] = GL_TRUE;

    if (xgu_texture->swizzled && texture_object->generate_mipmap) {
        gliGenSwizzledMipmaps(xgu_texture);
    }

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

    if ((type == GL_UNSIGNED_SHORT_5_6_5 && format != GL_RGB) ||
        ((type == GL_UNSIGNED_SHORT_4_4_4_4 || type == GL_UNSIGNED_SHORT_5_5_5_1) && format != GL_RGBA)) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (level < 0 || xoff < 0 || yoff < 0 || w < 0 || h < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_unit_t *texture_unit = &context->texture_environment.texture_units[texture_index];
    texture_object_t *texture_object = texture_unit->bound_texture_object;
    if (texture_object == NULL || texture_object->texture_name == 0 || texture_object->texture_2d == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    // No format conversion is done, the data must match the texture's format
    if (format != (GLenum)texture_object->internalformat) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
    if (level >= xgu_texture->mipmap_levels || (level > 0 && !xgu_texture->swizzled)) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    GLuint storage_width = xgu_texture->data_width;
    GLuint storage_height = xgu_texture->data_height;
    size_t level_offset = 0;
    for (GLint i = 0; i < level; i++) {
        level_offset += (size_t)storage_width * storage_height * xgu_texture->bytes_per_pixel;
        if (storage_width > 1) {
            storage_width /= 2;
        }
        if (storage_height > 1) {
            storage_height /= 2;
        }
    }

    GLuint level_width = storage_width;
    GLuint level_height = storage_height;
    if (level == 0) {
        // glTexImage2D swizzles level 0 at its real size, not the 8 texel minimum storage size
        level_width = storage_width = xgu_texture->tex_width;
        level_height = storage_height = xgu_texture->tex_height;
    }

    if ((GLuint)w > level_width || (GLuint)h > level_height || (GLuint)xoff > level_width - (GLuint)w ||
        (GLuint)yoff > level_height - (GLuint)h) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint storage_bytes_per_pixel = 0;
    XguTexFormatColor requested_format =
        gliEnumToNvTexFormat(format, type, &storage_bytes_per_pixel, xgu_texture->swizzled);
    if (requested_format == (XguTexFormatColor)-1 || requested_format != xgu_texture->format ||
        storage_bytes_per_pixel != (GLuint)xgu_texture->bytes_per_pixel) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (w == 0 || h == 0) {
        return;
    }
    if (pixels == NULL) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint source_bytes_per_pixel;
    if (type == GL_UNSIGNED_BYTE) {
        if (format == GL_RGB) {
            source_bytes_per_pixel = 3;
        } else if (format == GL_RGBA) {
            source_bytes_per_pixel = 4;
        } else if (format == GL_LUMINANCE_ALPHA) {
            source_bytes_per_pixel = 2;
        } else {
            source_bytes_per_pixel = 1;
        }
    } else {
        source_bytes_per_pixel = 2;
    }

    GLint alignment = context->pixel_store.unpack_alignment;
    size_t source_row_bytes = (size_t)w * source_bytes_per_pixel;
    size_t source_pitch = (source_row_bytes + (size_t)alignment - 1) & ~((size_t)alignment - 1);
    const GLubyte *source = (const GLubyte *)pixels;
    GLubyte *destination = xgu_texture->data + level_offset;

    for (GLint row = 0; row < h; row++) {
        const GLubyte *source_row = source + (size_t)row * source_pitch;
        for (GLint column = 0; column < w; column++) {
            GLubyte *destination_pixel = NULL;
            if (xgu_texture->swizzled) {
                size_t swizzle_index = texture_swizzled_pixel_offset(
                    storage_width, storage_height, (GLuint)(xoff + column), (GLuint)(yoff + row));
                destination_pixel = destination + swizzle_index * xgu_texture->bytes_per_pixel;
            } else {
                destination_pixel = destination + (size_t)(yoff + row) * xgu_texture->pitch +
                                    (size_t)(xoff + column) * xgu_texture->bytes_per_pixel;
            }

            const GLubyte *source_pixel = source_row + (size_t)column * source_bytes_per_pixel;
            if (type == GL_UNSIGNED_BYTE && (format == GL_RGB || format == GL_RGBA)) {
                destination_pixel[0] = source_pixel[2];
                destination_pixel[1] = source_pixel[1];
                destination_pixel[2] = source_pixel[0];
                destination_pixel[3] = (format == GL_RGBA) ? source_pixel[3] : 0xFF;
            } else {
                gli_memcpy(destination_pixel, source_pixel, storage_bytes_per_pixel);
            }
        }
    }

    texture_object->texture_object_dirty = GL_TRUE;

    if (level == 0 && xgu_texture->swizzled && texture_object->generate_mipmap) {
        // The texture may have been created before GL_GENERATE_MIPMAP was enabled, so the mip chain
        // storage may not exist yet. glGenerateMipmapOES (re)allocates it before generating.
        glGenerateMipmapOES(GL_TEXTURE_2D);
    }
}

static inline GLuint gliCompressedBlockSize(GLenum format)
{
    return (format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
}

static inline GLsizei gliCompressedImageSize(GLsizei width, GLsizei height, GLenum format)
{
    if (width <= 0 || height <= 0) {
        return 0;
    }
    return ((width + 3) / 4) * ((height + 3) / 4) * (GLsizei)gliCompressedBlockSize(format);
}

static inline XguTexFormatColor gliCompressedToNvFormat(GLenum format)
{
    switch (format) {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return XGU_TEXTURE_FORMAT_DXT1;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            return XGU_TEXTURE_FORMAT_DXT3;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return XGU_TEXTURE_FORMAT_DXT5;
        default:
            return (XguTexFormatColor)-1;
    }
}

static GLuint compressed_level_dim_and_offset(
    const xgu_texture_t *tex, GLint level, GLenum format, GLuint *out_w, GLuint *out_h)
{
    GLuint w = tex->tex_width;
    GLuint h = tex->tex_height;
    GLuint offset = 0;
    for (GLint i = 0; i < level; i++) {
        offset += (GLuint)gliCompressedImageSize((GLsizei)w, (GLsizei)h, format);
        if (w > 1) {
            w /= 2;
        }
        if (h > 1) {
            h /= 2;
        }
    }
    *out_w = w;
    *out_h = h;
    return offset;
}

// OES_compressed_paletted_texture (core in ES 1.1). NV2A can't sample these, so each level is expanded
// on the CPU and uploaded through glTexImage2D. The data is the palette followed by the indices of each mip level,
// tightly packed starting at the base level. 4-bit indices hold the first texel in the high nibble.
static void compressed_paletted_tex_image(
    GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei imageSize, const void *data)
{
    static const struct
    {
        GLenum format;
        GLenum type;
        GLuint entry_size;
    } entry_formats[] = {
        {GL_RGB,  GL_UNSIGNED_BYTE,          3},
        {GL_RGBA, GL_UNSIGNED_BYTE,          4},
        {GL_RGB,  GL_UNSIGNED_SHORT_5_6_5,   2},
        {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 2},
        {GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 2},
    };
    gli_context_t *context = gliGetContext();
    const GLuint index = internalformat - GL_PALETTE4_RGB8_OES;
    const GLboolean four_bit = index < 5;
    const GLenum format = entry_formats[index % 5].format;
    const GLenum type = entry_formats[index % 5].type;
    const GLuint entry_size = entry_formats[index % 5].entry_size;
    const size_t palette_size = (size_t)(four_bit ? 16 : 256) * entry_size;

    // level must be <= 0; -level is the number of mip levels after the base level in data.
    // A mip chain needs a power-of-two base level and can't go past 1x1.
    if (level > 0 || level < -31) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    const GLint num_levels = 1 - level;
    if (num_levels > 1 && (width == 0 || height == 0 || (width & (width - 1)) || (height & (height - 1)) ||
                           (GLI_MAX(width, height) >> (num_levels - 1)) == 0)) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    size_t expected_size = palette_size;
    for (GLint i = 0; i < num_levels; i++) {
        const size_t texels = (width && height) ? (size_t)GLI_MAX(1, width >> i) * GLI_MAX(1, height >> i) : 0;
        expected_size += four_bit ? (texels + 1) / 2 : texels;
    }
    if ((size_t)imageSize != expected_size) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_object_t *texture_object = context->texture_environment.texture_units[texture_index].bound_texture_object;
    const GLenum pending_error = context->last_error;
    const GLint old_unpack = context->pixel_store.unpack_alignment;
    const GLboolean old_gen_mipmap = texture_object->generate_mipmap;
    context->last_error = GL_NO_ERROR;
    context->pixel_store.unpack_alignment = 1;
    texture_object->generate_mipmap = GL_FALSE;

    const GLubyte *palette = (const GLubyte *)data;
    const GLubyte *indices = palette ? palette + palette_size : NULL;
    for (GLint i = 0; i < num_levels; i++) {
        const GLsizei w = (width && height) ? GLI_MAX(1, width >> i) : 0;
        const GLsizei h = (width && height) ? GLI_MAX(1, height >> i) : 0;
        const size_t texels = (size_t)w * (size_t)h;

        GLubyte *image = NULL;
        if (indices && texels) {
            image = GLI_MALLOC(texels * entry_size);
            if (!image) {
                gliSetError(GL_OUT_OF_MEMORY);
                break;
            }
            for (size_t t = 0; t < texels; t++) {
                const GLuint entry = four_bit ? ((indices[t / 2] >> ((t & 1) ? 0 : 4)) & 0xF) : indices[t];
                gli_memcpy(image + t * entry_size, palette + entry * entry_size, entry_size);
            }
            indices += four_bit ? (texels + 1) / 2 : texels;
        }

        glTexImage2D(GL_TEXTURE_2D, i, format, w, h, 0, format, type, image);
        if (image) {
            GLI_FREE(image);
        }
        if (context->last_error != GL_NO_ERROR) {
            break;
        }
    }

    context->pixel_store.unpack_alignment = old_unpack;
    texture_object->generate_mipmap = old_gen_mipmap;
    if (context->last_error == GL_NO_ERROR) {
        // Keep the compressed format so TexSubImage2D, CopyTexSubImage2D and mipmap generation are rejected
        texture_object->internalformat = internalformat;
    }
    if (pending_error != GL_NO_ERROR) {
        context->last_error = pending_error;
    }
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
    gli_context_t *context = gliGetContext();

    if (target != GL_TEXTURE_2D) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (!gliIsCompressedFormat(internalformat)) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (border != 0 || width < 0 || height < 0 || imageSize < 0 || width > GLI_MAX_TEXTURE_SIZE ||
        height > GLI_MAX_TEXTURE_SIZE) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // Paletted formats use a non-positive level to encode the number of mip levels
    if (gliIsPalettedFormat(internalformat)) {
        compressed_paletted_tex_image(level, internalformat, width, height, imageSize, data);
        return;
    }

    if (level < 0 || (width & (width - 1)) || (height & (height - 1)) ||
        imageSize != gliCompressedImageSize(width, height, internalformat)) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_object_t *texture_object = context->texture_environment.texture_units[texture_index].bound_texture_object;

    if (level > 0) {
        xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
        if (!xgu_texture || internalformat != texture_object->internalformat) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }

        GLuint expected_w, expected_h;
        GLuint level_offset =
            compressed_level_dim_and_offset(xgu_texture, level, internalformat, &expected_w, &expected_h);
        if ((GLuint)width != expected_w || (GLuint)height != expected_h) {
            gliSetError(GL_INVALID_VALUE);
            return;
        }

        GLuint required_size;
        uint8_t required_levels;
        gliCalcCompressedMipmapChain(xgu_texture->tex_width,
                                     xgu_texture->tex_height,
                                     gliCompressedBlockSize(internalformat),
                                     &required_size,
                                     &required_levels);
        if (level >= (GLint)required_levels) {
            gliSetError(GL_INVALID_VALUE);
            return;
        }

        if (xgu_texture->data_size < required_size) {
            GLubyte *new_data =
                MmAllocateContiguousMemoryEx(required_size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
            if (!new_data) {
                gliSetError(GL_OUT_OF_MEMORY);
                return;
            }

            gli_memcpy(new_data, xgu_texture->data, xgu_texture->data_size);
            gli_memset(new_data + xgu_texture->data_size, 0, required_size - xgu_texture->data_size);
            MmFreeContiguousMemory(xgu_texture->data);
            xgu_texture->data = new_data;
            xgu_texture->data_size = required_size;
            xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);
        }

        if ((uint8_t)(level + 1) > xgu_texture->mipmap_levels) {
            xgu_texture->mipmap_levels = (uint8_t)(level + 1);
        }

        if (data != NULL) {
            gli_memcpy(xgu_texture->data + level_offset, data, (size_t)imageSize);
        } else {
            gli_memset(xgu_texture->data + level_offset, 0, (size_t)imageSize);
        }

        texture_object->texture_object_dirty = GL_TRUE;
        return;
    }

    // level == 0
    if (width == 0 || height == 0) {
        if (texture_object->texture_2d != NULL) {
            xgu_texture_t *old_tex = (xgu_texture_t *)texture_object->texture_2d;
            if (old_tex->data) {
                MmFreeContiguousMemory(old_tex->data);
            }
            GLI_FREE(old_tex);
        }
        texture_object->texture_2d = NULL;
        texture_object->internalformat = internalformat;
        texture_object->texture_object_dirty = GL_TRUE;
        context->texture_environment.combiner_dirty = GL_TRUE;
        return;
    }

    xgu_texture_t *xgu_texture = GLI_MALLOC(sizeof(xgu_texture_t));
    if (xgu_texture == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }
    gli_memset(xgu_texture, 0, sizeof(xgu_texture_t));

    xgu_texture->swizzled = 1;
    xgu_texture->data_width = width;
    xgu_texture->data_height = height;
    xgu_texture->tex_width = width;
    xgu_texture->tex_height = height;
    xgu_texture->bytes_per_pixel = gliCompressedBlockSize(internalformat);
    xgu_texture->pitch = ((width + 3) / 4) * xgu_texture->bytes_per_pixel;
    xgu_texture->u_scale = 1.0f;
    xgu_texture->v_scale = 1.0f;
    xgu_texture->mipmap_levels = 1;
    xgu_texture->format = gliCompressedToNvFormat(internalformat);

    xgu_texture->data_size = (GLuint)imageSize;
    xgu_texture->data =
        MmAllocateContiguousMemoryEx((GLuint)imageSize, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (xgu_texture->data == NULL) {
        GLI_FREE(xgu_texture);
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    xgu_texture->data_physical_address = (GLubyte *)MmGetPhysicalAddress(xgu_texture->data);

    if (data != NULL) {
        gli_memcpy(xgu_texture->data, data, (size_t)imageSize);
    } else {
        gli_memset(xgu_texture->data, 0, (size_t)imageSize);
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
    context->texture_environment.combiner_dirty = GL_TRUE;
    context->transformation_state.texture_matrix_dirty[texture_index] = GL_TRUE;
    texture_object->texture_object_dirty = GL_TRUE;
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
    gli_context_t *context = gliGetContext();

    if (target != GL_TEXTURE_2D || !gliIsCompressedFormat(format)) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // OES_compressed_paletted_texture: paletted images can't be partially updated
    if (gliIsPalettedFormat(format)) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (level < 0 || xoff < 0 || yoff < 0 || w < 0 || h < 0 || imageSize < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_object_t *texture_object = context->texture_environment.texture_units[texture_index].bound_texture_object;
    if (!texture_object || texture_object->texture_name == 0 || !texture_object->texture_2d) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
    if (format != texture_object->internalformat || level >= (GLint)xgu_texture->mipmap_levels) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    GLuint storage_w, storage_h;
    GLuint level_offset = compressed_level_dim_and_offset(xgu_texture, level, format, &storage_w, &storage_h);

    if ((GLuint)w > storage_w || (GLuint)h > storage_h || (GLuint)xoff > storage_w - (GLuint)w ||
        (GLuint)yoff > storage_h - (GLuint)h || imageSize != gliCompressedImageSize(w, h, format)) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if ((xoff & 3) || (yoff & 3) || ((w & 3) && (GLuint)(xoff + w) != storage_w) ||
        ((h & 3) && (GLuint)(yoff + h) != storage_h)) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (w == 0 || h == 0) {
        return;
    }

    if (data == NULL) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint block_size = gliCompressedBlockSize(format);
    size_t dest_pitch_bytes = (size_t)((storage_w + 3) / 4) * block_size;
    size_t src_pitch_bytes = (size_t)((w + 3) / 4) * block_size;
    GLuint sub_blocks_y = (GLuint)((h + 3) / 4);
    GLubyte *dest =
        xgu_texture->data + level_offset + (size_t)(yoff / 4) * dest_pitch_bytes + (size_t)(xoff / 4) * block_size;
    const GLubyte *src = (const GLubyte *)data;

    for (GLuint by = 0; by < sub_blocks_y; by++) {
        gli_memcpy(dest + by * dest_pitch_bytes, src + by * src_pitch_bytes, src_pitch_bytes);
    }

    texture_object->texture_object_dirty = GL_TRUE;
}

// Reads a framebuffer region as tightly packed RGBA (the ES-guaranteed readback format), then repacks it
// in place to the destination base format. Per the CopyTexImage conversion rules L takes R and A takes A.
// The caller must have validated the arguments so that glReadPixels cannot fail, and w, h > 0.
static GLubyte *copy_tex_read_framebuffer(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format)
{
    gli_context_t *context = gliGetContext();
    const size_t count = (size_t)w * (size_t)h;
    GLubyte *buffer = GLI_MALLOC(count * 4);
    if (!buffer) {
        gliSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }
    const GLint old_pack = context->pixel_store.pack_alignment;
    context->pixel_store.pack_alignment = 1;
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    context->pixel_store.pack_alignment = old_pack;

    static const GLubyte alpha[] = {3}, luminance[] = {0}, luminance_alpha[] = {0, 3}, rgb[] = {0, 1, 2};
    const GLubyte *components;
    GLuint n;
    switch (format) {
        case GL_ALPHA:
            components = alpha, n = 1;
            break;
        case GL_LUMINANCE:
            components = luminance, n = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            components = luminance_alpha, n = 2;
            break;
        case GL_RGB:
            components = rgb, n = 3;
            break;
        default:
            return buffer; // GL_RGBA
    }

    // In place is safe: components are increasing and n < 4, so a write never passes an unread byte
    GLubyte *dst = buffer;
    for (size_t i = 0; i < count; i++) {
        const GLubyte *src = buffer + i * 4;
        for (GLuint c = 0; c < n; c++) {
            *dst++ = src[components[c]];
        }
    }
    return buffer;
}

// Validation shared by glCopyTexImage2D/glCopyTexSubImage2D that must happen before reading the framebuffer
static GLboolean copy_tex_check_source(GLsizei w, GLsizei h)
{
    // No texture level can be larger than GL_MAX_TEXTURE_SIZE, this also keeps w * h * 4 from overflowing
    if (w > GLI_MAX_TEXTURE_SIZE || h > GLI_MAX_TEXTURE_SIZE) {
        gliSetError(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    // Only the default framebuffer can be read back (see glReadPixels)
    if (gliGetContext()->fbo_binding != 0) {
        gliSetError(GL_INVALID_OPERATION);
        return GL_FALSE;
    }
    return GL_TRUE;
}

GL_API void GL_APIENTRY glCopyTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    gli_context_t *context = gliGetContext();

    if (target != GL_TEXTURE_2D) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (internalformat != GL_ALPHA && internalformat != GL_LUMINANCE && internalformat != GL_LUMINANCE_ALPHA &&
        internalformat != GL_RGB && internalformat != GL_RGBA) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || border != 0 || width < 0 || height < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (!copy_tex_check_source(width, height)) {
        return;
    }

    // The default framebuffer is A8R8G8B8, so every base format can be produced from it.
    // Any remaining errors are raised by glTexImage2D, the readback itself has no side effects.
    GLubyte *buffer = NULL;
    if (width > 0 && height > 0) {
        buffer = copy_tex_read_framebuffer(x, y, width, height, internalformat);
        if (!buffer) {
            return;
        }
    }
    const GLint old_unpack = context->pixel_store.unpack_alignment;
    context->pixel_store.unpack_alignment = 1;
    glTexImage2D(target, level, internalformat, width, height, 0, internalformat, GL_UNSIGNED_BYTE, buffer);
    context->pixel_store.unpack_alignment = old_unpack;
    if (buffer) {
        GLI_FREE(buffer);
    }
}

GL_API void GL_APIENTRY
glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h)
{
    gli_context_t *context = gliGetContext();

    if (target != GL_TEXTURE_2D) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (w < 0 || h < 0 || xoff < 0 || yoff < 0 || level < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    GLuint texture_index = context->texture_environment.server_active_texture - GL_TEXTURE0;
    texture_object_t *texture_object = context->texture_environment.texture_units[texture_index].bound_texture_object;
    if (texture_object == NULL || texture_object->texture_name == 0 || texture_object->texture_2d == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (gliIsCompressedFormat(texture_object->internalformat)) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (!copy_tex_check_source(w, h)) {
        return;
    }

    // Any remaining errors (level, region bounds, storage format) are raised by glTexSubImage2D,
    // the readback itself has no side effects. A zero-sized copy only validates.
    const GLenum format = (GLenum)texture_object->internalformat;
    GLubyte *buffer = NULL;
    if (w > 0 && h > 0) {
        buffer = copy_tex_read_framebuffer(x, y, w, h, format);
        if (!buffer) {
            return;
        }
    }
    const GLint old_unpack = context->pixel_store.unpack_alignment;
    context->pixel_store.unpack_alignment = 1;
    glTexSubImage2D(target, level, xoff, yoff, w, h, format, GL_UNSIGNED_BYTE, buffer);
    context->pixel_store.unpack_alignment = old_unpack;
    if (buffer) {
        GLI_FREE(buffer);
    }
}

void gliTextureFlush(void)
{
    gli_context_t *context = gliGetContext();

    uint32_t *pb = NULL;
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];
        texture_object_t *texture_object = texture_unit->bound_texture_object;
        if (!texture_object) {
            texture_object = &texture_unit->unbound_texture_object;
            texture_unit->bound_texture_object = texture_object;
        }
        xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;

        if (!texture_object->texture_object_dirty && !texture_unit->texture_unit_dirty) {
            continue;
        }

        // If the texture unit is disabled or there is no texture bound, skip
        if (!texture_unit->texture_2d_enabled || !xgu_texture ||
            texture_object == &texture_unit->unbound_texture_object) {
            if (!pb) {
                pb = pb_begin();
            }
            pb = xgu_set_texture_control0(pb, i, false, 0, 0);
            pb = xgu_set_texture_matrix_enable(pb, i, false);

            texture_unit->texture_unit_dirty = GL_TRUE;
            context->texture_environment.combiner_dirty = GL_TRUE;
            continue;
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

        if (!pb) {
            pb = pb_begin();
        }
        pb = xgu_set_texture_offset(pb, i, xgu_texture->data_physical_address);
        pb = xgu_set_texture_format(pb,
                                    i,
                                    2,
                                    false,
                                    XGU_SOURCE_COLOR,
                                    2,
                                    xgu_texture->format,
                                    xgu_texture->mipmap_levels,
                                    __builtin_ctz(xgu_texture->tex_width),  // Only used for swizzled
                                    __builtin_ctz(xgu_texture->tex_height), // Only used for swizzled
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
    }
    if (pb) {
        pb_end(pb);
    }

    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];
        if (texture_unit->bound_texture_object) {
            texture_unit->bound_texture_object->texture_object_dirty = GL_FALSE;
        }
    }

    // Only rebuild NV2A register combiners when texture env state actually changed
    if (context->texture_environment.combiner_dirty) {
        combiner_set_texture_env();
        context->texture_environment.combiner_dirty = GL_FALSE;
    }

    // After combiners have evaluated texture_unit_dirty, clear the flag
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        context->texture_environment.texture_units[i].texture_unit_dirty = GL_FALSE;
    }
}
