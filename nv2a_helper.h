#pragma once
#include <GLES/gl.h>
#include <cglm/cglm.h>
#include <stb_sprintf.h>
#include <xgu.h>
#include <xgux.h>
#include <swizzle.h>

#define GLI_MAX_LIGHTS XGU_LIGHT_COUNT
#define GLI_MAX_TEXTURE_UNITS (XGU_TEXTURE_COUNT - 1) // We reserve one texture unit for user clip planes
#define GLI_MAX_CLIP_PLANES 4 // One texture unit can handle 4 planes.
#define GLI_MAX_TEXTURE_SIZE 4096
#define GLI_VENDOR_STRING "nxdk GLES Renderer"
#define GLI_RENDERER_STRING "nv2a-based GPU"
#define GLI_EXTENSIONS_STRING "GL_OES_element_index_uint GL_OES_point_size_array"

// For line and point sizes xbox takes a 6.3 fix point, total of 9 bits. Max = 0x1FF / 2^3 = 63.875f
#define GLI_MAX_ALIASED_POINT_SIZE 63.875f
#define GLI_MAX_SMOOTH_POINT_SIZE 63.875f
#define GLI_MAX_ALIASED_LINE_WIDTH 63.875f
#define GLI_MAX_SMOOTH_LINE_WIDTH 63.875f

#define GLI_DEBUG_PRINT_INCLUDE <xboxkrnl/xboxkrnl.h>
#define GLI_DEBUG_PRINT(...) DbgPrint(__VA_ARGS__)

void combiner_init(void);
void combiner_set_texture_env(void);
uint32_t *combiner_specular_fog_config(uint32_t *p, GLboolean fog_enabled, GLboolean specular_enabled);

XguVertexArrayType _gl_enum_to_xgu_type(GLenum type);
XguPrimitiveType _gl_enum_to_xgu_primitive(GLenum mode);
DWORD _gl_enum_to_xgu_logic_op(GLenum opcode);
XguFuncType _gl_enum_to_xgu_func(GLenum func);
XguBlendFactor _gl_enum_to_xgu_blend_factor(GLenum factor);
XguCullFace _gl_enum_to_xgu_cull_face(GLenum mode);
XguFrontFace _gl_enum_to_xgu_front_face(GLenum mode);
XguStencilOp _gl_enum_to_xgu_stencilop(GLenum op);
XguShadeModel _gl_enum_to_xgu_shade_model(GLenum code);
XguFogMode _gl_enum_to_xgu_fog_mode(GLenum mode);
XguTextureAddress _gl_wrap_to_xgu_address_mode(GLenum wrap);
XguTexFilter _gl_filter_to_xgu_tex_filter(GLenum filter);
XguTexFormatColor _gl_enum_to_xgu_tex_format(GLenum format, GLenum type, GLuint *bytes_per_pixel, GLboolean swizzled);

typedef struct xgu_texture
{
    GLint data_width;
    GLint data_height;
    GLint tex_width;
    GLint tex_height;
    GLint bytes_per_pixel;
    GLint pitch;
    GLint swizzled;
    GLfloat u_scale;
    GLfloat v_scale;
    XguTexFormatColor format;
    XguTexFilter filter;
    XguTextureAddress mode_u;
    XguTextureAddress mode_v;
    GLubyte *data;
    GLubyte *data_physical_address;
} xgu_texture_t;

#define PHYSICAL_MEMORY(x) ((void *)((uintptr_t)x & 0x03FFFFFF))

#define FLOAT4_TO_PACKED_ARGB32(f) \
    ((((GLubyte)((f)[3] * 255.0f) & 0xFF) << 24) | \
     (((GLubyte)((f)[0] * 255.0f) & 0xFF) << 16) | \
     (((GLubyte)((f)[1] * 255.0f) & 0xFF) << 8) |  \
     (((GLubyte)((f)[2] * 255.0f) & 0xFF) << 0))

#define FLOAT4_TO_PACKED_ABGR32(f) \
    ((((GLubyte)((f)[3] * 255.0f) & 0xFF) << 24) | \
     (((GLubyte)((f)[2] * 255.0f) & 0xFF) << 16) | \
     (((GLubyte)((f)[1] * 255.0f) & 0xFF) << 8) |  \
     (((GLubyte)((f)[0] * 255.0f) & 0xFF) << 0))

static inline uint32_t npot2pot(uint32_t num)
{
    uint32_t msb;
    __asm__("bsr %1, %0" : "=r"(msb) : "r"(num));

    if ((1 << msb) == num) {
        return num;
    }

    return 1 << (msb + 1);
}
