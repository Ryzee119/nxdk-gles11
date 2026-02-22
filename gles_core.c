#include "gles_private.h"

#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

static gli_context_t g_context;

GL_API void GL_APIENTRY glFinish(void)
{
    glFlush();
    while (pb_busy())
        ;
    while (pb_finished())
        ;
}

GL_API void GL_APIENTRY glFlush(void)
{
    uint32_t *pb = pb_begin();
    pb = pb_push1(pb, NV097_WAIT_FOR_IDLE, 0);
    pb_end(pb);
}

void gliFlushStateChange(void)
{
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
        case GL_LIGHT0 ... GL_LIGHT0 + GLI_MAX_LIGHTS - 1:
            // If enabled, include light i in the evaluation of the lighting equation. See glLightModel and glLight.
            const GLuint light = cap - GL_LIGHT0;
            context->lighting_state.lights[light].enabled = enable;
            break;
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
                pb = xgu_set_scissor_rect(pb,
                                          false,
                                          context->pixel_ops_state.scissor_box[0],
                                          context->pixel_ops_state.scissor_box[1],
                                          context->pixel_ops_state.scissor_box[2],
                                          context->pixel_ops_state.scissor_box[3]);
            }
            break;
        case GL_STENCIL_TEST:
            // If enabled, do stencil testing and update the stencil buffer. See glStencilFunc, glStencilMask, and
            // glStencilOp.
            context->pixel_ops_state.stencil_test_enabled = enable;
            pb = xgu_set_stencil_test_enable(pb, enable);
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

    const uint32_t nv_clear_color = FLOAT4_TO_PACKED_ARGB32(context->framebuffer_control.clear_color);

    DWORD zstencil =
        (context->framebuffer_control.clear_stencil & 0xFF) |
        (((GLuint)(context->framebuffer_control.clear_depth * (GLfloat)GLI_DEPTH_BUFFER_MAX) << 8) & 0xFFFFFF00);

    uint32_t *pb = pb_begin();
    pb = push_command_parameter(pb, NV097_SET_COLOR_CLEAR_VALUE, nv_clear_color);
    pb = push_command_parameter(pb, NV097_SET_ZSTENCIL_CLEAR_VALUE, zstencil);
    pb = push_command_parameter(pb, NV097_CLEAR_SURFACE, nv_clear_mask);
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
    memset(context, 0, sizeof(*context));

    while (pb_init() < 0) {
        gliDebugF("[nxdk renderer] pbkit initialization failed, retrying...\n");
    }

    pb_show_front_screen();
    pb_target_back_buffer();

    combiner_init();

    uint32_t *pb = pb_begin();
    pb = combiner_specular_fog_config(pb, GL_FALSE, GL_FALSE);
    pb_end(pb);

    /* --- Table 6.18: Pixel store --- */
    context->pixel_store.unpack_alignment = GLI_UNPACK_ALIGNMENT;
    context->pixel_store.pack_alignment = GLI_PACK_ALIGNMENT;

    /* --- Table 6.19: Hints --- */
    context->hints_state.perspective_correction_hint = GL_DONT_CARE;
    context->hints_state.point_smooth_hint = GL_DONT_CARE;
    context->hints_state.line_smooth_hint = GL_DONT_CARE;
    context->hints_state.fog_hint = GL_DONT_CARE;
    context->hints_state.generate_mipmap_hint = GL_DONT_CARE;

    /* --- Table 6.20: Implementation limits (seed with spec mins/zeros) --- */
    context->implementation_limits.max_lights = GLI_MAX_LIGHTS;
    context->implementation_limits.max_clip_planes = GLI_MAX_CLIP_PLANES;
    context->implementation_limits.max_modelview_stack_depth = GLI_MAX_MODELVIEW_STACK;
    context->implementation_limits.max_projection_stack_depth = GLI_MAX_PROJECTION_STACK;
    context->implementation_limits.max_texture_stack_depth = GLI_MAX_TEXTURE_STACK;
    context->implementation_limits.subpixel_bits = GLI_SUBPIXEL_BITS;
    context->implementation_limits.max_texture_size = GLI_MAX_TEXTURE_SIZE;
    context->implementation_limits.max_viewport_dims[0] = GLI_MAX_VIEWPORT_WIDTH;
    context->implementation_limits.max_viewport_dims[1] = GLI_MAX_VIEWPORT_HEIGHT;

    /* Point/line width ranges: seed with common safe values */
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

    /* --- Table 6.3: Current values --- */
    glm_vec4_copy(GLM_VEC4_ONE, context->current_values.current_color);
    for (int i = 0; i < GLI_MAX_TEXTURE_UNITS; ++i) {
        glm_vec4_copy(GLM_VEC4_BLACK, context->current_values.current_texcoord[i]);
    }
    glm_vec4_copy(GLM_ZUP, context->current_values.current_normal);

    /* --- Table 6.4/6.5: Client vertex array state --- */
    context->vertex_array_data.client_active_texture = GL_TEXTURE0;
    glActiveTexture(GL_TEXTURE0);

    /* Vertex */
    context->vertex_array_data.vertex_array_enabled = GL_FALSE;
    context->vertex_array_data.vertex_array_dirty = GL_TRUE;
    context->vertex_array_data.vertex_array_size = 4;
    context->vertex_array_data.vertex_array_type = GL_FLOAT;
    context->vertex_array_data.vertex_array_stride = 0;
    context->vertex_array_data.vertex_array_ptr = NULL;
    glVertexPointer(context->vertex_array_data.vertex_array_size,
                    context->vertex_array_data.vertex_array_type,
                    context->vertex_array_data.vertex_array_stride,
                    context->vertex_array_data.vertex_array_ptr);

    /* Normal (size is always 3 in ES 1.1) */
    context->vertex_array_data.normal_array_enabled = GL_FALSE;
    context->vertex_array_data.normal_array_dirty = GL_TRUE;
    context->vertex_array_data.normal_array_type = GL_FLOAT; /* BYTE/SHORT/FIXED/FLOAT allowed */
    context->vertex_array_data.normal_array_stride = 0;
    context->vertex_array_data.normal_array_ptr = NULL;
    glNormalPointer(context->vertex_array_data.normal_array_type,
                    context->vertex_array_data.normal_array_stride,
                    context->vertex_array_data.normal_array_ptr);

    /* Color (size must be 4 in ES 1.1) */
    context->vertex_array_data.color_array_enabled = GL_FALSE;
    context->vertex_array_data.color_array_dirty = GL_TRUE;
    context->vertex_array_data.color_array_size = 4;
    context->vertex_array_data.color_array_type = GL_FLOAT;
    context->vertex_array_data.color_array_stride = 0;
    context->vertex_array_data.color_array_ptr = NULL;
    glColorPointer(context->vertex_array_data.color_array_size,
                   context->vertex_array_data.color_array_type,
                   context->vertex_array_data.color_array_stride,
                   context->vertex_array_data.color_array_ptr);

    /* Texcoords per unit */
    for (int i = 0; i < GLI_MAX_TEXTURE_UNITS; ++i) {
        context->vertex_array_data.texcoord_array_enabled[i] = GL_FALSE;
        context->vertex_array_data.texcoord_array_dirty[i] = GL_TRUE;
        context->vertex_array_data.texcoord_array_size[i] = 4;
        context->vertex_array_data.texcoord_array_type[i] = GL_FLOAT; /* FIXED/FLOAT allowed */
        context->vertex_array_data.texcoord_array_stride[i] = 0;
        context->vertex_array_data.texcoord_array_ptr[i] = NULL;
        glActiveTexture(GL_TEXTURE0 + i);
        glTexCoordPointer(context->vertex_array_data.texcoord_array_size[i],
                          context->vertex_array_data.texcoord_array_type[i],
                          context->vertex_array_data.texcoord_array_stride[i],
                          context->vertex_array_data.texcoord_array_ptr[i]);
    }
    glActiveTexture(GL_TEXTURE0);

    /* OES_point_size_array (if present in your build; harmless defaults otherwise) */
    context->vertex_array_data.point_size_array_enabled = GL_FALSE;
    context->vertex_array_data.point_size_array_dirty = GL_TRUE;
    context->vertex_array_data.point_size_array_type = GL_FLOAT; /* or GL_FIXED (OES) */
    context->vertex_array_data.point_size_array_stride = 0;
    context->vertex_array_data.point_size_array_ptr = NULL;
    glPointSizePointerOES(context->vertex_array_data.point_size_array_type,
                          context->vertex_array_data.point_size_array_stride,
                          context->vertex_array_data.point_size_array_ptr);

    /* Buffer bindings (global) + any per-attribute names you track */
    context->vertex_array_data.array_buffer_binding = 0;
    context->vertex_array_data.vertex_array_buffer_binding = 0;
    context->vertex_array_data.normal_array_buffer_binding = 0;
    context->vertex_array_data.color_array_buffer_binding = 0;

    for (int i = 0; i < GLI_MAX_TEXTURE_UNITS; ++i) {
        context->vertex_array_data.texcoord_array_buffer_binding[i] = 0;
    }
    context->vertex_array_data.point_size_array_buffer_binding = 0;
    context->vertex_array_data.element_array_buffer_binding = 0;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /* --- Table 6.6: Buffer object state registry --- */
    context->buffer_objects = NULL;

    /* --- Table 6.7: Transformation state --- */
    for (int i = 0; i < GLI_MAX_MODELVIEW_STACK; ++i) {
        glm_mat4_identity(context->transformation_state.modelview_matrix_stack[i]);
    }
    for (int i = 0; i < GLI_MAX_PROJECTION_STACK; ++i) {
        glm_mat4_identity(context->transformation_state.projection_matrix_stack[i]);
    }
    for (int u = 0; u < GLI_MAX_TEXTURE_UNITS; ++u) {
        for (int i = 0; i < GLI_MAX_TEXTURE_STACK; ++i) {
            glm_mat4_identity(context->transformation_state.texture_matrix_stack[u][i]);
        }
    }

    context->transformation_state.viewport[0] = 0;
    context->transformation_state.viewport[1] = 0;
    context->transformation_state.viewport[2] = window_width;
    context->transformation_state.viewport[3] = window_height;
    glViewport(0, 0, window_width, window_height);

    context->transformation_state.depth_range[0] = 0.0f;
    context->transformation_state.depth_range[1] = 1.0f;
    glDepthRangef(context->transformation_state.depth_range[0], context->transformation_state.depth_range[1]);

    /* Each stack starts with one identity matrix */
    context->transformation_state.modelview_matrix_stack_depth = 1;
    context->transformation_state.projection_matrix_stack_depth = 1;
    for (int u = 0; u < GLI_MAX_TEXTURE_UNITS; ++u) {
        context->transformation_state.texture_matrix_stack_depth[u] = 1;
    }

    context->transformation_state.matrix_mode = GL_MODELVIEW;
    context->transformation_state.normalize_enabled = GL_FALSE;
    context->transformation_state.rescale_normal_enabled = GL_FALSE;

    for (int i = 0; i < GLI_MAX_CLIP_PLANES; ++i) {
        glm_vec4_copy(GLM_VEC4_ZERO, context->transformation_state.clip_plane[i]);
        context->transformation_state.clip_plane_enabled[i] = GL_FALSE;
    }
    context->transformation_state.clip_plane_dirty = GL_TRUE;

    /* --- Table 6.8: Coloring (fog & shading) --- */
    glm_vec4_copy(GLM_VEC4_ZERO, context->coloring_state.fog_color);
    context->coloring_state.fog_density = 1.0f;
    context->coloring_state.fog_start = 0.0f;
    context->coloring_state.fog_end = 1.0f;
    context->coloring_state.fog_mode = GL_EXP;
    context->coloring_state.fog_enabled = GL_FALSE;
    context->coloring_state.fog_dirty = GL_TRUE;
    glFogf(GL_FOG_DENSITY, context->coloring_state.fog_density);
    glFogf(GL_FOG_START, context->coloring_state.fog_start);
    glFogf(GL_FOG_END, context->coloring_state.fog_end);
    glFogf(GL_FOG_MODE, context->coloring_state.fog_mode);
    glDisable(GL_FOG);

    context->coloring_state.shade_model = GL_SMOOTH;
    glShadeModel(context->coloring_state.shade_model);

    context->lighting_state.lighting_enabled = GL_FALSE;
    glDisable(GL_LIGHTING);
    context->lighting_state.color_material_enabled = GL_FALSE;
    glDisable(GL_COLOR_MATERIAL);

    glm_vec2_copy((vec4){0.2f, 0.2f, 0.2f, 1.0f}, context->lighting_state.material_front.ambient);
    glm_vec2_copy((vec4){0.8f, 0.8f, 0.8f, 1.0f}, context->lighting_state.material_front.diffuse);
    glm_vec4_copy(GLM_VEC4_BLACK, context->lighting_state.material_front.specular);
    glm_vec4_copy(GLM_VEC4_BLACK, context->lighting_state.material_front.emission);
    context->lighting_state.material_front.shininess = 0.0f;
    context->lighting_state.material_front.material_dirty = GL_TRUE;

    glm_vec4_copy((vec4){0.2f, 0.2f, 0.2f, 1.0f}, context->lighting_state.material_back.ambient);
    glm_vec4_copy((vec4){0.8f, 0.8f, 0.8f, 1.0f}, context->lighting_state.material_back.diffuse);
    glm_vec4_copy(GLM_VEC4_BLACK, context->lighting_state.material_back.specular);
    glm_vec4_copy(GLM_VEC4_BLACK, context->lighting_state.material_back.emission);
    context->lighting_state.material_back.shininess = 0.0f;
    context->lighting_state.material_back.material_dirty = GL_TRUE;

    glm_vec4_copy((vec4){0.2f, 0.2f, 0.2f, 1.0f}, context->lighting_state.light_model_ambient);
    context->lighting_state.lighting_model_dirty = GL_TRUE;
    context->lighting_state.light_model_two_side = GL_FALSE;

    for (int i = 0; i < GLI_MAX_LIGHTS; ++i) {
        light_t *light = &context->lighting_state.lights[i];

        glm_vec4_copy(GLM_VEC4_BLACK, light->ambient);
        if (i == 0) {
            glm_vec4_copy(GLM_VEC4_ONE, light->diffuse);
            glm_vec4_copy(GLM_VEC4_ONE, light->specular);

        } else {
            glm_vec4_copy(GLM_VEC4_BLACK, light->diffuse);
            glm_vec4_copy(GLM_VEC4_BLACK, light->specular);
        }

        glm_vec4_copy((vec4){0.0f, 0.0f, 1.0f, 0.0f}, light->position);
        light->constant_attenuation = 1.0f;
        light->linear_attenuation = 0.0f;
        light->quadratic_attenuation = 0.0f;
        glm_vec3_copy(GLM_FORWARD, light->spot_direction);
        light->spot_exponent = 0.0f;
        light->spot_cutoff = 180.0f; /* disabled */
        light->enabled = GL_FALSE;
        light->light_dirty = GL_TRUE;
    }

    /* --- Table 6.11: Rasterization --- */
    /* Points */
    context->rasterization_state.point_size = 1.0f;
    context->rasterization_state.point_smooth_enabled = GL_FALSE;
    context->rasterization_state.point_size_min = 0.0f;
    context->rasterization_state.point_size_max = 1.0f;
    context->rasterization_state.point_fade_threshold_size = 1.0f;
    context->rasterization_state.point_distance_attenuation[0] = 1.0f;
    context->rasterization_state.point_distance_attenuation[1] = 0.0f;
    context->rasterization_state.point_distance_attenuation[2] = 0.0f;
    context->rasterization_state.point_sprite_oes_enabled = GL_FALSE; /* OES_point_sprite */
    context->rasterization_state.point_params_dirty = GL_TRUE;
    glPointSize(context->rasterization_state.point_size);
    glDisable(GL_POINT_SMOOTH);
    glPointParameterf(GL_POINT_SIZE_MIN, context->rasterization_state.point_size_min);
    glPointParameterf(GL_POINT_SIZE_MAX, context->rasterization_state.point_size_max);
    glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, context->rasterization_state.point_fade_threshold_size);
    glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, context->rasterization_state.point_distance_attenuation);
    glDisable(GL_POINT_SPRITE_OES);

    /* Lines */
    context->rasterization_state.line_width = 1.0f;
    context->rasterization_state.line_smooth_enabled = GL_FALSE;
    glLineWidth(context->rasterization_state.line_width);
    glDisable(context->rasterization_state.line_smooth_enabled);

    /* Polygons / culling / offset */
    context->rasterization_state.cull_face_enabled = GL_FALSE;
    context->rasterization_state.cull_face_mode = GL_BACK;
    context->rasterization_state.cull_front_face = GL_CCW;
    context->rasterization_state.polygon_offset_factor = 0.0f;
    context->rasterization_state.polygon_offset_units = 0.0f;
    context->rasterization_state.polygon_offset_fill_enabled = GL_FALSE;
    glDisable(GL_CULL_FACE);
    glCullFace(context->rasterization_state.cull_face_mode);
    glFrontFace(context->rasterization_state.cull_front_face);
    glPolygonOffset(context->rasterization_state.polygon_offset_factor,
                    context->rasterization_state.polygon_offset_units);
    glDisable(GL_POLYGON_OFFSET_FILL);

    /* --- Table 6.12: Multisampling --- */
    context->multisampling_state.multisample_enabled = GL_TRUE;
    context->multisampling_state.sample_alpha_to_coverage_enabled = GL_FALSE;
    context->multisampling_state.sample_alpha_to_one_enabled = GL_FALSE;
    context->multisampling_state.sample_coverage_enabled = GL_FALSE;
    context->multisampling_state.sample_coverage_value = 1.0f;
    context->multisampling_state.sample_coverage_invert = GL_FALSE;
    glEnable(GL_MULTISAMPLE);
    glSampleCoverage(1.0f, GL_FALSE);

    /* --- Tables 6.13/6.14: Textures & TexEnv (per unit) --- */
    context->texture_environment.server_active_texture = GL_TEXTURE0;
    context->texture_environment.texture_objects = NULL;

    // Set up default texture unit that is used for all units initially
    for (GLint i = 0; i < GLI_MAX_TEXTURE_UNITS; ++i) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];
        texture_unit->texture_unit_dirty = GL_TRUE;
        texture_unit->texture_2d_enabled = GL_FALSE;
        texture_unit->bound_texture_object = &texture_unit->unbound_texture_object;
        {
            // According to spec all texture units have same default texture binding with 0 name
            texture_unit->unbound_texture_object.texture_name = 0;
            texture_unit->unbound_texture_object.texture_object_dirty = GL_TRUE;
            texture_unit->unbound_texture_object.min_filter = GL_NEAREST_MIPMAP_LINEAR;
            texture_unit->unbound_texture_object.mag_filter = GL_LINEAR;
            texture_unit->unbound_texture_object.wrap_s = GL_REPEAT;
            texture_unit->unbound_texture_object.wrap_t = GL_REPEAT;
            texture_unit->unbound_texture_object.generate_mipmap = GL_FALSE;
        }
        texture_unit->texture_binding_2d = 0;
        texture_unit->tex_env_mode = GL_MODULATE;
        texture_unit->coord_replace_oes_enabled = GL_FALSE;
        glm_vec4_zero(texture_unit->tex_env_color);

        texture_unit->combine_rgb_function = GL_MODULATE;
        texture_unit->combine_alpha_function = GL_MODULATE;
        texture_unit->combine_rgb_source[0] = GL_TEXTURE;
        texture_unit->combine_rgb_source[1] = GL_PREVIOUS;
        texture_unit->combine_rgb_source[2] = GL_CONSTANT;
        texture_unit->combine_alpha_source[0] = GL_TEXTURE;
        texture_unit->combine_alpha_source[1] = GL_PREVIOUS;
        texture_unit->combine_alpha_source[2] = GL_CONSTANT;
        texture_unit->combine_rgb_operand[0] = GL_SRC_COLOR;
        texture_unit->combine_rgb_operand[1] = GL_SRC_COLOR;
        texture_unit->combine_rgb_operand[2] = GL_SRC_ALPHA;
        texture_unit->combine_alpha_operand[0] = GL_SRC_ALPHA;
        texture_unit->combine_alpha_operand[1] = GL_SRC_ALPHA;
        texture_unit->combine_alpha_operand[2] = GL_SRC_ALPHA;
        texture_unit->rgb_scale = 1.0f;
        texture_unit->alpha_scale = 1.0f;
    }

    /* --- Table 6.16: Per-fragment / pixel ops --- */
    context->pixel_ops_state.scissor_test_enabled = GL_FALSE;
    context->pixel_ops_state.scissor_box[0] = 0;
    context->pixel_ops_state.scissor_box[1] = 0;
    context->pixel_ops_state.scissor_box[2] = window_width;
    context->pixel_ops_state.scissor_box[3] = window_height;
    glDisable(GL_SCISSOR_TEST);

    context->pixel_ops_state.alpha_test_enabled = GL_FALSE;
    context->pixel_ops_state.alpha_test_func = GL_ALWAYS;
    context->pixel_ops_state.alpha_test_ref = 0.0f;
    glDisable(GL_ALPHA_TEST);
    glAlphaFunc(GL_ALWAYS, 0.0f);

    context->pixel_ops_state.stencil_test_enabled = GL_FALSE;
    context->pixel_ops_state.stencil_func = GL_ALWAYS;
    context->pixel_ops_state.stencil_value_mask = ~0u;
    context->pixel_ops_state.stencil_ref = 0;
    context->pixel_ops_state.stencil_fail_op = GL_KEEP;
    context->pixel_ops_state.stencil_zfail_op = GL_KEEP;
    context->pixel_ops_state.stencil_zpass_op = GL_KEEP;
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, ~0u);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    context->pixel_ops_state.depth_test_enabled = GL_FALSE;
    context->pixel_ops_state.depth_func = GL_LESS;
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    context->pixel_ops_state.blend_enabled = GL_FALSE;
    context->pixel_ops_state.blend_src = GL_ONE;
    context->pixel_ops_state.blend_dst = GL_ZERO;
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);

    context->pixel_ops_state.dither_enabled = GL_TRUE;
    glEnable(GL_DITHER);

    context->pixel_ops_state.color_logic_op_enabled = GL_FALSE;
    context->pixel_ops_state.color_logic_op = GL_COPY;
    glDisable(GL_COLOR_LOGIC_OP);
    glLogicOp(context->pixel_ops_state.color_logic_op);

    /* --- Table 6.17: Framebuffer control --- */
    context->framebuffer_control.color_mask[0] = GL_TRUE;
    context->framebuffer_control.color_mask[1] = GL_TRUE;
    context->framebuffer_control.color_mask[2] = GL_TRUE;
    context->framebuffer_control.color_mask[3] = GL_TRUE;
    context->framebuffer_control.depth_writemask = GL_TRUE;
    context->framebuffer_control.stencil_writemask = ~0u;
    glColorMask(context->framebuffer_control.color_mask[0],
                context->framebuffer_control.color_mask[1],
                context->framebuffer_control.color_mask[2],
                context->framebuffer_control.color_mask[3]);
    glDepthMask(GL_TRUE);
    glStencilMask(context->framebuffer_control.stencil_writemask);

    glm_vec4_copy(GLM_VEC4_ZERO, context->framebuffer_control.clear_color);
    context->framebuffer_control.clear_depth = 1.0f;
    context->framebuffer_control.clear_stencil = 0;
    glClearColor(context->framebuffer_control.clear_color[0],
                 context->framebuffer_control.clear_color[1],
                 context->framebuffer_control.clear_color[2],
                 context->framebuffer_control.clear_color[3]);
    glClearDepthf(context->framebuffer_control.clear_depth);
    glClearStencil(context->framebuffer_control.clear_stencil);

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
