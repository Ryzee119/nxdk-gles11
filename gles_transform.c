// gles_transform.c
#include "gles_private.h"

static mat4 *current_stack_base(GLint **depth, GLint *max_depth)
{
    gli_context_t *c = gliGetContext();
    transformation_state_t *ts = &c->transformation_state;
    switch (ts->matrix_mode) {
        case GL_MODELVIEW:
            *depth = &ts->modelview_matrix_stack_depth;
            *max_depth = GLI_MAX_MODELVIEW_STACK;
            ts->modelview_matrix_dirty = GL_TRUE;
            return ts->modelview_matrix_stack;
        case GL_PROJECTION:
            *depth = &ts->projection_matrix_stack_depth;
            *max_depth = GLI_MAX_PROJECTION_STACK;
            ts->projection_matrix_dirty = GL_TRUE;
            return ts->projection_matrix_stack;
        case GL_TEXTURE: {
            GLint unit = c->texture_environment.server_active_texture - GL_TEXTURE0;
            *depth = &ts->texture_matrix_stack_depth[unit];
            *max_depth = GLI_MAX_TEXTURE_STACK;
            ts->texture_matrix_dirty[unit] = GL_TRUE;
            return ts->texture_matrix_stack[unit];
        }
        default:
            assert(0 && "Invalid matrix mode");
            return NULL;
    }
}

static inline mat4 *current_matrix_ptr(void)
{
    GLint *depth;
    GLint max_depth;
    mat4 *base = current_stack_base(&depth, &max_depth);
    return &base[*depth - 1];
}

// cur = cur * rhs
static inline void post_multiply(mat4 rhs)
{
    mat4 *cur = current_matrix_ptr();
    glm_mat4_mul(*cur, rhs, *cur);
}

GL_API void GL_APIENTRY glMatrixMode(GLenum mode)
{
    gli_context_t *c = gliGetContext();
    if (mode != GL_MODELVIEW && mode != GL_PROJECTION && mode != GL_TEXTURE) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    c->transformation_state.matrix_mode = mode;
}

GL_API void GL_APIENTRY glLoadIdentity(void)
{
    glm_mat4_identity(*current_matrix_ptr());
}

GL_API void GL_APIENTRY glLoadMatrixf(const GLfloat *m)
{
    mat4 m_arg;
    glm_mat4_make(m, m_arg);

    glm_mat4_copy(m_arg, *current_matrix_ptr());
}

GL_API void GL_APIENTRY glLoadMatrixx(const GLfixed *m)
{
    GLfloat mf[16];
    gliFixedMatrixToFloat(m, mf);
    glLoadMatrixf(mf);
}

GL_API void GL_APIENTRY glMultMatrixf(const GLfloat *m)
{
    mat4 m_arg;
    glm_mat4_make(m, m_arg);

    post_multiply(m_arg);
}

GL_API void GL_APIENTRY glMultMatrixx(const GLfixed *m)
{
    GLfloat mf[16];
    gliFixedMatrixToFloat(m, mf);
    glMultMatrixf(mf);
}

GL_API void GL_APIENTRY glPushMatrix(void)
{
    GLint *depth;
    GLint max_depth;
    mat4 *m = current_stack_base(&depth, &max_depth);

    if (*depth >= max_depth) {
        gliSetError(GL_STACK_OVERFLOW);
        return;
    }
    glm_mat4_copy(m[*depth - 1], m[*depth]);
    (*depth)++;
}

GL_API void GL_APIENTRY glPopMatrix(void)
{
    GLint *depth;
    GLint max_depth;
    current_stack_base(&depth, &max_depth);

    if (*depth == 1) {
        gliSetError(GL_STACK_UNDERFLOW);
        return;
    }
    (*depth)--;
}

GL_API void GL_APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    glm_translate(*current_matrix_ptr(), (vec3){x, y, z});
}

GL_API void GL_APIENTRY glTranslatex(GLfixed x, GLfixed y, GLfixed z)
{
    GLfloat xf = gliFixedtoFloat(x);
    GLfloat yf = gliFixedtoFloat(y);
    GLfloat zf = gliFixedtoFloat(z);

    glTranslatef(xf, yf, zf);
}

GL_API void GL_APIENTRY glRotatef(GLfloat a, GLfloat x, GLfloat y, GLfloat z)
{
    glm_rotate(*current_matrix_ptr(), glm_rad(a), (vec3){x, y, z});
}

GL_API void GL_APIENTRY glRotatex(GLfixed a, GLfixed x, GLfixed y, GLfixed z)
{
    GLfloat af = gliFixedtoFloat(a);
    GLfloat xf = gliFixedtoFloat(x);
    GLfloat yf = gliFixedtoFloat(y);
    GLfloat zf = gliFixedtoFloat(z);

    glRotatef(af, xf, yf, zf);
}

GL_API void GL_APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    glm_scale(*current_matrix_ptr(), (vec3){x, y, z});
}

GL_API void GL_APIENTRY glScalex(GLfixed x, GLfixed y, GLfixed z)
{
    GLfloat xf = gliFixedtoFloat(x);
    GLfloat yf = gliFixedtoFloat(y);
    GLfloat zf = gliFixedtoFloat(z);

    glScalef(xf, yf, zf);
}

GL_API void GL_APIENTRY glFrustumf(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
    if (n <= 0.0f || f <= 0.0f || l == r || b == t || n == f) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    mat4 m;
    glm_frustum(l, r, b, t, n, f, m);
    post_multiply(m);
}

GL_API void GL_APIENTRY glFrustumx(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f)
{
    GLfloat lf = gliFixedtoFloat(l);
    GLfloat rf = gliFixedtoFloat(r);
    GLfloat bf = gliFixedtoFloat(b);
    GLfloat tf = gliFixedtoFloat(t);
    GLfloat nf = gliFixedtoFloat(n);
    GLfloat ff = gliFixedtoFloat(f);

    glFrustumf(lf, rf, bf, tf, nf, ff);
}

GL_API void GL_APIENTRY glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
    if (l == r || b == t || n == f) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    mat4 m;
    glm_ortho(l, r, b, t, n, f, m);
    post_multiply(m);
}

GL_API void GL_APIENTRY glOrthox(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f)
{
    GLfloat lf = gliFixedtoFloat(l);
    GLfloat rf = gliFixedtoFloat(r);
    GLfloat bf = gliFixedtoFloat(b);
    GLfloat tf = gliFixedtoFloat(t);
    GLfloat nf = gliFixedtoFloat(n);
    GLfloat ff = gliFixedtoFloat(f);

    glOrthof(lf, rf, bf, tf, nf, ff);
}

GL_API void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    gli_context_t *c = gliGetContext();

    if (w < 0 || h < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // Viewport width and height are silently clamped to a range that depends on the implementation.
    w = GLI_MIN(w, c->implementation_limits.max_viewport_dims[0]);
    h = GLI_MIN(h, c->implementation_limits.max_viewport_dims[1]);

    c->transformation_state.viewport[0] = x;
    c->transformation_state.viewport[1] = y;
    c->transformation_state.viewport[2] = w;
    c->transformation_state.viewport[3] = h;
    c->transformation_state.viewport_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glDepthRangef(GLfloat n, GLfloat f)
{
    gli_context_t *c = gliGetContext();

    n = glm_clamp(n, 0.0f, 1.0f);
    f = glm_clamp(f, 0.0f, 1.0f);

    c->transformation_state.depth_range[0] = n;
    c->transformation_state.depth_range[1] = f;
    c->transformation_state.depth_range_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glDepthRangex(GLfixed n, GLfixed f)
{
    GLfloat nf = gliFixedtoFloat(n);
    GLfloat ff = gliFixedtoFloat(f);

    glDepthRangef(nf, ff);
}

GL_API void GL_APIENTRY glClipPlanef(GLenum p, const GLfloat *eqn)
{
    gli_context_t *context = gliGetContext();

    if (p < GL_CLIP_PLANE0 || p >= GL_CLIP_PLANE0 + GLI_MAX_CLIP_PLANES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    const GLuint plane_index = p - GL_CLIP_PLANE0;

    // When glClipPlane is called, the coefficients given by equation are transformed by the inverse of the modelview
    // matrix and stored in the resulting eye coordinates

    // Current modelview matrix
    mat4 *modelview = &context->transformation_state
                           .modelview_matrix_stack[context->transformation_state.modelview_matrix_stack_depth - 1];

    // Inverse-transpose of modelview
    mat4 inv;
    glm_mat4_inv(*modelview, inv);
    glm_mat4_transpose(inv);

    // Load eqn into a vec4 (A,B,C,D)
    vec4 eqn_vec;
    glm_vec4_make(eqn, eqn_vec);

    // Transform to eye coords
    vec4 transformed_eqn;
    glm_mat4_mulv(inv, eqn_vec, transformed_eqn);

    // Store in context as eye-space plane
    glm_vec4_copy(transformed_eqn, context->transformation_state.clip_plane[plane_index]);
    context->transformation_state.clip_plane_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glClipPlanex(GLenum p, const GLfixed *eqn)
{
    GLfloat eqnf[4];
    for (int i = 0; i < 4; ++i) {
        eqnf[i] = gliFixedtoFloat(eqn[i]);
    }

    glClipPlanef(p, eqnf);
}

GL_API void GL_APIENTRY glGetClipPlanef(GLenum plane, GLfloat *equation)
{
    gli_context_t *c = gliGetContext();

    if (plane < GL_CLIP_PLANE0 || plane >= GL_CLIP_PLANE0 + GLI_MAX_CLIP_PLANES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    const GLuint plane_index = plane - GL_CLIP_PLANE0;
    glm_vec4_copy(c->transformation_state.clip_plane[plane_index], equation);
}

GL_API void GL_APIENTRY glGetClipPlanex(GLenum plane, GLfixed *equation)
{
    GLfloat eqnf[4];
    glGetClipPlanef(plane, eqnf);
    for (int i = 0; i < 4; ++i) {
        equation[i] = gliFloattoFixed(eqnf[i]);
    }
}

static inline GLboolean is_mat4_identity(const mat4 m)
{
    static const mat4 identity = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    return __builtin_memcmp(m, identity, sizeof(mat4)) == 0;
}

void gliTransformFlush(void)
{
    gli_context_t *c = gliGetContext();
    transformation_state_t *ts = &c->transformation_state;

    const GLboolean any_transform_dirty =
        ts->modelview_matrix_dirty || ts->projection_matrix_dirty || ts->viewport_dirty || ts->depth_range_dirty;

    if (any_transform_dirty) {
        uint32_t *pb = pb_begin();

        // Model view and inverse model view
        if (ts->modelview_matrix_dirty) {
            mat4 *modelview = &ts->modelview_matrix_stack[ts->modelview_matrix_stack_depth - 1];
            pb = pb_push_transposed_matrix(pb, NV097_SET_MODEL_VIEW_MATRIX, (const float *)(*modelview));

            if (c->lighting_state.lighting_enabled) {
                mat4 modelview_inv;
                glm_mat4_inv(*modelview, modelview_inv);
                pb = pb_push_4x4_matrix(pb, NV097_SET_INVERSE_MODEL_VIEW_MATRIX, (const float *)modelview_inv);
            }
        }

        // Projection, Viewport, Composite
        if (ts->modelview_matrix_dirty || ts->projection_matrix_dirty || ts->viewport_dirty) {
            if (ts->viewport_dirty) {
                GLfloat zsize = (ts->depth_range[1] - ts->depth_range[0]) * (GLfloat)(GLI_DEPTH_BUFFER_MAX);
                GLint x = ts->viewport[0];
                GLint y = ts->viewport[1];
                GLsizei w = ts->viewport[2];
                GLsizei h = ts->viewport[3];

                GLfloat translate_x = x + (w / 2.0f);
                GLfloat translate_y = c->current_surface_height - y - (h / 2.0f);
                GLfloat translate_z = zsize / 2.0f;
                GLfloat scale_x = w / 2.0f;
                GLfloat scale_y = (h / -2.0f);
                GLfloat scale_z = zsize / 2.0f;
                GLfloat scale_w = 1.0f;

                mat4 viewport = {
                    {scale_x,     0.0f,        0.0f,        0.0f   },
                    {0.0f,        scale_y,     0.0f,        0.0f   },
                    {0.0f,        0.0f,        scale_z,     0.0f   },
                    {translate_x, translate_y, translate_z, scale_w}
                };

                glm_mat4_copy(viewport, ts->viewport_matrix);

                pb = xgu_set_viewport_offset(pb, 0.0f, 0.0f, 0.0f, 0.0f);
                pb = xgu_set_viewport_scale(pb, 1.0f, 1.0f, 1.0f, 1.0f);
                ts->viewport_dirty = GL_FALSE;
                ts->vp_matrix_dirty = GL_TRUE;
            }

            if (ts->projection_matrix_dirty || ts->vp_matrix_dirty) {
                mat4 *projection = &ts->projection_matrix_stack[ts->projection_matrix_stack_depth - 1];
                glm_mat4_mul(ts->viewport_matrix, *projection, ts->vp_matrix);
                ts->vp_matrix_dirty = GL_FALSE;
            }

            mat4 *modelview = &ts->modelview_matrix_stack[ts->modelview_matrix_stack_depth - 1];
            mat4 mvp;
            glm_mat4_mul(ts->vp_matrix, *modelview, mvp);

            pb = pb_push_transposed_matrix(pb, NV097_SET_COMPOSITE_MATRIX, (const float *)mvp);

            ts->modelview_matrix_dirty = GL_FALSE;
            ts->projection_matrix_dirty = GL_FALSE;
        }

        if (ts->depth_range_dirty) {
            uint32_t control0 = NV097_SET_CONTROL0_TEXTUREPERSPECTIVE | NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE;
            pb = push_command_parameter(pb, NV097_SET_CONTROL0, control0);
            pb = push_command_parameter(pb, NV097_SET_ZMIN_MAX_CONTROL, 1);
            pb = push_command_parameter(pb, NV097_SET_COMPRESS_ZBUFFER_EN, 1);
            pb = xgu_set_clip_min(pb, ts->depth_range[0] * (GLfloat)GLI_DEPTH_BUFFER_MAX);
            pb = xgu_set_clip_max(pb, ts->depth_range[1] * (GLfloat)GLI_DEPTH_BUFFER_MAX);
            ts->depth_range_dirty = GL_FALSE;
        }

        pb_end(pb);
    }

    // Texture matrices
    uint32_t *pb = NULL;
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &c->texture_environment.texture_units[i];
        texture_object_t *texture_object = texture_unit->bound_texture_object;

        if (!texture_object) {
            continue;
        }

        xgu_texture_t *xgu_texture = (xgu_texture_t *)texture_object->texture_2d;
        if (!xgu_texture) {
            continue;
        }

        if (ts->texture_matrix_dirty[i]) {
            ts->texture_matrix_dirty[i] = GL_FALSE;
            if (!pb) {
                pb = pb_begin();
            }

            const mat4 *src_tex_mat = &ts->texture_matrix_stack[i][ts->texture_matrix_stack_depth[i] - 1];
            if (xgu_texture->u_scale == 1.0f && xgu_texture->v_scale == 1.0f && is_mat4_identity(*src_tex_mat)) {
                pb = xgu_set_texture_matrix_enable(pb, i, false);
            } else {
                mat4 texture_matrix;
                glm_mat4_copy(*src_tex_mat, texture_matrix);
                // Scales the s and t output rows (including translation) to texel coordinates
                for (int col = 0; col < 4; col++) {
                    texture_matrix[col][0] *= xgu_texture->u_scale;
                    texture_matrix[col][1] *= xgu_texture->v_scale;
                }

                pb = xgu_set_texture_matrix_enable(pb, i, true);
                pb = pb_push_transposed_matrix(
                    pb, NV097_SET_TEXTURE_MATRIX + i * (4 * 4) * 4, (const float *)texture_matrix);
            }
        }
    }
    if (pb) {
        pb_end(pb);
    }
}
