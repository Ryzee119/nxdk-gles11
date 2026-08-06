#define GL_GLEXT_PROTOTYPES
#include "gles_private.h"
#include <string.h>
#include <xmmintrin.h>

#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

static gli_context_t g_context;

GL_API void GL_APIENTRY glFinish(void)
{
    glFlush();
    while (pb_busy())
        ;
}

GL_API void GL_APIENTRY glFlush(void)
{
    // pbkit always flushes the push buffer on pb_end() calls.
    return;
}

void gliFlushStateChange(void)
{
    gliFBOFlush();
    gliTransformFlush();
    gliFogFlush();
    gliTextureFlush();
    gliPointParamsFlush();
    gliLightingFlush();
    gliArrayFlush();
}

GL_API GLenum GL_APIENTRY glGetError(void)
{
    gli_context_t *context = gliGetContext();
    GLenum error = context->last_error;
    context->last_error = GL_NO_ERROR;
    return error;
}

GL_API const GLubyte *GL_APIENTRY glGetString(GLenum name)
{
    switch (name) {
        case GL_VENDOR:
            return (const GLubyte *)GLI_VENDOR_STRING;
        case GL_RENDERER:
            return (const GLubyte *)GLI_RENDERER_STRING;
        case GL_VERSION:
            return (const GLubyte *)GLI_VERSION_STRING;
        case GL_EXTENSIONS:
            return (const GLubyte *)GLI_EXTENSIONS_STRING;
        default:
            gliSetError(GL_INVALID_ENUM);
            return NULL;
    }
}

GL_API void GL_APIENTRY glHint(GLenum target, GLenum mode)
{
    gli_context_t *context = gliGetContext();

    if (mode != GL_FASTEST && mode != GL_NICEST && mode != GL_DONT_CARE) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    switch (target) {
        case GL_FOG_HINT:
            context->hints_state.fog_hint = mode;
            break;
        case GL_GENERATE_MIPMAP_HINT:
            context->hints_state.generate_mipmap_hint = mode;
            break;
        case GL_LINE_SMOOTH_HINT:
            context->hints_state.line_smooth_hint = mode;
            break;
        case GL_PERSPECTIVE_CORRECTION_HINT:
            context->hints_state.perspective_correction_hint = mode;
            break;
        case GL_POINT_SMOOTH_HINT:
            context->hints_state.point_smooth_hint = mode;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glLogicOp(GLenum opcode)
{
    gli_context_t *context = gliGetContext();
    DWORD nv_opcode = gliEnumToNvLogicOp(opcode);
    if (nv_opcode == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->pixel_ops_state.color_logic_op = opcode;

    uint32_t *pb = pb_begin();
    pb = push_command_parameter(pb, NV097_SET_LOGIC_OP, nv_opcode);
    pb_end(pb);
}

GL_API void GL_APIENTRY glPixelStorei(GLenum pname, GLint param)
{
    gli_context_t *context = gliGetContext();

    if (param != 1 && param != 2 && param != 4 && param != 8) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // Affect the operation of subsequent glReadPixels as well as the unpacking of glTexImage2D, and glTexSubImage2D.
    if (pname == GL_UNPACK_ALIGNMENT) {
        context->pixel_store.unpack_alignment = param;
    } else if (pname == GL_PACK_ALIGNMENT) {
        context->pixel_store.pack_alignment = param;
    } else {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
}

static void glEnableDisable(GLenum cap, GLboolean enable)
{
    gli_context_t *context = gliGetContext();
    uint32_t *pb = pb_begin();
    switch (cap) {
        case GL_ALPHA_TEST:
            // If enabled, do alpha testing. See glAlphaFunc.
            context->pixel_ops_state.alpha_test_enabled = enable;
            pb = xgu_set_alpha_test_enable(pb, enable);
            break;
        case GL_BLEND:
            // If enabled, blend the computed fragment color values with the values in the color buffers. See
            // glBlendFunc.
            context->pixel_ops_state.blend_enabled = enable;
            pb = xgu_set_blend_enable(pb, enable);
            break;
        case GL_COLOR_LOGIC_OP:
            // If enabled, apply the currently selected logical operation to the computed fragment color and color
            // buffer values. See glLogicOp.
            context->pixel_ops_state.color_logic_op_enabled = enable;
            pb = push_command_boolean(pb, NV097_SET_LOGIC_OP_ENABLE, enable);
            break;
        case GL_CLIP_PLANE0 ... GL_CLIP_PLANE0 + GLI_MAX_CLIP_PLANES - 1:
            // If enabled, clip geometry against user-defined clipping plane i. See glClipPlane.
            const GLuint plane = cap - GL_CLIP_PLANE0;
            context->transformation_state.clip_plane_enabled[plane] = enable;
            context->transformation_state.clip_plane_dirty = GL_TRUE;
            break;
        case GL_COLOR_MATERIAL:
            // When enabled, both the ambient (acm) and diffuse (dcm) properties of both the front and back material are
            // immediately set to the value of the current color,
            context->lighting_state.color_material_enabled = enable;
            context->lighting_state.lighting_model_dirty = GL_TRUE;
            break;
        case GL_CULL_FACE:
            // If enabled, cull polygons based on their winding in window coordinates. See glCullFace.
            context->rasterization_state.cull_face_enabled = enable;
            pb = xgu_set_cull_face_enable(pb, enable);
            break;
        case GL_DEPTH_TEST:
            // If enabled, do depth comparisons and update the depth buffer. Note that even if the depth buffer exists
            // and the depth mask is non-zero, the depth buffer is not updated if the depth test is disabled. See
            // glDepthFunc and glDepthRange.
            context->pixel_ops_state.depth_test_enabled = enable;
            pb = xgu_set_depth_test_enable(pb, enable);
            break;
        case GL_DITHER:
            // If enabled, dither color components before they are written to the color buffer.
            context->pixel_ops_state.dither_enabled = enable;
            pb = xgu_set_dither_enable(pb, enable);
            break;
        case GL_FOG:
            // If enabled, blend a fog color into the posttexturing color. See glFog.
            context->coloring_state.fog_enabled = enable;
            pb = xgu_set_fog_enable(pb, enable);
            pb = xgu_set_fog_gen_mode(pb, XGU_FOG_GEN_MODE_RADIAL); // No API to change, this matches OpenGL look
            pb = combiner_specular_fog_config(
                pb, context->coloring_state.fog_enabled, context->lighting_state.lighting_enabled);
            break;
        case GL_LIGHT0 ... GL_LIGHT0 + GLI_MAX_LIGHTS - 1: {
            // If enabled, include light i in the evaluation of the lighting equation. See glLightModel and glLight.
            const GLuint light = cap - GL_LIGHT0;
            if (context->lighting_state.lights[light].enabled != enable) {
                context->lighting_state.lights[light].enabled = enable;
                context->lighting_state.light_mask_dirty = GL_TRUE;
            }
            break;
        }
        case GL_LIGHTING:
            // If enabled, use the current lighting parameters to compute the vertex color. Otherwise, simply associate
            // the current color with each vertex. See glMaterial, glLightModel, and glLight.
            context->lighting_state.lighting_enabled = enable;
            pb = xgu_set_lighting_enable(pb, enable);
            pb = xgu_set_specular_enable(pb, enable);
            pb = combiner_specular_fog_config(
                pb, context->coloring_state.fog_enabled, context->lighting_state.lighting_enabled);
            break;
        case GL_LINE_SMOOTH:
            // If enabled, draw lines with correct filtering. Otherwise, draw aliased lines. See glLineWidth.
            context->rasterization_state.line_smooth_enabled = enable;
            pb = push_command_boolean(pb, NV097_SET_LINE_SMOOTH_ENABLE, enable);
            break;
        case GL_MULTISAMPLE:
            // If enabled, use multiple fragment samples in computing the final color of a pixel. See glSampleCoverage.
            // NV097_SET_SURFACE_FORMAT_ANTI_ALIASING?
            context->multisampling_state.multisample_enabled = enable;
            break;
        case GL_NORMALIZE:
            // If enabled, normal vectors are normalized to unit length after transformation and before lighting. This
            // method is generally less efficient than GL_RESCALE_NORMAL. See glNormal and glNormalPointer.
            context->transformation_state.normalize_enabled = enable;
            pb = xgu_set_normalization_enable(pb,
                                              context->transformation_state.rescale_normal_enabled ||
                                                  context->transformation_state.normalize_enabled);
            break;
        case GL_POINT_SMOOTH:
            // If enabled, draw points with proper filtering. Otherwise, draw aliased points. See glPointSize.
            context->rasterization_state.point_smooth_enabled = enable;
            pb = push_command_boolean(pb, NV097_SET_POINT_SMOOTH_ENABLE, enable);
            break;
        case GL_POINT_SPRITE_OES:
            // If enabled, point sprites are enabled. See glPointSize and glTexEnv
            context->rasterization_state.point_sprite_oes_enabled = enable;
            pb = push_command_boolean(
                pb, NV097_SET_POINT_SMOOTH_ENABLE, enable); // Seems to need to be enabled for point sprite
            pb = push_command_boolean(pb, NV097_SET_POINT_PARAMS_ENABLE, enable);
            break;
        case GL_POLYGON_OFFSET_FILL:
            // If enabled, an offset is added to depth values of a polygon's fragments before the depth comparison is
            // performed. See glPolygonOffset.
            context->rasterization_state.polygon_offset_fill_enabled = enable;
            pb = push_command_boolean(pb, NV097_SET_POLY_OFFSET_FILL_ENABLE, enable);
            break;
        case GL_RESCALE_NORMAL:
            // If enabled, normal vectors are scaled after transformation and before lighting by a factor computed from
            // the modelview matrix. If the modelview matrix scales space uniformly, this has the effect of restoring
            // the transformed normal to unit length. This method is generally more efficient than GL_NORMALIZE. See
            // glNormal and glNormalPointer.
            context->transformation_state.rescale_normal_enabled = enable;

            // Just use nv2a normalization feature
            pb = xgu_set_normalization_enable(pb,
                                              context->transformation_state.rescale_normal_enabled ||
                                                  context->transformation_state.normalize_enabled);
            break;
        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            // If enabled, compute a temporary coverage value where each bit is determined by the alpha value at the
            // corresponding sample location. The temporary coverage value is then ANDed with the fragment coverage
            // value.
            context->multisampling_state.sample_alpha_to_coverage_enabled = enable;
            // FIXME
            break;
        case GL_SAMPLE_ALPHA_TO_ONE:
            // If enabled, each sample alpha value is replaced by the maximum representable alpha value.
            context->multisampling_state.sample_alpha_to_one_enabled = enable;
            // FIXME
            break;
        case GL_SAMPLE_COVERAGE:
            // If enabled, the fragment's coverage is ANDed with the temporary coverage value. If
            // GL_SAMPLE_COVERAGE_INVERT is set to GL_TRUE, invert the coverage value. See glSampleCoverage.
            context->multisampling_state.sample_coverage_enabled = enable;
            // FIXME
            break;
        case GL_SCISSOR_TEST:
            // If enabled, discard fragments that are outside the scissor rectangle. See glScissor.
            context->pixel_ops_state.scissor_test_enabled = enable;
            if (!enable) {
                // Can't disable so max it out
                pb = xgu_set_scissor_rect(pb, false, 0, 0, 4095, 4095);
            } else {
                GLint hw_x = context->pixel_ops_state.scissor_box[0];
                GLint hw_y = context->pixel_ops_state.scissor_box[1];
                GLint hw_w = context->pixel_ops_state.scissor_box[2];
                GLint hw_h = context->pixel_ops_state.scissor_box[3];

                gliCalculateHardwareScissor(context, &hw_x, &hw_y, &hw_w, &hw_h);

                pb = xgu_set_scissor_rect(pb, false, hw_x, hw_y, hw_w, hw_h);
            }
            break;
        case GL_STENCIL_TEST:
            // If enabled, do stencil testing and update the stencil buffer. See glStencilFunc, glStencilMask, and
            // glStencilOp.
            context->pixel_ops_state.stencil_test_enabled = enable;
            pb = xgu_set_stencil_test_enable(pb, enable);
            if (!enable) {
                pb = xgu_set_stencil_func(pb, XGU_FUNC_ALWAYS);
                pb = xgu_set_stencil_func_ref(pb, 0);
                pb = xgu_set_stencil_func_mask(pb, 0xFF);
                pb = xgu_set_stencil_op_fail(pb, XGU_STENCIL_OP_KEEP);
                pb = xgu_set_stencil_op_zfail(pb, XGU_STENCIL_OP_KEEP);
                pb = xgu_set_stencil_op_zpass(pb, XGU_STENCIL_OP_KEEP);
                pb = xgu_set_stencil_mask(pb, 0xFF);
            } else {
                const pixel_ops_state_t *pos = &context->pixel_ops_state;
                pb = xgu_set_stencil_func(pb, gliEnumToNvFunc(pos->stencil_func));
                pb = xgu_set_stencil_func_ref(pb, pos->stencil_ref);
                pb = xgu_set_stencil_func_mask(pb, pos->stencil_value_mask);
                pb = xgu_set_stencil_op_fail(pb, gliEnumToNvStencilOp(pos->stencil_fail_op));
                pb = xgu_set_stencil_op_zfail(pb, gliEnumToNvStencilOp(pos->stencil_zfail_op));
                pb = xgu_set_stencil_op_zpass(pb, gliEnumToNvStencilOp(pos->stencil_zpass_op));
                pb = xgu_set_stencil_mask(pb, context->framebuffer_control.stencil_writemask);
            }
            break;
        case GL_TEXTURE_2D:
            // If enabled, two-dimensional texturing is performed for the active texture unit. See glActiveTexture,
            // glTexImage2D, glCompressedTexImage2D, and glCopyTexImage2D.
            const GLuint texture = context->texture_environment.server_active_texture - GL_TEXTURE0;
            context->texture_environment.texture_units[texture].texture_2d_enabled = enable;
            context->texture_environment.texture_units[texture].texture_unit_dirty = GL_TRUE;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            break;
    }
    pb_end(pb);
}

GL_API void GL_APIENTRY glEnable(GLenum cap)
{
    glEnableDisable(cap, GL_TRUE);
}

GL_API void GL_APIENTRY glDisable(GLenum cap)
{
    glEnableDisable(cap, GL_FALSE);
}

GL_API GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
    gli_context_t *context = gliGetContext();
    switch (cap) {
        case GL_ALPHA_TEST:
            return context->pixel_ops_state.alpha_test_enabled;
        case GL_BLEND:
            return context->pixel_ops_state.blend_enabled;
        case GL_COLOR_LOGIC_OP:
            return context->pixel_ops_state.color_logic_op_enabled;
        case GL_CLIP_PLANE0 ... GL_CLIP_PLANE0 + GLI_MAX_CLIP_PLANES:
            const GLuint plane = cap - GL_CLIP_PLANE0;
            return context->transformation_state.clip_plane_enabled[plane];
        case GL_COLOR_MATERIAL:
            return context->lighting_state.color_material_enabled;
        case GL_CULL_FACE:
            return context->rasterization_state.cull_face_enabled;
        case GL_DEPTH_TEST:
            return context->pixel_ops_state.depth_test_enabled;
        case GL_DITHER:
            return context->pixel_ops_state.dither_enabled;
        case GL_FOG:
            return context->coloring_state.fog_enabled;
        case GL_LIGHT0 ... GL_LIGHT0 + GLI_MAX_LIGHTS:
            const GLuint light = cap - GL_LIGHT0;
            return context->lighting_state.lights[light].enabled;
        case GL_LIGHTING:
            return context->lighting_state.lighting_enabled;
        case GL_LINE_SMOOTH:
            return context->rasterization_state.line_smooth_enabled;
        case GL_MULTISAMPLE:
            return context->multisampling_state.multisample_enabled;
        case GL_NORMALIZE:
            return context->transformation_state.normalize_enabled;
        case GL_POINT_SMOOTH:
            return context->rasterization_state.point_smooth_enabled;
        case GL_POINT_SPRITE_OES:
            return context->rasterization_state.point_sprite_oes_enabled;
        case GL_POLYGON_OFFSET_FILL:
            return context->rasterization_state.polygon_offset_fill_enabled;
        case GL_RESCALE_NORMAL:
            return context->transformation_state.rescale_normal_enabled;
        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            return context->multisampling_state.sample_alpha_to_coverage_enabled;
        case GL_SAMPLE_ALPHA_TO_ONE:
            return context->multisampling_state.sample_alpha_to_one_enabled;
        case GL_SAMPLE_COVERAGE:
            return context->multisampling_state.sample_coverage_enabled;
        case GL_SCISSOR_TEST:
            return context->pixel_ops_state.scissor_test_enabled;
        case GL_STENCIL_TEST:
            return context->pixel_ops_state.stencil_test_enabled;
        case GL_TEXTURE_2D:
            const GLuint texture = context->texture_environment.server_active_texture - GL_TEXTURE0;
            return context->texture_environment.texture_units[texture].texture_2d_enabled;
        default:
            gliSetError(GL_INVALID_ENUM);
            break;
    }
    return GL_FALSE;
}

GL_API void GL_APIENTRY glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    gli_context_t *context = gliGetContext();
    context->framebuffer_control.color_mask[0] = r;
    context->framebuffer_control.color_mask[1] = g;
    context->framebuffer_control.color_mask[2] = b;
    context->framebuffer_control.color_mask[3] = a;

    XguColorMask mask = 0;
    if (r) {
        mask |= XGU_RED;
    }
    if (g) {
        mask |= XGU_GREEN;
    }
    if (b) {
        mask |= XGU_BLUE;
    }
    if (a) {
        mask |= XGU_ALPHA;
    }
    uint32_t *pb = pb_begin();
    pb = xgu_set_color_mask(pb, mask);
    pb_end(pb);
}

GL_API void GL_APIENTRY glDepthMask(GLboolean flag)
{
    gli_context_t *context = gliGetContext();
    context->framebuffer_control.depth_writemask = flag;
    uint32_t *pb = pb_begin();
    pb = xgu_set_depth_mask(pb, flag);
    pb_end(pb);
}

GL_API void GL_APIENTRY glClear(GLbitfield mask)
{
    gli_context_t *context = gliGetContext();

    const uint32_t fmt_color = (context->current_surface_format & NV097_SET_SURFACE_FORMAT_COLOR) >> 0;
    const uint32_t fmt_zeta = (context->current_surface_format & NV097_SET_SURFACE_FORMAT_ZETA) >> 4;
    const uint32_t fmt_type = (context->current_surface_format & NV097_SET_SURFACE_FORMAT_TYPE) >> 8;

    uint32_t nv_clear_mask = 0;
    if (mask & GL_COLOR_BUFFER_BIT) {
        nv_clear_mask |= NV097_CLEAR_SURFACE_COLOR;
    }
    if (mask & GL_DEPTH_BUFFER_BIT) {
        nv_clear_mask |= NV097_CLEAR_SURFACE_Z;
    }
    if (mask & GL_STENCIL_BUFFER_BIT) {
        nv_clear_mask |= NV097_CLEAR_SURFACE_STENCIL;
    }

    if (fmt_zeta == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
        nv_clear_mask &= ~NV097_CLEAR_SURFACE_STENCIL;
    }

    if (!nv_clear_mask) {
        return;
    }

    const DWORD nv_clear_color = gliColor4fToNvColor(fmt_color, context->framebuffer_control.clear_color);
    const DWORD nv_clear_zstencil = gliDepthStencilToNvZeta(
        fmt_zeta, context->framebuffer_control.clear_depth, context->framebuffer_control.clear_stencil);

    GLint sx = 0, sy = 0;
    GLint sw = context->current_surface_width;
    GLint sh = context->current_surface_height;

    if (context->pixel_ops_state.scissor_test_enabled) {
        sx = context->pixel_ops_state.scissor_box[0];
        sy = context->pixel_ops_state.scissor_box[1];
        sw = context->pixel_ops_state.scissor_box[2];
        sh = context->pixel_ops_state.scissor_box[3];

        gliCalculateHardwareScissor(context, &sx, &sy, &sw, &sh);
    }

    uint32_t *pb = pb_begin();

    if (fmt_type == NV097_SET_SURFACE_FORMAT_TYPE_SWIZZLE) {
        uint32_t temp_format = context->current_surface_format & ~NV097_SET_SURFACE_FORMAT_TYPE;
        temp_format |= XGU_MASK(NV097_SET_SURFACE_FORMAT_TYPE, NV097_SET_SURFACE_FORMAT_TYPE_PITCH);
        pb = push_command_parameter(pb, NV097_SET_SURFACE_FORMAT, temp_format);
    }

    pb = push_command_parameter(pb, NV097_SET_CLEAR_RECT_HORIZONTAL, ((sx + sw - 1) << 16) | sx);
    pb = push_command_parameter(pb, NV097_SET_CLEAR_RECT_VERTICAL, ((sy + sh - 1) << 16) | sy);
    pb = push_command_parameter(pb, NV097_SET_COLOR_CLEAR_VALUE, nv_clear_color);
    pb = push_command_parameter(pb, NV097_SET_ZSTENCIL_CLEAR_VALUE, nv_clear_zstencil);
    pb = push_command_parameter(pb, NV097_CLEAR_SURFACE, nv_clear_mask);

    if (fmt_type == NV097_SET_SURFACE_FORMAT_TYPE_SWIZZLE) {
        pb = push_command_parameter(pb, NV097_SET_SURFACE_FORMAT, context->current_surface_format);
    }
    pb_end(pb);
}

GL_API void GL_APIENTRY glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    gli_context_t *context = gliGetContext();
    glm_vec4_copy((vec4){r, g, b, a}, context->framebuffer_control.clear_color);
}

GL_API void GL_APIENTRY glClearColorx(GLfixed r, GLfixed g, GLfixed b, GLfixed a)
{
    GLfloat rf = gliFixedtoFloat(r);
    GLfloat gf = gliFixedtoFloat(g);
    GLfloat bf = gliFixedtoFloat(b);
    GLfloat af = gliFixedtoFloat(a);
    glClearColor(rf, gf, bf, af);
}

GL_API void GL_APIENTRY glClearDepthf(GLfloat d)
{
    gli_context_t *context = gliGetContext();
    context->framebuffer_control.clear_depth = d;
}

GL_API void GL_APIENTRY glClearDepthx(GLfixed d)
{
    GLfloat df = gliFixedtoFloat(d);
    glClearDepthf(df);
}

GL_API void GL_APIENTRY glStencilMask(GLuint mask)
{
    gli_context_t *context = gliGetContext();

    context->framebuffer_control.stencil_writemask = mask;

    uint32_t *pb = pb_begin();
    pb = xgu_set_stencil_mask(pb, mask);
    pb_end(pb);
}

GL_API void GL_APIENTRY glClearStencil(GLint s)
{
    gli_context_t *context = gliGetContext();
    context->framebuffer_control.clear_stencil = s;
}

GL_API void GL_APIENTRY glShadeModel(GLenum mode)
{
    gli_context_t *context = gliGetContext();

    XguShadeModel xgu_mode = gliEnumToNvShadeModel(mode);
    if (xgu_mode == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    context->coloring_state.shade_model = mode;

    uint32_t *pb = pb_begin();
    pb = pb_push1(pb, NV097_SET_SHADE_MODEL, xgu_mode);
    pb_end(pb);
}

GL_API void GL_APIENTRY glSampleCoverage(GLfloat value, GLboolean invert)
{
    // The value is clamped to the range [0, 1], where 0 represents no coverage and 1 full coverage.
    value = glm_clamp(value, 0.0f, 1.0f);

    gli_context_t *context = gliGetContext();
    context->multisampling_state.sample_coverage_value = value;
    context->multisampling_state.sample_coverage_invert = invert;

    // FIXME. Is this supported by nv2a?
}

GL_API void GL_APIENTRY glSampleCoveragex(GLclampx value, GLboolean invert)
{
    GLfloat fvalue = gliFixedtoFloat(value);
    glSampleCoverage(fvalue, invert);
}

gli_context_t *gliGetContext(void)
{
    return &g_context;
}

void gliSetError(GLenum error)
{
    gli_context_t *context = gliGetContext();
    // No other errors are recorded until glGetError is called
    if (context->last_error != GL_NO_ERROR) {
        return;
    }
    context->last_error = error;
}

void glContextInit(GLint window_width, GLint window_height)
{
    gli_context_t *context = gliGetContext();
    gli_memset(context, 0, sizeof(*context));

    while (pb_init() < 0) {
        gliDebugF("[nxdk renderer] pbkit initialization failed, retrying...\n");
    }

    pb_show_front_screen();
    pb_target_back_buffer();

    combiner_init();

    uint32_t *pb = pb_begin();
    pb = combiner_specular_fog_config(pb, GL_FALSE, GL_FALSE);
    pb = push_command_parameter(pb, NV097_SET_PROVOKING_VERTEX, NV097_SET_PROVOKING_VERTEX_LAST);
    pb_end(pb);

    /* --- Table 6.18: Pixel store --- */
    glPixelStorei(GL_UNPACK_ALIGNMENT, GLI_UNPACK_ALIGNMENT);
    glPixelStorei(GL_PACK_ALIGNMENT, GLI_PACK_ALIGNMENT);

    /* --- Table 6.19: Hints --- */
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_DONT_CARE);
    glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glHint(GL_FOG_HINT, GL_DONT_CARE);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE);

    /* --- Table 6.20: Implementation limits --- */
    context->implementation_limits.max_lights = GLI_MAX_LIGHTS;
    context->implementation_limits.max_clip_planes = GLI_MAX_CLIP_PLANES;
    context->implementation_limits.max_modelview_stack_depth = GLI_MAX_MODELVIEW_STACK;
    context->implementation_limits.max_projection_stack_depth = GLI_MAX_PROJECTION_STACK;
    context->implementation_limits.max_texture_stack_depth = GLI_MAX_TEXTURE_STACK;
    context->implementation_limits.subpixel_bits = GLI_SUBPIXEL_BITS;
    context->implementation_limits.max_texture_size = GLI_MAX_TEXTURE_SIZE;
    context->implementation_limits.max_viewport_dims[0] = GLI_MAX_VIEWPORT_WIDTH;
    context->implementation_limits.max_viewport_dims[1] = GLI_MAX_VIEWPORT_HEIGHT;
    context->implementation_limits.aliased_point_size_range[0] = GLI_MIN_ALIASED_POINT_SIZE;
    context->implementation_limits.aliased_point_size_range[1] = GLI_MAX_ALIASED_POINT_SIZE;
    context->implementation_limits.antialiased_point_size_range[0] = GLI_MIN_SMOOTH_POINT_SIZE;
    context->implementation_limits.antialiased_point_size_range[1] = GLI_MAX_SMOOTH_POINT_SIZE;
    context->implementation_limits.aliased_line_width_range[0] = GLI_MIN_ALIASED_LINE_WIDTH;
    context->implementation_limits.aliased_line_width_range[1] = GLI_MAX_ALIASED_LINE_WIDTH;
    context->implementation_limits.antialiased_line_width_range[0] = GLI_MIN_SMOOTH_LINE_WIDTH;
    context->implementation_limits.antialiased_line_width_range[1] = GLI_MAX_SMOOTH_LINE_WIDTH;
    context->implementation_limits.max_texture_units = GLI_MAX_TEXTURE_UNITS;
    context->implementation_limits.sample_buffers = 0;
    context->implementation_limits.samples = 0;
    context->implementation_limits.compressed_texture_formats = NULL;
    context->implementation_limits.num_compressed_texture_formats = GLI_NUM_COMPRESSED_TEXTURE_FORMATS;
    context->current_surface_width = window_width;
    context->current_surface_height = window_height;
    context->current_surface_format =
        XGU_MASK(NV097_SET_SURFACE_FORMAT_COLOR, NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8) |
        XGU_MASK(NV097_SET_SURFACE_FORMAT_ZETA, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8) |
        XGU_MASK(NV097_SET_SURFACE_FORMAT_TYPE, NV097_SET_SURFACE_FORMAT_TYPE_PITCH);

    /* --- Table 6.3: Current values --- */
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    for (int i = 0; i < GLI_MAX_TEXTURE_UNITS; ++i) {
        glMultiTexCoord4f(GL_TEXTURE0 + i, 0.0f, 0.0f, 0.0f, 1.0f);
    }
    glNormal3f(0.0f, 0.0f, 1.0f);

    /* --- Table 6.4 Vertex Array Data --- */
    glClientActiveTexture(GL_TEXTURE0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(4, GL_FLOAT, 0, NULL);
    glDisableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, 0, NULL);
    glDisableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_FLOAT, 0, NULL);

    /* --- Table 6.5 Vertex Array Data Continued --- */
    for (int i = 0; i < GLI_MAX_TEXTURE_UNITS; ++i) {
        glClientActiveTexture(GL_TEXTURE0 + i);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(4, GL_FLOAT, 0, NULL);
    }
    glClientActiveTexture(GL_TEXTURE0);

    /* OES_point_size_array */
    glDisableClientState(GL_POINT_SIZE_ARRAY_OES);
    glPointSizePointerOES(GL_FLOAT, 0, NULL);

    /* Buffer bindings (global) + any per-attribute names you track */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /* --- Table 6.6: Buffer object state registry --- */
    context->buffer_objects = NULL;

    /* --- Table 6.7: Transformation state --- */
    context->transformation_state.modelview_matrix_stack_depth = 1;
    context->transformation_state.projection_matrix_stack_depth = 1;

    /* Each stack starts with one identity matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    for (int u = 0; u < GLI_MAX_TEXTURE_UNITS; ++u) {
        context->transformation_state.texture_matrix_stack_depth[u] = 1;
        glActiveTexture(GL_TEXTURE0 + u);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
    }
    glActiveTexture(GL_TEXTURE0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, window_width, window_height);
    glDepthRangef(0.0f, 1.0f);

    glDisable(GL_NORMALIZE);
    glDisable(GL_RESCALE_NORMAL);

    for (int i = 0; i < GLI_MAX_CLIP_PLANES; ++i) {
        glDisable(GL_CLIP_PLANE0 + i);
        glClipPlanef(GL_CLIP_PLANE0 + i, GLM_VEC4_ZERO);
    }

    /* --- Table 6.8: Coloring (fog & shading) --- */
    glFogfv(GL_FOG_COLOR, GLM_VEC4_ZERO);
    glFogf(GL_FOG_DENSITY, 1.0f);
    glFogf(GL_FOG_START, 0.0f);
    glFogf(GL_FOG_END, 1.0f);
    glFogf(GL_FOG_MODE, GL_EXP);
    glDisable(GL_FOG);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);

    GLfloat mat_ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat mat_diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat mat_specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat mat_emission[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emission);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

    GLfloat lm_ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm_ambient);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0.0f);

    GLfloat light_ambient[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat light_diffuse_zero[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat light_diffuse_one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat light_position[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    GLfloat light_spot_dir[3] = {0.0f, 0.0f, -1.0f};
    for (int i = 0; i < GLI_MAX_LIGHTS; ++i) {
        GLenum light = GL_LIGHT0 + i;
        glDisable(light);

        glLightfv(light, GL_AMBIENT, light_ambient);
        if (i == 0) {
            glLightfv(light, GL_DIFFUSE, light_diffuse_one);
            glLightfv(light, GL_SPECULAR, light_diffuse_one);
        } else {
            glLightfv(light, GL_DIFFUSE, light_diffuse_zero);
            glLightfv(light, GL_SPECULAR, light_diffuse_zero);
        }

        glLightfv(light, GL_POSITION, light_position);
        glLightfv(light, GL_SPOT_DIRECTION, light_spot_dir);
        glLightf(light, GL_SPOT_EXPONENT, 0.0f);
        glLightf(light, GL_SPOT_CUTOFF, 180.0f);
        glLightf(light, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(light, GL_LINEAR_ATTENUATION, 0.0f);
        glLightf(light, GL_QUADRATIC_ATTENUATION, 0.0f);
    }

    /* --- Table 6.11: Rasterization --- */
    /* Points */
    glPointSize(1.0f);
    glDisable(GL_POINT_SMOOTH);
    glPointParameterf(GL_POINT_SIZE_MIN, 0.0f);
    glPointParameterf(GL_POINT_SIZE_MAX, GLI_MAX_ALIASED_POINT_SIZE);
    glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
    GLfloat point_dist_attenuation[3] = {1.0f, 0.0f, 0.0f};
    glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, point_dist_attenuation);
    glDisable(GL_POINT_SPRITE_OES);

    /* Lines */
    glLineWidth(1.0f);
    glDisable(GL_LINE_SMOOTH);

    /* Polygons / culling / offset */
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);

    /* --- Table 6.12: Multisampling --- */
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    glDisable(GL_SAMPLE_COVERAGE);
    glSampleCoverage(1.0f, GL_FALSE);

    /* --- Tables 6.13/6.14: Textures & TexEnv (per unit) --- */
    for (GLint i = 0; i < GLI_MAX_TEXTURE_UNITS; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GLM_VEC4_ZERO);
        glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_FALSE);

        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);

        glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_CONSTANT);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PREVIOUS);
        glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_ALPHA, GL_CONSTANT);

        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);

        glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
        glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);
    }
    glActiveTexture(GL_TEXTURE0);

    /* --- Table 6.16: Per-fragment / pixel ops --- */
    glDisable(GL_SCISSOR_TEST);
    glScissor(0, 0, window_width, window_height);
    glDisable(GL_ALPHA_TEST);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, ~0u);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glBlendEquationOES(GL_FUNC_ADD_OES);
    glDisable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_COPY);
    glEnable(GL_DITHER);

    /* --- Table 6.17: Framebuffer control --- */
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(~0u);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepthf(1.0f);
    glClearStencil(0);

    gliStagingInit();

    gliFlushStateChange();
    while (pb_busy()) {
    }
}

int gliDebugF(const char *fmt, ...)
{
    char buffer[512];
    va_list ap;
    va_start(ap, fmt);
    int n = stbsp_vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    GLI_DEBUG_PRINT("%s", buffer);
    return n;
}

GLuint gliEnumtoByteSize(GLenum type)
{
    switch (type) {
        case GL_BYTE:
            return sizeof(GLbyte);
        case GL_UNSIGNED_BYTE:
            return sizeof(GLubyte);
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        case GL_SHORT:
            return sizeof(GLshort);
        case GL_UNSIGNED_SHORT:
            return sizeof(GLushort);
        case GL_FIXED:
            return sizeof(GLfixed);
        case GL_FLOAT:
            return sizeof(GLfloat);
        default:
            return 0;
    }
}

GLuint gliFormatToBpp(GLenum format)
{
    switch (format) {
        case GL_RGBA8_OES:
        case GL_RGB8_OES:
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH24_STENCIL8_OES:
            return 4;
        case GL_RGB565_OES:
        case GL_RGB5_A1_OES:
        case GL_RGBA4_OES:
        case GL_DEPTH_COMPONENT16_OES:
            return 2;
        default:
            return 0;
    }
}

void *gli_memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    int streamed = 0;

    // Fast path 1: Both Source and Destination are 16-byte aligned
    if (n >= 16 && ((uintptr_t)d & 15) == 0 && ((uintptr_t)s & 15) == 0) {
        streamed = 1;
        size_t n64 = n >> 6;
        while (n64--) {
            _mm_prefetch((const char *)(s + 128), _MM_HINT_NTA);

            __m128 x0 = _mm_load_ps((const float *)s);
            __m128 x1 = _mm_load_ps((const float *)(s + 16));
            __m128 x2 = _mm_load_ps((const float *)(s + 32));
            __m128 x3 = _mm_load_ps((const float *)(s + 48));

            // Fills exactly two 32-byte Coppermine WCBs for a clean bus burst
            _mm_stream_ps((float *)d, x0);
            _mm_stream_ps((float *)(d + 16), x1);
            _mm_stream_ps((float *)(d + 32), x2);
            _mm_stream_ps((float *)(d + 48), x3);

            d += 64;
            s += 64;
        }
        n &= 63;

        size_t n16 = n >> 4;
        while (n16--) {
            __m128 x = _mm_load_ps((const float *)s);
            _mm_stream_ps((float *)d, x);
            d += 16;
            s += 16;
        }
        n &= 15;
    }
    // Fast path 2: Destination is 16-byte aligned
    else if (n >= 16 && ((uintptr_t)d & 15) == 0) {
        streamed = 1;
        size_t n16 = n >> 4;
        while (n16--) {
            __m128 x = _mm_loadu_ps((const float *)s);
            _mm_stream_ps((float *)d, x);
            d += 16;
            s += 16;
        }
        n &= 15;
    }

    if (streamed) {
        _mm_sfence();
    }

    // Fallback path: 4-byte aligned
    if (n >= 4 && ((uintptr_t)d & 3) == 0 && ((uintptr_t)s & 3) == 0) {
        uint32_t *d32 = (uint32_t *)d;
        const uint32_t *s32 = (const uint32_t *)s;
        size_t n4 = n >> 2;
        while (n4--) {
            *d32++ = *s32++;
        }
        d = (uint8_t *)d32;
        s = (const uint8_t *)s32;
        n &= 3;
    }

    // Byte-by-byte tail cleanup
    while (n > 0) {
        *d++ = *s++;
        n--;
    }
    return dst;
}

void *gli_memset(void *dst, int c, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    uint8_t val = (uint8_t)c;
    uint32_t val32 = val | (val << 8) | (val << 16) | (val << 24);

    // Only bother with SSE for payloads large enough to justify the overhead
    if (n >= 32) {
        // Get to 16 byte alignment
        size_t align_offset = (16 - ((uintptr_t)d & 15)) & 15;
        n -= align_offset;
        while (align_offset--) {
            *d++ = val;
        }

        // Generate the SSE broadcast register (SSE1 safe, strict-aliasing safe)
        float fval;
        memcpy(&fval, &val32, sizeof(fval));
        __m128 xf = _mm_set1_ps(fval);

        size_t n64 = n >> 6; // 64-byte chunks
        if (n64) {
            if (((uintptr_t)d & 31) != 0) {
                _mm_stream_ps((float *)d, xf);
                d += 16;
                n -= 16;
                n64 = n >> 6; // Recalculate chunks
            }

            while (n64--) {
                _mm_stream_ps((float *)d, xf);
                _mm_stream_ps((float *)(d + 16), xf);
                _mm_stream_ps((float *)(d + 32), xf);
                _mm_stream_ps((float *)(d + 48), xf);
                d += 64;
            }
        }

        // 16 byte chunks
        n &= 63;
        size_t n16 = n >> 4;
        while (n16--) {
            _mm_stream_ps((float *)d, xf);
            d += 16;
        }
        n &= 15;

        _mm_sfence();
    }

    // Handle remaining 4-byte chunks
    size_t n4 = n >> 2;
    if (n4) {
        uint32_t *d32 = (uint32_t *)d;
        while (n4--) {
            *d32++ = val32;
        }
        d = (uint8_t *)d32;
        n &= 3;
    }

    // Handle remaining byte chunks
    while (n--) {
        *d++ = val;
    }

    return dst;
}
