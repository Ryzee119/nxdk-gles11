#include "gles_private.h"

enum gli_get_type
{
    GLI_BOOLEAN,
    GLI_INT,
    GLI_FLOAT,
};

static const void *gliGetElementPtr(GLenum pname, enum gli_get_type *element_type, GLint *element_count);

GL_API void GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean *data)
{
    enum gli_get_type element_type;
    GLint element_count;
    const void *src = gliGetElementPtr(pname, &element_type, &element_count);
    if (element_type == -1 || element_count == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (data == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    assert(src != NULL);

    // If we match type, its a simple copy
    if (element_type == GLI_BOOLEAN) {
        for (GLint i = 0; i < element_count; ++i) {
            data[i] = ((GLboolean *)src)[i];
        }
    }

    // If GetBooleanv is called, a floating-point or integer value converts to GL_FALSE if and only
    // if it is zero (otherwise it converts to GL_TRUE)
    else if (element_type == GLI_INT) {
        for (GLint i = 0; i < element_count; ++i) {
            data[i] = (((GLint *)src)[i] == 0) ? GL_FALSE : GL_TRUE;
        }

    } else if (element_type == GLI_FLOAT) {
        for (GLint i = 0; i < element_count; ++i) {
            data[i] = (((GLfloat *)src)[i] == 0.0f) ? GL_FALSE : GL_TRUE;
        }
    }
}

GL_API void GL_APIENTRY glGetIntegerv(GLenum pname, GLint *data)
{
    enum gli_get_type element_type;
    GLint element_count;
    const void *src = gliGetElementPtr(pname, &element_type, &element_count);
    GLint *dst = (GLint *)data;
    if (element_type == -1 || element_count == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (data == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    assert(src != NULL);

    // If we match type, its a simple copy, except uint to int needs clamping
    if (element_type == GLI_INT) {
        for (GLint i = 0; i < element_count; ++i) {
            dst[i] = ((GLint *)src)[i];
        }
    }

    // Floating-point value is rounded to the nearest integer, unless the value is an RGBA color component, a
    // DepthRange value, a depth buffer clear value, or a normal coordinate. In these cases, the Get command does a
    // linear mapping that maps 1.0 to the most positive representable integer value, and -1.0 to the most negative
    // representable integer value.
    switch (pname) {
        case GL_COLOR_CLEAR_VALUE:
        case GL_CURRENT_COLOR:
        case GL_FOG_COLOR:
        case GL_LIGHT_MODEL_AMBIENT:
        case GL_DEPTH_CLEAR_VALUE:
        case GL_DEPTH_RANGE:
        case GL_CURRENT_NORMAL:
            assert(element_type == GLI_FLOAT);
            const GLfloat *fsrc = (const GLfloat *)src;
            for (GLint i = 0; i < element_count; i++) {
                GLfloat val = glm_clamp(fsrc[i], -1.0f, 1.0f);
                dst[i] = FLOAT_TO_INT(val);
            }
            return;
        default:
            // Handle normal rounding from float to int
            if (element_type == GLI_FLOAT) {
                const GLfloat *fsrc = (const GLfloat *)src;
                for (GLint i = 0; i < element_count; ++i) {
                    dst[i] = (GLint)roundf(fsrc[i]);
                }
            }
            break;
    }

    // Boolean value is interpreted as either 1 or 0
    if (element_type == GLI_BOOLEAN) {
        const GLboolean *bsrc = (const GLboolean *)src;
        for (GLint i = 0; i < element_count; ++i) {
            dst[i] = bsrc[i] ? 1 : 0;
        }
    }
}

GL_API void GL_APIENTRY glGetFloatv(GLenum pname, GLfloat *data)
{
    enum gli_get_type element_type;
    GLint element_count;
    const void *src = gliGetElementPtr(pname, &element_type, &element_count);
    GLfloat *dst = (GLfloat *)data;

    if (element_type == -1 || element_count == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (data == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    assert(src != NULL);

    // If we match type, its a simple copy
    if (element_type == GLI_FLOAT) {
        const GLfloat *fsrc = (const GLfloat *)src;
        for (GLint i = 0; i < element_count; ++i) {
            dst[i] = fsrc[i];
        }
    }

    // An integer is coerced to floating-point
    // If a value is so large in magnitude that it cannot be represented with the requested type,
    // then the nearest value representable using the requested type is returned.
    else if (element_type == GLI_INT) {
        const GLint *isrc = (const GLint *)src;
        for (GLint i = 0; i < element_count; ++i) {
            dst[i] = (GLfloat)isrc[i];
        }

        // A boolean value is interpreted as either 1.0 or 0.0
    } else if (element_type == GLI_BOOLEAN) {
        const GLboolean *bsrc = (const GLboolean *)src;
        for (GLint i = 0; i < element_count; ++i) {
            dst[i] = bsrc[i] ? 1.0f : 0.0f;
        }
    }
}

GL_API void GL_APIENTRY glGetFixedv(GLenum pname, GLfixed *data)
{
    enum gli_get_type element_type;
    GLint element_count;

    const void *src = gliGetElementPtr(pname, &element_type, &element_count);

    if (element_type == -1 || element_count == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (data == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    assert(src != NULL);

    // Get a local copy as float and convert to fixed
    GLfloat dest[16];
    glGetFloatv(pname, dest);
    for (int i = 0; i < element_count; i++) {
        data[i] = gliFloattoFixed(dest[i]);
    }
}

GL_API void GL_APIENTRY glGetPointerv(GLenum pname, void **params)
{
    gli_context_t *context = gliGetContext();

    if (params == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
        case GL_COLOR_ARRAY_POINTER:
            *params = (void *)context->vertex_array_data.color_array_ptr;
            return;
        case GL_NORMAL_ARRAY_POINTER:
            *params = (void *)context->vertex_array_data.normal_array_ptr;
            return;
        case GL_POINT_SIZE_ARRAY_POINTER_OES:
            *params = (void *)context->vertex_array_data.point_size_array_ptr;
            return;
        case GL_TEXTURE_COORD_ARRAY_POINTER:
            *params = (void *)context->vertex_array_data
                          .texcoord_array_ptr[context->vertex_array_data.client_active_texture - GL_TEXTURE0];
            return;
        case GL_VERTEX_ARRAY_POINTER:
            *params = (void *)context->vertex_array_data.vertex_array_ptr;
            return;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

static const void *gliGetElementPtr(GLenum pname, enum gli_get_type *element_type, GLint *element_count)
{
    gli_context_t *context = gliGetContext();
    const GLint tu = context->texture_environment.server_active_texture - GL_TEXTURE0;

    switch (pname) {
        case GL_ACTIVE_TEXTURE:
            // params returns a single value indicating the active multitexture unit. The initial value is GL_TEXTURE0.
            // See glActiveTexture.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->texture_environment.server_active_texture;
        case GL_ALIASED_POINT_SIZE_RANGE:
            // params returns two values, the smallest and largest supported sizes for aliased points. The range must
            // include 1. See glPointSize.
            *element_type = GLI_FLOAT;
            *element_count = 2;
            return context->implementation_limits.aliased_point_size_range;
        case GL_ALIASED_LINE_WIDTH_RANGE:
            // params returns two values, the smallest and largest supported widths for aliased lines. The range must
            // include 1. See glLineWidth.
            *element_type = GLI_FLOAT;
            *element_count = 2;
            return context->implementation_limits.aliased_line_width_range;
        case GL_ALPHA_BITS:
            // params returns one value, the number of alpha bitplanes in the color buffer.
            *element_type = GLI_INT;
            *element_count = 1;
            //// FIXME. 8 for RGBA8888 etc
            assert(0);
            return NULL;
        case GL_ALPHA_TEST:
            // params returns a single boolean value indicating whether alpha testing of fragments is enabled. The
            // initial value is GL_FALSE. See glAlphaFunc.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->pixel_ops_state.alpha_test_enabled;
        case GL_ALPHA_TEST_FUNC:
            // params returns one value, the symbolic name of the alpha test function. See glAlphaFunc.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.alpha_test_func;
        case GL_ALPHA_TEST_REF:
            // params returns one value, the reference value for the alpha test. An integer value, if requested, is
            // linearly mapped from the internal floating-point representation such that 1.0 returns the most positive
            // representable integer value, and -1.0 returns the most negative representable integer value. See
            // glAlphaFunc.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->pixel_ops_state.alpha_test_ref;
        case GL_ARRAY_BUFFER_BINDING:
            // params returns a single value, the name of the buffer object currently bound to the target
            // GL_ARRAY_BUFFER. If no buffer object is bound to this target, 0 is returned. The initial value is 0. See
            // glBindBuffer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.array_buffer_binding;
        case GL_BLEND:
            // params returns a single boolean value indicating whether blending of fragments is enabled. The initial
            // value is GL_FALSE. See glBlendFunc and glLogicOp.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->pixel_ops_state.blend_enabled;
        case GL_BLEND_DST:
            // params returns one value, the symbolic constant identifying the destination blend function. See
            // glBlendFunc.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.blend_dst;
        case GL_BLEND_SRC:
            // params returns one value, the symbolic constant identifying the source blend function. See glBlendFunc.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.blend_src;
        case GL_BLUE_BITS:
            // params returns one value, the number of blue bitplanes in the color buffer.
            *element_type = GLI_INT;
            *element_count = 1;
            //// FIXME. 8 for RGBA8888 etc
            assert(0);
            return NULL;
        case GL_CLIENT_ACTIVE_TEXTURE:
            // params returns a single value indicating the current client active multitexture unit. The initial value
            // is GL_TEXTURE0. See glClientActiveTexture.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.client_active_texture;
        case GL_CLIP_PLANE0 ...(GL_CLIP_PLANE0 + GLI_MAX_CLIP_PLANES - 1):
            // params returns a single boolean value indicating whether the ith user clipping plane is enabled. The
            // initial value is GL_FALSE. See glClipPlane.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->transformation_state.clip_plane_enabled[pname - GL_CLIP_PLANE0];
        case GL_COLOR_ARRAY:
            // params returns a single boolean value indicating whether the color array is enabled. The initial value is
            // GL_FALSE. See glColorPointer.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->vertex_array_data.color_array_enabled;
        case GL_COLOR_ARRAY_BUFFER_BINDING:
            // params returns one value, the color array buffer binding. See glColorPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.color_array_buffer_binding;
        case GL_COLOR_ARRAY_SIZE:
            // params returns one value, the number of components per color in the color array. See glColorPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.color_array_size;
        case GL_COLOR_ARRAY_STRIDE:
            // params returns one value, the byte offset between consecutive colors in the color array. See
            // glColorPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.color_array_stride;
        case GL_COLOR_ARRAY_TYPE:
            // params returns one value, returns the data type of each component in the color array. See glColorPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.color_array_type;
        case GL_COLOR_CLEAR_VALUE:
            // params returns four values: the red, green, blue, and alpha values used to clear the color buffers. See
            // glClearColor.
            *element_type = GLI_FLOAT;
            *element_count = 4;
            return (GLfloat *)context->framebuffer_control.clear_color;
        case GL_COLOR_LOGIC_OP:
            // params returns a single boolean value indicating whether logical operation on color values is enabled.
            // The initial value is GL_FALSE. See glLogicOp.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->pixel_ops_state.color_logic_op_enabled;
        case GL_COLOR_MATERIAL:
            // params returns a single boolean value indicating whether color material tracking is enabled. The initial
            // value is GL_FALSE. See glMaterial.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->lighting_state.color_material_enabled;
        case GL_COLOR_WRITEMASK:
            // params returns four boolean values: the red, green, blue, and alpha write enables for the color buffers.
            // See glColorMask.
            *element_type = GLI_BOOLEAN;
            *element_count = 4;
            return context->framebuffer_control.color_mask;
        case GL_COMPRESSED_TEXTURE_FORMATS:
            // params returns GL_NUM_COMPRESSED_TEXTURE_FORMATS values, the supported compressed texture formats. See
            // glCompressedTexImage2D and glCompressedTexSubImage2D.
            *element_type = GLI_INT;
            *element_count = context->implementation_limits.num_compressed_texture_formats;
            return context->implementation_limits.compressed_texture_formats;
        case GL_CULL_FACE:
            // params returns a single boolean value indicating whether polygon culling is enabled. The initial value is
            // GL_FALSE. See glCullFace.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->rasterization_state.cull_face_enabled;
        case GL_CULL_FACE_MODE:
            // params returns one value, a symbolic constant indicating which polygon faces are to be culled. The
            // initial value is GL_BACK. See glCullFace.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->rasterization_state.cull_face_mode;
        case GL_CURRENT_COLOR:
            // params returns four values: the red, green, blue, and alpha values of the current color. Integer values,
            // if requested, are linearly mapped from the internal floating-point representation such that 1.0 returns
            // the most positive representable integer value, and -1.0 returns the most negative representable integer
            // value. The initial value is (1, 1, 1, 1). See glColor.
            *element_type = GLI_FLOAT;
            *element_count = 4;
            return (GLfloat *)context->current_values.current_color;
        case GL_CURRENT_NORMAL:
            // params returns three values: the x, y, and z values of the current normal. Integer values, if requested,
            // are linearly mapped from the internal floating-point representation such that 1.0 returns the most
            // positive representable integer value, and -1.0 returns the most negative representable integer value. The
            // initial value is (0, 0, 1). See glNormal.
            *element_type = GLI_FLOAT;
            *element_count = 3;
            return (GLfloat *)context->current_values.current_normal;
        case GL_CURRENT_TEXTURE_COORDS:
            // params returns four values: the s, t, r, and q current texture coordinates. The initial value is (0, 0,
            // 0, 1). See glMultiTexCoord.
            *element_type = GLI_FLOAT;
            *element_count = 4;
            return (GLfloat *)context->current_values.current_texcoord[tu];
        case GL_DEPTH_BITS:
            // params returns one value, the number of bitplanes in the depth buffer.
            // FIXME. 24 for D24S8 etc
            *element_type = GLI_INT;
            *element_count = 1;
            assert(0);
            return NULL;
            break;
        case GL_DEPTH_CLEAR_VALUE:
            // params returns one value, the value that is used to clear the depth buffer. See glClearDepth.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->framebuffer_control.clear_depth;
        case GL_DEPTH_FUNC:
            // params returns one value, the symbolic name of the depth comparison function. See glDepthFunc.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.depth_func;
        case GL_DEPTH_RANGE:
            // params returns two values: the near and far mapping limits for the depth buffer. See glDepthRange.
            *element_type = GLI_FLOAT;
            *element_count = 2;
            return context->transformation_state.depth_range;
        case GL_DEPTH_TEST:
            // params returns a single boolean value indicating whether depth testing of fragments is enabled. The
            // initial value is GL_FALSE. See glDepthFunc and glDepthRange.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->pixel_ops_state.depth_test_enabled;
        case GL_DEPTH_WRITEMASK:
            // params returns a single boolean value indicating if the depth buffer is enabled for writing. See
            // glDepthMask.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->framebuffer_control.depth_writemask;
        case GL_ELEMENT_ARRAY_BUFFER_BINDING:
            // params returns a single value, the name of the buffer object currently bound to the target
            // GL_ELEMENT_ARRAY_BUFFER. If no buffer object is bound to this target, 0 is returned. The initial value is
            // 0. See glBindBuffer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.element_array_buffer_binding;
        case GL_FOG:
            // params returns a single boolean value indicating whether fog is enabled. The initial value is GL_FALSE.
            // See glFog.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->coloring_state.fog_enabled;
        case GL_FOG_COLOR:
            // params returns four values: the red, green, blue, and alpha components of the fog color. See glFog.
            *element_type = GLI_FLOAT;
            *element_count = 4;
            return (GLfloat *)context->coloring_state.fog_color;
        case GL_FOG_DENSITY:
            // params returns one value, the fog density parameter. See glFog.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->coloring_state.fog_density;
        case GL_FOG_END:
            // params returns one value, the end factor for the linear fog equation. See glFog.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->coloring_state.fog_end;
        case GL_FOG_HINT:
            // params returns one value, a symbolic constant indicating the mode of the fog hint. See glHint.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->hints_state.fog_hint;
        case GL_FOG_MODE:
            // params returns one value, a symbolic constant indicating which fog equation is selected. See glFog.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->coloring_state.fog_mode;
        case GL_FOG_START:
            // params returns one value, the start factor for the linear fog equation. See glFog.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->coloring_state.fog_start;
        case GL_FRONT_FACE:
            // params returns one value, a symbolic constant indicating whether clockwise or counterclockwise polygon
            // winding is treated as front-facing. See glFrontFace.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->rasterization_state.cull_front_face;
        case GL_GREEN_BITS:
            // params returns one value, the number of green bitplanes in the color buffer.
            *element_type = GLI_INT;
            *element_count = 1;
            //// FIXME. 8 for RGBA8888 etc
            assert(0);
            return NULL;
        case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
            // params returns one value, the preferred format for pixel read back. See glReadPixels.
            *element_type = GLI_INT;
            *element_count = 1;
            // FIXME. Probably use GL_RGBA but add a ifdef
            assert(0);
            return NULL;
        case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
            // params returns one value, the preferred type for pixel read back. See glReadPixels.
            *element_type = GLI_INT;
            *element_count = 1;
            // FIXME. GL_UNSIGNED_BYTE
            assert(0);
            return NULL;
        case GL_LIGHT_MODEL_AMBIENT:
            // params returns four values: the red, green, blue, and alpha components of the ambient intensity of the
            // entire scene. See glLightModel.
            *element_type = GLI_FLOAT;
            *element_count = 4;
            return (GLfloat *)context->lighting_state.light_model_ambient;
        case GL_LIGHT_MODEL_TWO_SIDE:
            // params returns a single boolean value indicating whether separate materials are used to compute lighting
            // for front and back facing polygons. See glLightModel.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->lighting_state.light_model_two_side;
        case GL_LIGHT0 ...(GL_LIGHT0 + GLI_MAX_LIGHTS - 1):
            // params returns a single boolean value indicating whether the ith light is enabled. The initial value is
            // GL_FALSE. See glLight and glLightModel.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->lighting_state.lights[pname - GL_LIGHT0].enabled;
            break;
        case GL_LIGHTING:
            // params returns a single boolean value indicating whether lighting is enabled. The initial value is
            // GL_FALSE. See glLight, glLightModel, and glMaterial.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->lighting_state.lighting_enabled;
        case GL_LINE_SMOOTH:
            // params returns a single boolean value indicating whether line antialiasing is enabled. The initial value
            // is GL_FALSE. See glLineWidth.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->rasterization_state.line_smooth_enabled;
        case GL_LINE_SMOOTH_HINT:
            // params returns one value, a symbolic constant indicating the mode of the line antialiasing hint. See
            // glHint.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->hints_state.line_smooth_hint;
        case GL_LINE_WIDTH:
            // params returns one value, the line width as specified with glLineWidth.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->rasterization_state.line_width;
        case GL_LOGIC_OP_MODE:
            // params returns one value, a symbolic constant indicating the selected logic operation mode. See
            // glLogicOp.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.color_logic_op;
        case GL_MATRIX_MODE:
            // params returns one value, a symbolic constant indicating which matrix stack is currently the target of
            // all matrix operations. See glMatrixMode.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->transformation_state.matrix_mode;
        case GL_MAX_CLIP_PLANES:
            // params returns one value, the maximum number of application defined clipping planes. The value must be at
            // least 1. See glClipPlane.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.max_clip_planes;
        case GL_MAX_LIGHTS:
            // params returns one value, the maximum number of lights. The value must be at least 8. See glLight.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.max_lights;
        case GL_MAX_MODELVIEW_STACK_DEPTH:
            // params returns one value, the maximum supported depth of the modelview matrix stack. The value must be at
            // least 16. See glPushMatrix.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.max_modelview_stack_depth;
        case GL_MAX_PROJECTION_STACK_DEPTH:
            // params returns one value, the maximum supported depth of the projection matrix stack. The value must be
            // at least 2. See glPushMatrix.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.max_projection_stack_depth;
        case GL_MAX_TEXTURE_SIZE:
            // params returns one value. The value gives a rough estimate of the largest texture that the GL can handle.
            // The value must be at least 64. See glTexImage2D, glCompressedTexImage2D, and glCopyTexImage2D.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.max_texture_size;
        case GL_MAX_TEXTURE_STACK_DEPTH:
            // params returns one value, the maximum supported depth of the texture matrix stack. The value must be at
            // least 2. See glPushMatrix.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.max_texture_stack_depth;
        case GL_MAX_TEXTURE_UNITS:
            // params returns a single value indicating the number of texture units supported. The value must be at
            // least 1. See glActiveTexture, glClientActiveTexture and glMultiTexCoord.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.max_texture_units;
        case GL_MAX_VIEWPORT_DIMS:
            // params returns two values: the maximum supported width and height of the viewport. These must be at least
            // as large as the visible dimensions of the display being rendered to. See glViewport.
            *element_type = GLI_INT;
            *element_count = 2;
            return context->implementation_limits.max_viewport_dims;
        case GL_MODELVIEW_MATRIX:
            // params returns sixteen values: the modelview matrix on the top of the modelview matrix stack. See
            // glPushMatrix.
            *element_type = GLI_FLOAT;
            *element_count = 16;
            return *gliCurrentModelView();
        case GL_MODELVIEW_STACK_DEPTH:
            // params returns one value, the number of matrices on the modelview matrix stack. See glPushMatrix.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->transformation_state.modelview_matrix_stack_depth;
        case GL_MULTISAMPLE:
            // params returns a single boolean value indicating whether multisampling is enabled. The initial value is
            // GL_TRUE.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->multisampling_state.multisample_enabled;
        case GL_NORMAL_ARRAY:
            // params returns a single boolean value indicating whether the normal array is enabled. The initial value
            // is GL_FALSE. See glNormalPointer.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->vertex_array_data.normal_array_enabled;
        case GL_NORMAL_ARRAY_BUFFER_BINDING:
            // params returns one value, the normal array buffer binding. See glNormalPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.normal_array_buffer_binding;
        case GL_NORMAL_ARRAY_STRIDE:
            // params returns one value, the byte offset between consective normals in the normal array. See
            // glNormalPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.normal_array_stride;
        case GL_NORMAL_ARRAY_TYPE:
            // params returns one value, the data type of each normal in the normal array. See glNormalPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.normal_array_type;
        case GL_NORMALIZE:
            // params returns a single boolean value indicating whether normalization of normals is enabled. The initial
            // value is GL_FALSE. See glNormal.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->transformation_state.normalize_enabled;
        case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
            // params returns one value, the number of supported compressed texture formats. See glCompressedTexImage2D
            // and glCompressedTexSubImage2D.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.num_compressed_texture_formats;
        case GL_PACK_ALIGNMENT:
            // params returns one value, the byte alignment used for writing pixel data to memory. See glPixelStorei.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_store.pack_alignment;
        case GL_PERSPECTIVE_CORRECTION_HINT:
            // params returns one value, a symbolic constant indicating the mode of the perspective correction hint. See
            // glHint.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->hints_state.perspective_correction_hint;
        case GL_POINT_DISTANCE_ATTENUATION:
            // params returns three values, the distance attenuation function coefficients a, b, and c. The initial
            // value is (1, 0, 0). See glPointParameter.
            *element_type = GLI_FLOAT;
            *element_count = 3;
            return context->rasterization_state.point_distance_attenuation;
        case GL_POINT_FADE_THRESHOLD_SIZE:
            // params returns one value, the point fade threshold. The initial value is 1. See glPointParameter.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->rasterization_state.point_fade_threshold_size;
        case GL_POINT_SIZE:
            // params returns one value, the point size as specified by glPointSize.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->rasterization_state.point_size;
        case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
            // params returns one value, the point size array buffer binding. See glPointSizePointerOES.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.point_size_array_buffer_binding;
        case GL_POINT_SIZE_ARRAY_OES:
            // params returns a single boolean value indicating whether the point size array is enabled. The initial
            // value is GL_FALSE. See glPointSizePointerOES.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->vertex_array_data.point_size_array_enabled;
        case GL_POINT_SIZE_ARRAY_STRIDE_OES:
            // params returns one value, the byte offset between consecutive point sizes in the point size array. See
            // glPointSizePointerOES.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.point_size_array_stride;
        case GL_POINT_SIZE_ARRAY_TYPE_OES:
            // params returns one value, the data type of each point size in the point array. See glPointSizePointerOES.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.point_size_array_type;
        case GL_POINT_SIZE_MAX:
            // params returns one value, the upper bound to which the derived point size is clamped. The initial value
            // is the maximum of the implementation dependent max aliased and smooth point sizes. See glPointParameter.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->rasterization_state.point_size_max;
        case GL_POINT_SIZE_MIN:
            // params returns one value, the lower bound to which the derived point size is clamped. The initial value
            // is 0. See glPointParameter.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->rasterization_state.point_size_min;
        case GL_POINT_SMOOTH:
            // params returns a single boolean value indicating whether point antialiasing is enabled. The initial value
            // is GL_FALSE. See glPointSize.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->rasterization_state.point_smooth_enabled;
        case GL_POINT_SMOOTH_HINT:
            // params returns one value, a symbolic constant indicating the mode of the point antialiasing hint. See
            // glHint.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->hints_state.point_smooth_hint;
        case GL_POINT_SPRITE_OES:
            // params returns a single boolean value indicating whether point sprites are enabled. The initial value is
            // GL_FALSE. See glTexEnv.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->rasterization_state.point_sprite_oes_enabled;
        case GL_POLYGON_OFFSET_FACTOR:
            // params returns one value, the scaling factor used to determine the variable offset that is added to the
            // depth value of each fragment generated when a polygon is rasterized. See glPolygonOffset.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->rasterization_state.polygon_offset_factor;
        case GL_POLYGON_OFFSET_FILL:
            // params returns a single boolean value indicating whether polygon offset is enabled for polygons in fill
            // mode. The initial value is GL_FALSE. See glPolygonOffset.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->rasterization_state.polygon_offset_fill_enabled;
        case GL_POLYGON_OFFSET_UNITS:
            // params returns one value. This value is multiplied by an implementation-specific value and then added to
            // the depth value of each fragment generated when a polygon is rasterized. See glPolygonOffset.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->rasterization_state.polygon_offset_units;
        case GL_PROJECTION_MATRIX:
            // params returns sixteen values: the projection matrix on the top of the projection matrix stack. See
            // glPushMatrix.
            *element_type = GLI_FLOAT;
            *element_count = 16;
            return context->transformation_state
                .projection_matrix_stack[context->transformation_state.projection_matrix_stack_depth - 1];
        case GL_PROJECTION_STACK_DEPTH:
            // params returns one value, the number of matrices on the projection matrix stack. See glPushMatrix.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->transformation_state.projection_matrix_stack_depth;
        case GL_RED_BITS:
            // params returns one value, the number of red bitplanes in each color buffer.
            *element_type = GLI_INT;
            *element_count = 1;
            //// FIXME. 8 for RGBA8888 etc
            assert(0);
            return NULL;
        case GL_RESCALE_NORMAL:
            // params returns a single boolean value indicating whether rescaling of normals is enabled. The initial
            // value is GL_FALSE. See glNormal.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->transformation_state.rescale_normal_enabled;
        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            // params returns a single boolean value indicating if the fragment coverage value should be ANDed with a
            // temporary coverage value based on the fragment's alpha value. The initial value is GL_FALSE. See
            // glSampleCoverage.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->multisampling_state.sample_alpha_to_coverage_enabled;
        case GL_SAMPLE_ALPHA_TO_ONE:
            // params returns a single boolean value indicating if the fragment's alpha value should be replaced by the
            // maximum representable alpha value after coverage determination. The initial value is GL_FALSE. See
            // glSampleCoverage.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->multisampling_state.sample_alpha_to_one_enabled;
        case GL_SAMPLE_BUFFERS:
            // params returns a single integer value indicating the number of sample buffers associated with the
            // currently bound framebuffer. See glSampleCoverage.
            // 0 — no multisample buffer (no MSAA).
            // 1 — one multisample buffer (the common case when MSAA is enabled).
            *element_type = GLI_INT;
            *element_count = 1;
            assert(0);
            return NULL; // FIXME
        case GL_SAMPLE_COVERAGE:
            // params returns a single boolean value indicating if the fragment coverage value should be ANDed with a
            // temporary coverage value based on the current sample coverage value. The initial value is GL_FALSE. See
            // glSampleCoverage.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->multisampling_state.sample_coverage_enabled;
        case GL_SAMPLE_COVERAGE_INVERT:
            // params returns a single boolean value indicating if the temporary coverage value should be inverted. See
            // glSampleCoverage.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->multisampling_state.sample_coverage_invert;
        case GL_SAMPLE_COVERAGE_VALUE:
            // params returns a single positive floating-point value indicating the current sample coverage value. See
            // glSampleCoverage.
            *element_type = GLI_FLOAT;
            *element_count = 1;
            return &context->multisampling_state.sample_coverage_value;
        case GL_SAMPLES:
            // params returns a single integer value indicating the coverage mask size of the currently bound
            // framebuffer. See glSampleCoverage.
            *element_type = GLI_INT;
            *element_count = 1;
            assert(0);
            return NULL; // FIXME what is this value?
            break;
        case GL_SCISSOR_BOX:
            // params returns four values: the x and y window coordinates of the scissor box, followed by its width and
            // height. See glScissor.
            *element_type = GLI_INT;
            *element_count = 4;
            return context->pixel_ops_state.scissor_box;
        case GL_SCISSOR_TEST:
            // params returns a single boolean value indicating whether scissoring is enabled. The initial value is
            // GL_FALSE. See glScissor.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->pixel_ops_state.scissor_test_enabled;
        case GL_SHADE_MODEL:
            // params returns one value, a symbolic constant indicating whether the shading mode is flat or smooth. See
            // glShadeModel.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->coloring_state.shade_model;
        case GL_SMOOTH_LINE_WIDTH_RANGE:
            // params returns two values, the smallest and largest supported widths for antialiased lines. The range
            // must include 1. See glLineWidth.
            *element_type = GLI_FLOAT;
            *element_count = 2;
            return context->implementation_limits.antialiased_line_width_range;
        case GL_SMOOTH_POINT_SIZE_RANGE:
            // params returns two values, the smallest and largest supported widths for antialiased points. The range
            // must include 1. See glPointSize.
            *element_type = GLI_FLOAT;
            *element_count = 2;
            return context->implementation_limits.antialiased_point_size_range;
        case GL_STENCIL_BITS:
            // params returns one value, the number of bitplanes in the stencil buffer.
            *element_type = GLI_INT;
            *element_count = 1;
            //// FIXME. 8 for D24S8 etc
            return NULL;
        case GL_STENCIL_CLEAR_VALUE:
            // params returns one value, the index to which the stencil bitplanes are cleared. See glClearStencil.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->framebuffer_control.clear_stencil;
            break;
        case GL_STENCIL_FAIL:
            // params returns one value, a symbolic constant indicating what action is taken when the stencil test
            // fails. See glStencilOp.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.stencil_fail_op;
        case GL_STENCIL_FUNC:
            // params returns one value, a symbolic constant indicating what function is used to compare the stencil
            // reference value with the stencil buffer value. See glStencilFunc.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.stencil_func;
        case GL_STENCIL_PASS_DEPTH_FAIL:
            // params returns one value, a symbolic constant indicating what action is taken when the stencil test
            // passes, but the depth test fails. See glStencilOp.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.stencil_zfail_op;
        case GL_STENCIL_PASS_DEPTH_PASS:
            // params returns one value, a symbolic constant indicating what action is taken when the stencil test
            // passes, and the depth test passes. See glStencilOp.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.stencil_zpass_op;
        case GL_STENCIL_REF:
            // params returns one value, the reference value that is compared with the contents of the stencil buffer.
            // See glStencilFunc.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.stencil_ref;
        case GL_STENCIL_TEST:
            // params returns a single boolean value indicating whether stencil testing of fragments is enabled. The
            // initial value is GL_FALSE. See glStencilFunc and glStencilOp.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->pixel_ops_state.stencil_test_enabled;
        case GL_STENCIL_VALUE_MASK:
            // params returns one value, the mask that is used to mask both the stencil reference value and the stencil
            // buffer value before they are compared. See glStencilFunc.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_ops_state.stencil_value_mask;
        case GL_STENCIL_WRITEMASK:
            // params returns one value, the mask that controls writing of the stencil bitplanes. See glStencilMask.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->framebuffer_control.stencil_writemask;
        case GL_SUBPIXEL_BITS:
            // params returns one value, an estimate of the number of bits of subpixel resolution that are used to
            // position rasterized geometry in window coordinates. The value must be at least 4.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->implementation_limits.subpixel_bits;
            break;
        case GL_TEXTURE_2D:
            // params returns a single boolean value indicating whether 2D texturing is enabled. The initial value is
            // GL_FALSE. See glTexImage2D.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->texture_environment.texture_units[tu].texture_2d_enabled;
        case GL_TEXTURE_BINDING_2D:
            // params returns one value, the name of the texture currently bound to the target GL_TEXTURE_2D. See
            // glBindTexture.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->texture_environment.texture_units[tu].texture_binding_2d;
        case GL_TEXTURE_COORD_ARRAY:
            // params returns a single boolean value indicating whether the texture coordinate array is enabled. The
            // initial value is GL_FALSE. See glTexCoordPointer.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->vertex_array_data.texcoord_array_enabled;
        case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
            // params returns one value, the texture coordinate array buffer binding. See glTexCoordPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.texcoord_array_buffer_binding;
        case GL_TEXTURE_COORD_ARRAY_SIZE:
            // params returns one value, the number of coordinates per element in the texture coordinate array. See
            // glTexCoordPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.texcoord_array_size;
        case GL_TEXTURE_COORD_ARRAY_STRIDE:
            // params returns one value, the byte offset between consecutive elements in the texture coordinate array.
            // See glTexCoordPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.texcoord_array_stride;
        case GL_TEXTURE_COORD_ARRAY_TYPE:
            // params returns one value, returns the data type of each coordinate in the texture coordinate array. See
            // glTexCoordPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.texcoord_array_type;
        case GL_TEXTURE_MATRIX:
            // params returns sixteen values: the texture matrix on the top of the texture matrix stack. See
            // glPushMatrix.
            *element_type = GLI_FLOAT;
            *element_count = 16;
            return context->transformation_state
                .texture_matrix_stack[tu][context->transformation_state.texture_matrix_stack_depth[tu] - 1];
        case GL_TEXTURE_STACK_DEPTH:
            // params returns one value, the number of matrices on the texture matrix stack. See glBindTexture.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->transformation_state.texture_matrix_stack_depth[tu];
        case GL_UNPACK_ALIGNMENT:
            // params returns one value, the byte alignment used for reading pixel data from memory. See glPixelStorei.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->pixel_store.unpack_alignment;
            break;
        case GL_VIEWPORT:
            // params returns four values:, the x and y window coordinates of the viewport, followed by its width and
            // height. See glViewport.
            *element_type = GLI_INT;
            *element_count = 4;
            return context->transformation_state.viewport;
        case GL_VERTEX_ARRAY:
            // params returns a single boolean value indicating whether the vertex array is enabled. The initial value
            // is GL_FALSE. See glVertexPointer.
            *element_type = GLI_BOOLEAN;
            *element_count = 1;
            return &context->vertex_array_data.vertex_array_enabled;
        case GL_VERTEX_ARRAY_BUFFER_BINDING:
            // params returns one value, the vertex array buffer binding. See glVertexPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.vertex_array_buffer_binding;
        case GL_VERTEX_ARRAY_SIZE:
            // params returns one value, number of coordinates per vertex in the vertex array. See glVertexPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.vertex_array_size;
        case GL_VERTEX_ARRAY_STRIDE:
            // params returns one value, the byte offset between consecutive vertexes in the vertex array. See
            // glVertexPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.vertex_array_stride;
        case GL_VERTEX_ARRAY_TYPE:
            // params returns one value, returns the data type of each coordinate in the vertex array. See
            // glVertexPointer.
            *element_type = GLI_INT;
            *element_count = 1;
            return &context->vertex_array_data.vertex_array_type;
        default:
            *element_type = -1;
            *element_count = -1;
            return NULL;
    }
}
