#pragma once

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <cglm/cglm.h>
#include <stb_sprintf.h>
#include <xgu.h>
#include <xgux.h>

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nv2a_helper.h"

#define GLI_MIN(a, b)       (((a)) < ((b)) ? (a) : (b))
#define GLI_MAX(a, b)       (((a)) > ((b)) ? (a) : (b))
#define GLI_CLAMP(x, l, h)  (((x) < (l)) ? (l) : (((x) > (h)) ? (h) : (x)))
#define GLI_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifndef GLI_MAX_LIGHTS
#define GLI_MAX_LIGHTS 8
#endif
#ifndef GLI_MAX_TEXTURE_UNITS
#define GLI_MAX_TEXTURE_UNITS 2
#endif
#ifndef GLI_MAX_MODELVIEW_STACK
#define GLI_MAX_MODELVIEW_STACK 16
#endif
#ifndef GLI_MAX_PROJECTION_STACK
#define GLI_MAX_PROJECTION_STACK 2
#endif
#ifndef GLI_MAX_TEXTURE_STACK
#define GLI_MAX_TEXTURE_STACK 2
#endif
#ifndef GLI_MAX_CLIP_PLANES
#define GLI_MAX_CLIP_PLANES 1
#endif
#ifndef GLI_UNPACK_ALIGNMENT
#define GLI_UNPACK_ALIGNMENT 4
#endif
#ifndef GLI_PACK_ALIGNMENT
#define GLI_PACK_ALIGNMENT 4
#endif
#ifndef GLI_MAX_TEXTURE_SIZE
#define GLI_MAX_TEXTURE_SIZE 64
#endif
#ifndef GLI_SUBPIXEL_BITS
#define GLI_SUBPIXEL_BITS 4
#endif
#ifndef GLI_NUM_COMPRESSED_TEXTURE_FORMATS
#define GLI_NUM_COMPRESSED_TEXTURE_FORMATS 0
#endif
#ifndef GLI_VENDOR_STRING
#define GLI_VENDOR_STRING "UnspecifiedVendor"
#endif
#ifndef GLI_RENDERER_STRING
#define GLI_RENDERER_STRING "UnspecifiedRenderer"
#endif
#ifndef GLI_VERSION_STRING
#define GLI_VERSION_STRING "OpenGL ES 1.1.0"
#endif
#ifndef GLI_EXTENSIONS_STRING
#define GLI_EXTENSIONS_STRING ""
#endif
#ifndef GLI_DEPTH_BUFFER_MAX
#define GLI_DEPTH_BUFFER_MAX ((1UL << 24) - 1)
#endif
#ifndef GLI_MAX_VIEWPORT_WIDTH
#define GLI_MAX_VIEWPORT_WIDTH 4096
#endif
#ifndef GLI_MAX_VIEWPORT_HEIGHT
#define GLI_MAX_VIEWPORT_HEIGHT 4096
#endif
#ifndef GLI_MIN_ALIASED_POINT_SIZE
#define GLI_MIN_ALIASED_POINT_SIZE 1.0f
#endif
#ifndef GLI_MAX_ALIASED_POINT_SIZE
#define GLI_MAX_ALIASED_POINT_SIZE 1.0f
#endif
#ifndef GLI_MIN_SMOOTH_POINT_SIZE
#define GLI_MIN_SMOOTH_POINT_SIZE 1.0f
#endif
#ifndef GLI_MAX_SMOOTH_POINT_SIZE
#define GLI_MAX_SMOOTH_POINT_SIZE 1.0f
#endif
#ifndef GLI_MIN_ALIASED_LINE_WIDTH
#define GLI_MIN_ALIASED_LINE_WIDTH 1.0f
#endif
#ifndef GLI_MAX_ALIASED_LINE_WIDTH
#define GLI_MAX_ALIASED_LINE_WIDTH 1.0f
#endif
#ifndef GLI_MIN_SMOOTH_LINE_WIDTH
#define GLI_MIN_SMOOTH_LINE_WIDTH 1.0f
#endif
#ifndef GLI_MAX_SMOOTH_LINE_WIDTH
#define GLI_MAX_SMOOTH_LINE_WIDTH 1.0f
#endif

#ifndef GLI_MEMORY_INCLUDE
#define GLI_MEMORY_INCLUDE <stdlib.h>
#endif
#include GLI_MEMORY_INCLUDE
#ifndef GLI_MALLOC
#define GLI_MALLOC(size) malloc(size)
#endif
#ifndef GLI_FREE
#define GLI_FREE(ptr) free(ptr)
#endif
#ifndef GLI_DEBUG_PRINT_INCLUDE
#define GLI_DEBUG_PRINT_INCLUDE <stdio.h>
#endif
#include GLI_DEBUG_PRINT_INCLUDE
#ifndef GLI_DEBUG_PRINT
#define GLI_DEBUG_PRINT(...) printf(__VA_ARGS__)
#endif

// Table 6.3 - Current Values and Associated Data
typedef struct
{
    vec4 current_color;
    vec4 current_texcoord[GLI_MAX_TEXTURE_UNITS];
    vec3 current_normal;
} current_values_t;

// Table 6.4 and Table 6.5 - Vertex Array Data
typedef struct
{
    GLenum client_active_texture;

    // Vertex
    GLboolean vertex_array_enabled;
    GLboolean vertex_array_dirty;
    GLuint vertex_array_size;
    GLenum vertex_array_type;
    GLuint vertex_array_stride;
    const GLvoid *vertex_array_ptr;

    GLboolean normal_array_enabled;
    GLboolean normal_array_dirty;
    GLenum normal_array_type;
    GLuint normal_array_stride;
    const GLvoid *normal_array_ptr;

    // Color
    GLboolean color_array_enabled;
    GLboolean color_array_dirty;
    GLuint color_array_size;
    GLenum color_array_type;
    GLuint color_array_stride;
    const GLvoid *color_array_ptr;

    // Texture coordinates
    GLboolean texcoord_array_enabled[GLI_MAX_TEXTURE_UNITS];
    GLboolean texcoord_array_dirty[GLI_MAX_TEXTURE_UNITS];
    GLuint texcoord_array_size[GLI_MAX_TEXTURE_UNITS];
    GLenum texcoord_array_type[GLI_MAX_TEXTURE_UNITS];
    GLuint texcoord_array_stride[GLI_MAX_TEXTURE_UNITS];
    const GLvoid *texcoord_array_ptr[GLI_MAX_TEXTURE_UNITS];

    // Point size array (Extension)
    GLboolean point_size_array_enabled;
    GLboolean point_size_array_dirty;
    GLenum point_size_array_type;
    GLuint point_size_array_stride;
    const GLvoid *point_size_array_ptr;

    // Bindings
    GLuint array_buffer_binding;
    GLuint element_array_buffer_binding;
    GLuint vertex_array_buffer_binding;
    GLuint normal_array_buffer_binding;
    GLuint color_array_buffer_binding;
    GLuint texcoord_array_buffer_binding[GLI_MAX_TEXTURE_UNITS];
    GLuint point_size_array_buffer_binding;
} vertex_array_data_t;

// Table 6.6 - Buffer Object State
typedef struct buffer_object
{
    GLuint buffer_name;
    GLuint buffer_size;
    GLenum buffer_usage;
    void *buffer_data;
    struct buffer_object *next;
} buffer_object_t;

// Table 6.7 - Transformation State
typedef struct
{
    mat4 modelview_matrix_stack[GLI_MAX_MODELVIEW_STACK];
    GLboolean modelview_matrix_stack_dirty;
    mat4 projection_matrix_stack[GLI_MAX_PROJECTION_STACK];
    GLboolean projection_matrix_stack_dirty;
    mat4 texture_matrix_stack[GLI_MAX_TEXTURE_UNITS][GLI_MAX_TEXTURE_STACK];
    GLboolean texture_matrix_stack_dirty[GLI_MAX_TEXTURE_UNITS];
    GLint viewport[4];    // X,Y,Width,Height
    mat4 viewport_matrix; // Computed from viewport
    GLboolean viewport_dirty;
    GLfloat depth_range[2];
    GLboolean depth_range_dirty;
    GLuint modelview_matrix_stack_depth;
    GLuint projection_matrix_stack_depth;
    GLuint texture_matrix_stack_depth[GLI_MAX_TEXTURE_UNITS];
    GLenum matrix_mode;
    GLboolean normalize_enabled;
    GLboolean rescale_normal_enabled;
    vec4 clip_plane[GLI_MAX_CLIP_PLANES];
    GLboolean clip_plane_enabled[GLI_MAX_CLIP_PLANES];
    GLboolean clip_plane_dirty;
} transformation_state_t;

// Table 6.8 - Coloring
typedef struct
{
    vec4 fog_color;
    GLboolean fog_color_dirty;
    GLfloat fog_density;
    GLboolean fog_density_dirty;
    GLfloat fog_start;
    GLboolean fog_start_dirty;
    GLfloat fog_end;
    GLboolean fog_end_dirty;
    GLenum fog_mode;
    GLboolean fog_mode_dirty;
    GLboolean fog_enabled;
    GLenum shade_model;
} coloring_state_t;

// Table 6.9 and 6.10 - Lighting
typedef struct
{
    vec4 ambient;
    GLboolean ambient_dirty;
    vec4 diffuse;
    GLboolean diffuse_dirty;
    vec4 specular;
    GLboolean specular_dirty;
    vec4 emission;
    GLboolean emission_dirty;
    GLfloat shininess;
    GLboolean shininess_dirty;
} material_t;

typedef struct
{
    vec4 ambient;
    GLboolean ambient_dirty;
    vec4 diffuse;
    GLboolean diffuse_dirty;
    vec4 specular;
    GLboolean specular_dirty;
    vec4 position;
    GLboolean position_dirty;
    GLfloat constant_attenuation;
    GLboolean constant_attenuation_dirty;
    GLfloat linear_attenuation;
    GLboolean linear_attenuation_dirty;
    GLfloat quadratic_attenuation;
    GLboolean quadratic_attenuation_dirty;
    vec3 spot_direction;
    GLboolean spot_direction_dirty;
    GLfloat spot_exponent;
    GLboolean spot_exponent_dirty;
    GLfloat spot_cutoff;
    GLboolean spot_cutoff_dirty;
    GLboolean enabled;
    GLboolean enabled_dirty;
} light_t;

typedef struct
{
    GLboolean lighting_enabled;
    GLboolean color_material_enabled;
    material_t material_front;
    material_t material_back;

    vec4 light_model_ambient;
    GLboolean light_model_ambient_dirty;
    GLboolean light_model_two_side;
    GLboolean light_model_two_side_dirty;

    light_t lights[GLI_MAX_LIGHTS];
} lighting_state_t;

// Table 6.11 - Rasterization State
typedef struct
{
    GLboolean point_params_dirty;
    GLfloat point_size;
    GLboolean point_smooth_enabled;
    GLfloat point_size_min;
    GLfloat point_size_max;
    GLfloat point_fade_threshold_size;
    GLfloat point_distance_attenuation[3];
    GLboolean point_sprites_enabled;

    GLfloat line_width;
    GLboolean line_smooth_enabled;

    GLboolean cull_face_enabled;
    GLenum cull_face_mode;
    GLenum cull_front_face;

    GLfloat polygon_offset_factor;
    GLfloat polygon_offset_units;
    GLboolean polygon_offset_fill_enabled;
} rasterization_state_t;

// Table 6.12 - Multisampling State
typedef struct
{
    GLboolean multisample_enabled;
    GLboolean sample_alpha_to_coverage_enabled;
    GLboolean sample_alpha_to_one_enabled;
    GLboolean sample_coverage_enabled;
    GLfloat sample_coverage_value;
    GLboolean sample_coverage_invert;
} multisampling_state_t;

// Table 6.13 and 6.14 and 6.15 - Textures
typedef struct texture_object
{
    GLuint texture_name;
    GLboolean texture_object_dirty;
    const GLvoid *texture_2d; // This is implementation-defined image data
    GLenum min_filter;
    GLenum mag_filter;
    GLenum wrap_s;
    GLenum wrap_t;

    GLboolean generate_mipmap;
    struct texture_object *next;
} texture_object_t;

typedef struct texture_unit
{
    GLboolean texture_unit_dirty;
    GLboolean texture_2d_enabled;
    GLuint texture_binding_2d;
    texture_object_t *bound_texture_object;
    texture_object_t unbound_texture_object; // Used when texture_binding_2d is 0

    GLenum tex_env_mode;
    vec4 tex_env_color;
    GLboolean coord_replace_oes_enabled;

    GLenum combine_rgb_function;
    GLenum combine_alpha_function;
    GLenum combine_rgb_source[3];    // SRC0, SRC1, SRC2
    GLenum combine_alpha_source[3];  // SRC0, SRC1, SRC2
    GLenum combine_rgb_operand[3];   // OPERAND0, OPERAND1, OPERAND2
    GLenum combine_alpha_operand[3]; // OPERAND0, OPERAND1, OPERAND2
    GLfloat rgb_scale;
    GLfloat alpha_scale;
} texture_unit_t;

typedef struct
{
    GLint server_active_texture;
    texture_unit_t texture_units[GLI_MAX_TEXTURE_UNITS];
    texture_object_t *texture_objects;
} texture_environment_t;

// Table 6.16 - Pixel Operations
typedef struct
{
    GLboolean scissor_test_enabled;
    GLint scissor_box[4];

    GLboolean alpha_test_enabled;
    GLenum alpha_test_func;
    GLfloat alpha_test_ref;

    GLboolean stencil_test_enabled;
    GLenum stencil_func;
    GLuint stencil_value_mask;
    GLuint stencil_ref;
    GLenum stencil_fail_op;
    GLenum stencil_zfail_op;
    GLenum stencil_zpass_op;

    GLboolean depth_test_enabled;
    GLenum depth_func;

    GLboolean blend_enabled;
    GLenum blend_src;
    GLenum blend_dst;

    GLboolean dither_enabled;

    GLboolean color_logic_op_enabled;
    GLenum color_logic_op;
} pixel_ops_state_t;

// Table 6.17 - Framebuffer Control
typedef struct
{
    GLboolean color_mask[4];
    GLboolean depth_writemask;
    GLuint stencil_writemask;
    vec4 clear_color;
    GLfloat clear_depth;
    GLuint clear_stencil;
} framebuffer_control_t;

// Table 6.18 - Pixel Store
typedef struct
{
    GLuint unpack_alignment;
    GLuint pack_alignment;
} pixel_store_t;

// Table 6.19 - Hints
typedef struct
{
    GLenum perspective_correction_hint;
    GLenum point_smooth_hint;
    GLenum line_smooth_hint;
    GLenum fog_hint;
    GLenum generate_mipmap_hint;
} hints_state_t;

// Table 6.20 - Implemenation Limits
typedef struct
{
    GLuint max_lights;
    GLuint max_clip_planes;
    GLuint max_modelview_stack_depth;
    GLuint max_projection_stack_depth;
    GLuint max_texture_stack_depth;
    GLuint subpixel_bits;
    GLuint max_texture_size;
    GLuint max_viewport_dims[2];
    GLfloat aliased_point_size_range[2];
    GLfloat antialiased_point_size_range[2];
    GLfloat aliased_line_width_range[2];
    GLfloat antialiased_line_width_range[2];
    GLuint max_texture_units;
    GLuint sample_buffers;
    GLuint samples;
    GLenum *compressed_texture_formats;
    GLint num_compressed_texture_formats;
} implementation_limits_t;

typedef struct
{
    current_values_t current_values;
    vertex_array_data_t vertex_array_data;
    buffer_object_t *buffer_objects;
    transformation_state_t transformation_state;
    coloring_state_t coloring_state;
    lighting_state_t lighting_state;
    rasterization_state_t rasterization_state;
    multisampling_state_t multisampling_state;
    texture_environment_t texture_environment;
    pixel_ops_state_t pixel_ops_state;
    framebuffer_control_t framebuffer_control;
    pixel_store_t pixel_store;
    hints_state_t hints_state;
    implementation_limits_t implementation_limits;
    GLenum last_error;
} gli_context_t;

void gliFlushStateChange(void);
int gliDebugF(const char *fmt, ...);
void gliSetError(GLenum error);
void gliArrayFlush(void);
void gliLightingFlush(void);
void gliTransformFlush(void);
void gliTextureFlush(void);
void gliFogFlush(void);
void gliPointParamsFlush(void);
buffer_object_t *gliGetBufferObject(GLuint name);
gli_context_t *gliGetContext(void);

GLuint _gl_enum_to_byte_size(GLenum type);

static inline GLfloat gliFixedtoFloat(GLfixed x)
{
    // 16.16 fixed -> float
    return (float)x * (1.0f / 65536.0f);
}

static inline GLfixed gliFloattoFixed(GLfloat x)
{
    // float -> 16.16 fixed
    return (GLfixed)(x * 65536.0f);
}

static inline void glimFixedtoFloat(const GLfixed *mfx, GLfloat mf_dest[16])
{
    for (GLint i = 0; i < 16; ++i) {
        mf_dest[i] = gliFixedtoFloat(mfx[i]);
    }
}

static inline mat4 *gliCurrentModelView(void)
{
    gli_context_t *context = gliGetContext();
    return &context->transformation_state
                .modelview_matrix_stack[context->transformation_state.modelview_matrix_stack_depth - 1];
}

/** Convert GLuint in [0,4294967295] to GLfloat in [0.0,1.0] */
#define UINT_TO_FLOAT(U)    ((GLfloat) ((U) * (1.0F / 4294967295.0)))

/** Convert GLfloat in [0.0,1.0] to GLuint in [0,4294967295] */
#define FLOAT_TO_UINT(X)    ((GLuint) ((X) * 4294967295.0))

/** Convert GLint in [-2147483648,2147483647] to GLfloat in [-1.0,1.0] */
#define INT_TO_FLOAT(I)     ((GLfloat) ((2.0F * (I) + 1.0F) * (1.0F/4294967294.0)))

/** Convert GLfloat in [-1.0,1.0] to GLint in [-2147483648,2147483647] */
#define FLOAT_TO_INT(X)     ( (GLint) (2147483647.0 * (X)) )