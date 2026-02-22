#pragma once
#include <GLES/gl.h>
#include <cglm/cglm.h>
#include <stb_sprintf.h>
#include <xgu.h>
#include <xgux.h>
#include <swizzle.h>

#define GLI_MAX_LIGHTS XGU_LIGHT_COUNT
#define GLI_MAX_TEXTURE_UNITS XGU_TEXTURE_COUNT // This is shared with clip planes (4 per stage) and only one point sprite can be enabled
#define GLI_MAX_CLIP_PLANES 4 // One texture unit can handle 4 planes.
#define GLI_MAX_TEXTURE_SIZE 4096
#define GLI_VENDOR_STRING "nxdk GLES Renderer"
#define GLI_RENDERER_STRING "nv2a-based GPU"
#define GLI_EXTENSIONS_STRING "GL_OES_element_index_uint GL_OES_point_size_array"

// For line and point sizes xbox takes a 6.3 fix point, total of 9 bits. Max = 0x1FF / 2^3 = 63.875f
#define GLI_MAX_ALIASED_POINT_SIZE ((float)0x1FF / (float)(1 << 3))
#define GLI_MAX_SMOOTH_POINT_SIZE ((float)0x1FF / (float)(1 << 3))
#define GLI_MAX_ALIASED_LINE_WIDTH ((float)0x1FF / (float)(1 << 3))
#define GLI_MAX_SMOOTH_LINE_WIDTH ((float)0x1FF / (float)(1 << 3))

#define GLI_DEBUG_PRINT_INCLUDE <xboxkrnl/xboxkrnl.h>
#define GLI_DEBUG_PRINT(...) DbgPrint(__VA_ARGS__)

#define PB_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

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
    struct xgu_texture *mipmap_head;
} xgu_texture_t;

void combiner_init(void);
void combiner_set_texture_env(void);
uint32_t *combiner_specular_fog_config(uint32_t *p, GLboolean fog_enabled, GLboolean specular_enabled);

XguVertexArrayType gliEnumToNvType(GLenum type);
DWORD gliEnumToNvPrimitive(GLenum mode);
DWORD gliEnumToNvLogicOp(GLenum opcode);
XguFuncType gliEnumToNvFunc(GLenum func);
XguBlendFactor gliEnumToNvBlendFactor(GLenum factor);
XguCullFace gliEnumToNvCullFace(GLenum mode);
XguFrontFace gliEnumToNvFrontFace(GLenum mode);
XguStencilOp gliEnumToNvStencilOp(GLenum op);
XguShadeModel gliEnumToNvShadeModel(GLenum code);
XguFogMode gliEnumToNvFogMode(GLenum mode);
XguTextureAddress gliEnumToNvAddressMode(GLenum wrap);
XguTexFilter gliEnumToNvTexFilter(GLenum filter);
XguTexFormatColor gliEnumToNvTexFormat(GLenum format, GLenum type, GLuint *bytes_per_pixel, GLboolean swizzled);

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
