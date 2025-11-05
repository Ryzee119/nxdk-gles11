#include "gles_private.h"

GL_API void GL_APIENTRY glCullFace(GLenum mode)
{
    gli_context_t *context = gliGetContext();

    XguCullFace xgu_mode = _gl_enum_to_xgu_cull_face(mode);
    if (xgu_mode == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->rasterization_state.cull_face_mode = mode;

    uint32_t *pb = pb_begin();
    pb = xgu_set_cull_face(pb, xgu_mode);
    pb_end(pb);
}

GL_API void GL_APIENTRY glFrontFace(GLenum mode)
{
    gli_context_t *context = gliGetContext();

    XguFrontFace xgu_mode = _gl_enum_to_xgu_front_face(mode);
    if (xgu_mode == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->rasterization_state.cull_front_face = mode;

    uint32_t *pb = pb_begin();
    pb = xgu_set_front_face(pb, xgu_mode);
    pb_end(pb);
}

GL_API void GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
    gli_context_t *context = gliGetContext();

    context->rasterization_state.polygon_offset_factor = factor;
    context->rasterization_state.polygon_offset_units = units;

    uint32_t *pb = pb_begin();
    pb = push_command_float(pb, NV097_SET_POLYGON_OFFSET_SCALE_FACTOR, factor);
    pb = push_command_float(pb, NV097_SET_POLYGON_OFFSET_BIAS, units);
    pb_end(pb);
}

GL_API void GL_APIENTRY glPolygonOffsetx(GLfixed factor, GLfixed units)
{
    GLfloat factorf = gliFixedtoFloat(factor);
    GLfloat unitsf = gliFixedtoFloat(units);
    glPolygonOffset(factorf, unitsf);
}

GL_API void GL_APIENTRY glLineWidth(GLfloat width)
{
    gli_context_t *context = gliGetContext();

    if (context->rasterization_state.line_smooth_enabled) {
        width = glm_clamp(width,
                      context->implementation_limits.antialiased_line_width_range[0],
                      context->implementation_limits.antialiased_line_width_range[1]);
    } else {
        width = glm_clamp(width,
                      context->implementation_limits.aliased_line_width_range[0],
                      context->implementation_limits.aliased_line_width_range[1]);
    }

    context->rasterization_state.line_width = width;

    uint32_t *pb = pb_begin();
    pb = push_command_parameter(pb, NV097_SET_LINE_WIDTH, (DWORD)(width * 8.0f));
    pb_end(pb);
}

GL_API void GL_APIENTRY glLineWidthx(GLfixed width)
{
    GLfloat widthf = gliFixedtoFloat(width);
    glLineWidth(width);
}

GL_API void GL_APIENTRY glPointSize(GLfloat size)
{
    gli_context_t *context = gliGetContext();

    if (context->rasterization_state.point_smooth_enabled) {
        size = glm_clamp(size,
                     context->implementation_limits.antialiased_point_size_range[0],
                     context->implementation_limits.antialiased_point_size_range[1]);
    } else {
        size = glm_clamp(size,
                     context->implementation_limits.aliased_point_size_range[0],
                     context->implementation_limits.aliased_point_size_range[1]);
    }

    context->rasterization_state.point_size = size;

    uint32_t *pb = pb_begin();
    pb = push_command_parameter(pb, NV097_SET_POINT_SIZE, (DWORD)(size * 8.0f));
    pb_end(pb);
}

GL_API void GL_APIENTRY glPointSizex(GLfixed size)
{
    GLfloat sizef = gliFixedtoFloat(size);
    glLineWidth(sizef);
}

GL_API void GL_APIENTRY glPointParameterf(GLenum pname, GLfloat param)
{
    gli_context_t *context = gliGetContext();
    rasterization_state_t *r = &context->rasterization_state;

    if (param < 0.0f) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    uint32_t *pb = pb_begin();
    pb = push_command_boolean(pb, NV097_SET_POINT_PARAMS_ENABLE, GL_TRUE);
    GLfloat range = r->point_size_max - r->point_size_min;

    switch (pname) {
        case GL_POINT_SIZE_MIN:
            r->point_size_min = param;
            pb = push_command_float(pb, NV097_SET_POINT_PARAMS_MIN_SIZE, param);
            // FIXME, check logic
            pb = push_command_float(pb, NV097_SET_POINT_PARAMS_SCALE_BIAS, -r->point_size_min / range);
            break;
        case GL_POINT_SIZE_MAX:
            r->point_size_max = param;
            range = r->point_size_max - r->point_size_min;
            // Don't know why DUP. Xbox does it though
            pb = push_command_float(pb, NV097_SET_POINT_PARAMS_SIZE_RANGE, range);
            pb = push_command_float(pb, NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_1, range);
            pb = push_command_float(pb, NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_2, range);
            break;
        case GL_POINT_FADE_THRESHOLD_SIZE:
            context->rasterization_state.point_fade_threshold_size = param;
            // FIXME
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            break;
    }
    pb_end(pb);
}

GL_API void GL_APIENTRY glPointParameterfv(GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();
    rasterization_state_t *r = &context->rasterization_state;
    transformation_state_t *t = &context->transformation_state;

    if (pname == GL_POINT_DISTANCE_ATTENUATION) {
        r->point_distance_attenuation[0] = params[0];
        r->point_distance_attenuation[1] = params[1];
        r->point_distance_attenuation[2] = params[2];

        const GLfloat range = r->point_size_max - r->point_size_min;
        const GLfloat size = r->point_size;
        const GLfloat height = t->viewport[3] - t->viewport[1];
        const GLfloat factor = range / (size * height);

        uint32_t *pb = pb_begin();
        pb = push_command_boolean(pb, NV097_SET_POINT_PARAMS_ENABLE, GL_TRUE);
        pb = push_command_float(pb, NV097_SET_POINT_PARAMS_SCALE_FACTOR_A, params[0] * factor);
        pb = push_command_float(pb, NV097_SET_POINT_PARAMS_SCALE_FACTOR_B, params[1] * factor);
        pb = push_command_float(pb, NV097_SET_POINT_PARAMS_SCALE_FACTOR_C, params[2] * factor);
        pb_end(pb);
        return;
    }

    glPointParameterf(pname, *params);
}

GL_API void GL_APIENTRY glPointParameterx(GLenum pname, GLfixed param)
{
    GLfloat paramf = gliFixedtoFloat(param);
    glPointParameterf(pname, paramf);
}

GL_API void GL_APIENTRY glPointParameterxv(GLenum pname, const GLfixed *params)
{
    (void)pname;
    (void)params;
    GLfloat paramsf[3];
    GLuint param_count = (pname == GL_POINT_DISTANCE_ATTENUATION) ? 3 : 1;
    for (int i = 0; i < param_count; i++) {
        paramsf[i] = gliFixedtoFloat(params[i]);
    }
    glPointParameterfv(pname, paramsf);
}
