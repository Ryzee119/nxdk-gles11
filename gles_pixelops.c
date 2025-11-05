#include "gles_private.h"

GL_API void GL_APIENTRY glAlphaFunc(GLenum func, GLfloat ref)
{
    gli_context_t *context = gliGetContext();

    XguFuncType xgu_func = _gl_enum_to_xgu_func(func);
    if (xgu_func == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    ref = glm_clamp(ref, 0.0f, 1.0f);

    context->pixel_ops_state.alpha_test_func = func;
    context->pixel_ops_state.alpha_test_ref = ref;

    uint32_t *pb = pb_begin();
    pb = xgu_set_alpha_func(pb, xgu_func);
    pb = xgu_set_alpha_ref(pb, ref);
    pb_end(pb);
}

GL_API void GL_APIENTRY glAlphaFuncx(GLenum func, GLfixed ref)
{
    GLfloat reff = gliFixedtoFloat(ref);
    glAlphaFunc(func, reff);
}

GL_API void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    gli_context_t *context = gliGetContext();
    XguBlendFactor xgu_sfactor = _gl_enum_to_xgu_blend_factor(sfactor);
    XguBlendFactor xgu_dfactor = _gl_enum_to_xgu_blend_factor(dfactor);

    if (xgu_sfactor == -1 || xgu_dfactor == -1 || sfactor == GL_SRC_COLOR || sfactor == GL_ONE_MINUS_SRC_COLOR ||
        dfactor == GL_DST_COLOR || dfactor == GL_ONE_MINUS_DST_COLOR || dfactor == GL_SRC_ALPHA_SATURATE) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->pixel_ops_state.blend_src = sfactor;
    context->pixel_ops_state.blend_dst = dfactor;

    uint32_t *pb = pb_begin();
    pb = xgu_set_blend_func_sfactor(pb, xgu_sfactor);
    pb = xgu_set_blend_func_dfactor(pb, xgu_dfactor);
    pb_end(pb);
}

GL_API void GL_APIENTRY glDepthFunc(GLenum func)
{
    gli_context_t *context = gliGetContext();

    XguFuncType xgu_func = _gl_enum_to_xgu_func(func);
    if (xgu_func == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->pixel_ops_state.depth_func = func;

    uint32_t *pb = pb_begin();
    pb = xgu_set_depth_func(pb, xgu_func);
    pb_end(pb);
}

GL_API void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    gli_context_t *context = gliGetContext();

    XguFuncType xgu_func = _gl_enum_to_xgu_func(func);
    if (xgu_func == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // Clamp it to 8 bits for xbox
    ref = GLI_CLAMP(ref, 0, 255);
    mask = GLI_CLAMP(mask, 0, 255);

    context->pixel_ops_state.stencil_func = func;
    context->pixel_ops_state.stencil_ref = ref;
    context->pixel_ops_state.stencil_value_mask = mask;

    uint32_t *pb = pb_begin();
    pb = xgu_set_stencil_func(pb, xgu_func);
    pb = xgu_set_stencil_func_ref(pb, ref);
    pb = xgu_set_stencil_func_mask(pb, mask);
    pb_end(pb);
}

GL_API void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    gli_context_t *context = gliGetContext();

    XguStencilOp xgu_fail = _gl_enum_to_xgu_stencilop(fail);
    XguStencilOp xgu_zfail = _gl_enum_to_xgu_stencilop(zfail);
    XguStencilOp xgu_zpass = _gl_enum_to_xgu_stencilop(zpass);
    if (xgu_fail == -1 || xgu_zfail == -1 || xgu_zpass == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->pixel_ops_state.stencil_fail_op = fail;
    context->pixel_ops_state.stencil_zfail_op = zfail;
    context->pixel_ops_state.stencil_zpass_op = zpass;

    uint32_t *pb = pb_begin();
    pb = xgu_set_stencil_op_fail(pb, xgu_fail);
    pb = xgu_set_stencil_op_zfail(pb, xgu_zfail);
    pb = xgu_set_stencil_op_zpass(pb, xgu_zpass);
    pb_end(pb);
}

GL_API void GL_APIENTRY glScissor(GLint x, GLint y, GLsizei w, GLsizei h)
{
    gli_context_t *context = gliGetContext();

    if (w < 0 || h < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    context->pixel_ops_state.scissor_box[0] = x;
    context->pixel_ops_state.scissor_box[1] = y;
    context->pixel_ops_state.scissor_box[2] = w;
    context->pixel_ops_state.scissor_box[3] = h;

    uint32_t *pb = pb_begin();
    pb = xgu_set_scissor_rect(pb,
                              false,
                              context->pixel_ops_state.scissor_box[0],
                              context->pixel_ops_state.scissor_box[1],
                              context->pixel_ops_state.scissor_box[2],
                              context->pixel_ops_state.scissor_box[3]);
    pb_end(pb);
}
