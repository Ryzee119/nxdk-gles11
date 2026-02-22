#include "gles_private.h"

void glFlipNV2A()
{
    gli_context_t *context = gliGetContext();

    pb_wait_for_vbl();

    while (pb_busy()) {
        NtYieldExecution();
    }

    while (pb_finished()) {
        NtYieldExecution();
    }

    // Reset bits that pb_finished changes
    uint32_t *pb = pb_begin();
    pb = pb_push1(pb,
                  NV097_SET_CONTROL0,
                  NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE | NV097_SET_CONTROL0_TEXTURE_PERSPECTIVE_ENABLE);
    pb = pb_push1(pb, NV097_SET_DEPTH_TEST_ENABLE, context->pixel_ops_state.depth_test_enabled);
    pb = pb_push1(pb, NV097_SET_STENCIL_TEST_ENABLE, context->pixel_ops_state.stencil_test_enabled);
    pb_end(pb);

    pb_reset();
}

XguVertexArrayType gliEnumToNvType(GLenum type)
{
    switch (type) {
        case GL_UNSIGNED_BYTE:
            return XGU_UNSIGNED_BYTE_OGL;
        case GL_SHORT:
            return XGU_SHORT;
        case GL_FLOAT:
            return XGU_FLOAT;
        case GL_FIXED:
            assert(0 && "No fixed point support in NV2A");
            return -1; // FIXME
        default:
            return -1; // Fallback
    }
}

DWORD gliEnumToNvPrimitive(GLenum mode)
{
    switch (mode) {
        case GL_POINTS:
            return NV097_SET_BEGIN_END_OP_POINTS;
        case GL_LINES:
            return NV097_SET_BEGIN_END_OP_LINES;
        case GL_LINE_STRIP:
            return NV097_SET_BEGIN_END_OP_LINE_STRIP;
        case GL_LINE_LOOP:
            return NV097_SET_BEGIN_END_OP_LINE_LOOP;
        case GL_TRIANGLES:
            return NV097_SET_BEGIN_END_OP_TRIANGLES;
        case GL_TRIANGLE_STRIP:
            return NV097_SET_BEGIN_END_OP_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN:
            return NV097_SET_BEGIN_END_OP_TRIANGLE_FAN;
        default:
            return -1;
    }
}

// FIXME. No idea if these are correct.
DWORD gliEnumToNvLogicOp(GLenum opcode)
{
    switch (opcode) {
        case GL_CLEAR:
            return 0x00;
        case GL_AND:
            return 0x01;
        case GL_AND_REVERSE:
            return 0x02;
        case GL_COPY:
            return 0x03;
        case GL_AND_INVERTED:
            return 0x04;
        case GL_NOOP:
            return 0x05;
        case GL_XOR:
            return 0x06;
        case GL_OR:
            return 0x07;
        case GL_NOR:
            return 0x08;
        case GL_EQUIV:
            return 0x09;
        case GL_INVERT:
            return 0x0a;
        case GL_OR_REVERSE:
            return 0x0b;
        case GL_COPY_INVERTED:
            return 0x0c;
        case GL_OR_INVERTED:
            return 0x0d;
        case GL_NAND:
            return 0x0e;
        case GL_SET:
            return 0x0f;
        default:
            return -1;
    }
}

XguFuncType gliEnumToNvFunc(GLenum func)
{
    switch (func) {
        case GL_NEVER:
            return XGU_FUNC_NEVER;
        case GL_LESS:
            return XGU_FUNC_LESS;
        case GL_EQUAL:
            return XGU_FUNC_EQUAL;
        case GL_LEQUAL:
            return XGU_FUNC_LESS_OR_EQUAL;
        case GL_GREATER:
            return XGU_FUNC_GREATER;
        case GL_NOTEQUAL:
            return XGU_FUNC_NOT_EQUAL;
        case GL_GEQUAL:
            return XGU_FUNC_GREATER_OR_EQUAL;
        case GL_ALWAYS:
            return XGU_FUNC_ALWAYS;
        default:
            return -1;
    }
}

XguBlendFactor gliEnumToNvBlendFactor(GLenum factor)
{
    switch (factor) {
        case GL_ZERO:
            return XGU_FACTOR_ZERO;
        case GL_ONE:
            return XGU_FACTOR_ONE;
        case GL_SRC_COLOR:
            return XGU_FACTOR_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR:
            return XGU_FACTOR_ONE_MINUS_SRC_COLOR;
        case GL_SRC_ALPHA:
            return XGU_FACTOR_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return XGU_FACTOR_ONE_MINUS_SRC_ALPHA;
        case GL_DST_ALPHA:
            return XGU_FACTOR_DST_ALPHA;
        case GL_ONE_MINUS_DST_ALPHA:
            return XGU_FACTOR_ONE_MINUS_DST_ALPHA;
        case GL_DST_COLOR:
            return XGU_FACTOR_DST_COLOR;
        case GL_ONE_MINUS_DST_COLOR:
            return XGU_FACTOR_ONE_MINUS_DST_COLOR;
        case GL_SRC_ALPHA_SATURATE:
            return XGU_FACTOR_SRC_ALPHA_SATURATE;
        default:
            return -1;
    }
}

XguCullFace gliEnumToNvCullFace(GLenum mode)
{
    switch (mode) {
        case GL_FRONT:
            return XGU_CULL_FRONT;
        case GL_BACK:
            return XGU_CULL_BACK;
        case GL_FRONT_AND_BACK:
            return XGU_CULL_FRONT_AND_BACK;
        default:
            return -1;
    }
}

XguFrontFace gliEnumToNvFrontFace(GLenum mode)
{
    switch (mode) {
        case GL_CCW:
            return XGU_FRONT_CCW;
        case GL_CW:
            return XGU_FRONT_CW;
        default:
            return -1;
    }
}

XguStencilOp gliEnumToNvStencilOp(GLenum op)
{
    switch (op) {
        case GL_KEEP:
            return XGU_STENCIL_OP_KEEP;
        case GL_ZERO:
            return XGU_STENCIL_OP_ZERO;
        case GL_REPLACE:
            return XGU_STENCIL_OP_REPLACE;
        case GL_INCR:
            return XGU_STENCIL_OP_INCRSAT;
        case GL_DECR:
            return XGU_STENCIL_OP_DECRSAT;
        case GL_INVERT:
            return XGU_STENCIL_OP_INVERT;
        default:
            return -1;
    }
}

XguShadeModel gliEnumToNvShadeModel(GLenum code)
{
    switch (code) {
        case GL_FLAT:
            return XGU_SHADE_MODEL_FLAT;
        case GL_SMOOTH:
            return XGU_SHADE_MODEL_SMOOTH;
        default:
            return -1;
    }
}

XguFogMode gliEnumToNvFogMode(GLenum mode)
{
    switch (mode) {
        case GL_LINEAR:
            return XGU_FOG_MODE_LINEAR;
        case GL_EXP:
            return XGU_FOG_MODE_EXP;
        case GL_EXP2:
            return XGU_FOG_MODE_EXP2;
        default:
            return -1;
    }
}

XguTextureAddress gliEnumToNvAddressMode(GLenum wrap)
{
    switch (wrap) {
        case GL_REPEAT:
            return XGU_WRAP;
        case GL_CLAMP_TO_EDGE:
            return XGU_CLAMP_TO_EDGE;
        default:
            return (XguTextureAddress)-1;
    }
}

XguTexFilter gliEnumToNvTexFilter(GLenum filter)
{
    switch (filter) {
        case GL_NEAREST:
            return NV_PGRAPH_TEXFILTER0_MIN_BOX_LOD0;
        case GL_LINEAR:
            return NV_PGRAPH_TEXFILTER0_MIN_TENT_LOD0;
        case GL_NEAREST_MIPMAP_NEAREST:
            return NV_PGRAPH_TEXFILTER0_MIN_BOX_NEARESTLOD;
        case GL_LINEAR_MIPMAP_NEAREST:
            return NV_PGRAPH_TEXFILTER0_MIN_TENT_NEARESTLOD;
        case GL_NEAREST_MIPMAP_LINEAR:
            return NV_PGRAPH_TEXFILTER0_MIN_BOX_TENT_LOD;
        case GL_LINEAR_MIPMAP_LINEAR:
            return NV_PGRAPH_TEXFILTER0_MIN_TENT_TENT_LOD;
        default:
            return (XguTexFilter)-1;
    }
}

XguTexFormatColor gliEnumToNvTexFormat(GLenum format, GLenum type, GLuint *bytes_per_pixel, GLboolean swizzled)
{
    if (type == GL_UNSIGNED_BYTE) {
        switch (format) {
            case GL_ALPHA:
                *bytes_per_pixel = 1;
                return (swizzled) ? XGU_TEXTURE_FORMAT_A8_SWIZZLED : XGU_TEXTURE_FORMAT_A8;
            case GL_LUMINANCE:
                *bytes_per_pixel = 1;
                return (swizzled) ? XGU_TEXTURE_FORMAT_Y8_SWIZZLED : XGU_TEXTURE_FORMAT_Y8;
            case GL_LUMINANCE_ALPHA:
                *bytes_per_pixel = 2;
                return (swizzled) ? XGU_TEXTURE_FORMAT_A8Y8_SWIZZLED : XGU_TEXTURE_FORMAT_A8Y8;
            case GL_RGB:
            case GL_RGBA:
                *bytes_per_pixel = 4;
                return (swizzled) ? XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED : XGU_TEXTURE_FORMAT_A8B8G8R8;
            default:
                break;
        }
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        *bytes_per_pixel = 2;
        return (swizzled) ? XGU_TEXTURE_FORMAT_R5G6B5_SWIZZLED : XGU_TEXTURE_FORMAT_R5G6B5;
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        *bytes_per_pixel = 2;
        return (swizzled) ? XGU_TEXTURE_FORMAT_A4R4G4B4_SWIZZLED : XGU_TEXTURE_FORMAT_A4R4G4B4;
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        *bytes_per_pixel = 2;
        return (swizzled) ? XGU_TEXTURE_FORMAT_A1R5G5B5_SWIZZLED : XGU_TEXTURE_FORMAT_A1R5G5B5;
    }

    return (XguTexFormatColor)-1;
}

#define REG_ZERO      0x0
#define REG_CONSTANT0 0x1
#define REG_CONSTANT1 0x2
#define REG_FOG       0x3
#define REG_COLOR0    0x4
#define REG_COLOR1    0x5
#define REG_TEXTURE0  0x8 // + n for texture units 0-3
#define REG_SPARE0    0xC
#define REG_SPECULAR  0x0E

// Although nv2a reg name says ALPHA they are the sames values for both alpha and color combiners
#define MAP_UNSIGNED_IDENTITY NV097_SET_COMBINER_ALPHA_ICW_A_MAP_UNSIGNED_IDENTITY
#define MAP_UNSIGNED_INVERT   NV097_SET_COMBINER_ALPHA_ICW_A_MAP_UNSIGNED_INVERT
#define MAP_EXPAND_NORMAL     NV097_SET_COMBINER_ALPHA_ICW_A_MAP_EXPAND_NORMAL
#define MAP_EXPAND_NEGATE     NV097_SET_COMBINER_ALPHA_ICW_A_MAP_EXPAND_NEGATE
#define MAP_HALFBIAS_NORMAL   NV097_SET_COMBINER_ALPHA_ICW_A_MAP_HALFBIAS_NORMAL
#define MAP_HALFBIAS_NEGATE   NV097_SET_COMBINER_ALPHA_ICW_A_MAP_HALFBIAS_NEGATE
#define MAP_SIGNED_IDENTITY   NV097_SET_COMBINER_ALPHA_ICW_A_MAP_SIGNED_IDENTITY
#define MAP_SIGNED_NEGATE     NV097_SET_COMBINER_ALPHA_ICW_A_MAP_SIGNED_NEGATE

// Although nv2a reg name says COLOR they are the sames values for both alpha and color combiners
#define SRC_A NV097_SET_COMBINER_COLOR_ICW_A_SOURCE
#define SRC_B NV097_SET_COMBINER_COLOR_ICW_B_SOURCE
#define SRC_C NV097_SET_COMBINER_COLOR_ICW_C_SOURCE
#define SRC_D NV097_SET_COMBINER_COLOR_ICW_D_SOURCE

#define SRC_A_FLAG_ALPHA PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_ALPHA, 0x01)
#define SRC_B_FLAG_ALPHA PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_ALPHA, 0x01)
#define SRC_C_FLAG_ALPHA PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_ALPHA, 0x01)
#define SRC_D_FLAG_ALPHA PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_ALPHA, 0x01)

#define SRC_A_FLAG_RGB PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_ALPHA, 0x00)
#define SRC_B_FLAG_RGB PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_ALPHA, 0x00)
#define SRC_C_FLAG_RGB PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_ALPHA, 0x00)
#define SRC_D_FLAG_RGB PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_ALPHA, 0x00)

// Invert means (1 - value)
#define SRC_A_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_MAP, MAP_UNSIGNED_INVERT)
#define SRC_B_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_MAP, MAP_UNSIGNED_INVERT)
#define SRC_C_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_MAP, MAP_UNSIGNED_INVERT)
#define SRC_D_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_MAP, MAP_UNSIGNED_INVERT)

// Half-bias means (value - 0.5)
#define SRC_A_FLAG_HALF_BIAS PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_MAP, MAP_HALFBIAS_NORMAL)
#define SRC_B_FLAG_HALF_BIAS PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_MAP, MAP_HALFBIAS_NORMAL)
#define SRC_C_FLAG_HALF_BIAS PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_MAP, MAP_HALFBIAS_NORMAL)
#define SRC_D_FLAG_HALF_BIAS PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_MAP, MAP_HALFBIAS_NORMAL)

// Negate half-bias means (0.5 - value)
#define SRC_A_FLAG_HALF_BIAS_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_MAP, MAP_HALFBIAS_NEGATE)
#define SRC_B_FLAG_HALF_BIAS_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_MAP, MAP_HALFBIAS_NEGATE)
#define SRC_C_FLAG_HALF_BIAS_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_MAP, MAP_HALFBIAS_NEGATE)
#define SRC_D_FLAG_HALF_BIAS_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_MAP, MAP_HALFBIAS_NEGATE)

// Expand normal means (value * 2 - 1) (i.e 0 - 1 is mapped to -1 to 1)
#define SRC_A_FLAG_EXPAND PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_MAP, MAP_EXPAND_NORMAL)
#define SRC_B_FLAG_EXPAND PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_MAP, MAP_EXPAND_NORMAL)
#define SRC_C_FLAG_EXPAND PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_MAP, MAP_EXPAND_NORMAL)
#define SRC_D_FLAG_EXPAND PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_MAP, MAP_EXPAND_NORMAL)

// Expand negate means (1 - value) * 2 (i.e. 0 - 1 mapped to 1 to -1)
#define SRC_A_FLAG_EXPAND_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_MAP, MAP_EXPAND_NEGATE)
#define SRC_B_FLAG_EXPAND_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_MAP, MAP_EXPAND_NEGATE)
#define SRC_C_FLAG_EXPAND_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_MAP, MAP_EXPAND_NEGATE)
#define SRC_D_FLAG_EXPAND_NEGATE PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_MAP, MAP_EXPAND_NEGATE)

// Combiner operations
#define OP_AxB          NV097_SET_COMBINER_COLOR_OCW_AB_DST
#define OP_CxD          NV097_SET_COMBINER_COLOR_OCW_CD_DST
#define OP_AxB_PLUS_CxD NV097_SET_COMBINER_COLOR_OCW_SUM_DST

// CW1
#define SRC_E NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_SOURCE
#define SRC_F NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_SOURCE
#define SRC_G NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_SOURCE

#define SRC_E_FLAG_ALPHA PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_ALPHA, 0x01)
#define SRC_F_FLAG_ALPHA PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_ALPHA, 0x01)
#define SRC_G_FLAG_ALPHA PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_ALPHA, 0x01)

void combiner_init(void)
{
    uint32_t *pb = pb_begin();
    pb = pb_push1(
        pb,
        NV097_SET_SHADER_OTHER_STAGE_INPUT,
        PB_MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE1, NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE1_INSTAGE_0) |
            PB_MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE2, NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE2_INSTAGE_0) |
            PB_MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE3, NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE3_INSTAGE_0));

    pb = pb_push1(
        pb,
        NV097_SET_COMBINER_CONTROL,
        PB_MASK(NV097_SET_COMBINER_CONTROL_FACTOR0, NV097_SET_COMBINER_CONTROL_FACTOR0_EACH_STAGE) |
            PB_MASK(NV097_SET_COMBINER_CONTROL_FACTOR1, NV097_SET_COMBINER_CONTROL_FACTOR1_EACH_STAGE) |
            PB_MASK(NV097_SET_COMBINER_CONTROL_ITERATION_COUNT, NV097_SET_COMBINER_CONTROL_ITERATION_COUNT_FOUR));

    pb_end(pb);
}

static DWORD get_source(GLenum src, DWORD stage_input, GLint unit_index)
{
    switch (src) {
        case GL_TEXTURE:
            return REG_TEXTURE0 + unit_index;
        case GL_CONSTANT:
            return REG_CONSTANT0;
        case GL_PRIMARY_COLOR:
            return REG_COLOR0;
        case GL_PREVIOUS:
        default:
            return stage_input;
    }
}

static DWORD get_op(GLenum operand, char source, GLboolean is_alpha_combiner)
{
    DWORD flag_invert = 0;
    DWORD flag_alpha = 0;

    switch (source) {
        case 'A':
            flag_invert = SRC_A_FLAG_INVERT;
            flag_alpha = SRC_A_FLAG_ALPHA;
            break;
        case 'B':
            flag_invert = SRC_B_FLAG_INVERT;
            flag_alpha = SRC_B_FLAG_ALPHA;
            break;
        case 'C':
            flag_invert = SRC_C_FLAG_INVERT;
            flag_alpha = SRC_C_FLAG_ALPHA;
            break;
        case 'D':
            flag_invert = SRC_D_FLAG_INVERT;
            flag_alpha = SRC_D_FLAG_ALPHA;
            break;
        default:
            return 0;
    }

    switch (operand) {
        case GL_SRC_COLOR:
            if (is_alpha_combiner) {
                return flag_alpha;
            }
            return 0;

        case GL_ONE_MINUS_SRC_COLOR:
            if (is_alpha_combiner) {
                return flag_alpha | flag_invert;
            }
            return flag_invert;

        case GL_SRC_ALPHA:
            return flag_alpha;

        case GL_ONE_MINUS_SRC_ALPHA:
            return flag_alpha | flag_invert;

        default:
            return 0;
    }
}

static GLenum invert_operand(GLenum op)
{
    switch (op) {
        case GL_SRC_COLOR:
            return GL_ONE_MINUS_SRC_COLOR;
        case GL_ONE_MINUS_SRC_COLOR:
            return GL_SRC_COLOR;
        case GL_SRC_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA:
            return GL_SRC_ALPHA;
        default:
            return op;
    }
}

// Stage 0,1,2,3 all have the same value, just wrap this for readability
#define NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PROGRAM_NONE  NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PROGRAM_NONE
#define NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PASS_THROUGH  NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PASS_THROUGH
#define NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_CLIP_PLANE    NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_CLIP_PLANE
#define NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_2D_PROJECTIVE NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_2D_PROJECTIVE

void combiner_set_texture_env(void)
{
    gli_context_t *context = gliGetContext();
    uint32_t *pb;

    DWORD shader_program[4] = {
        NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PROGRAM_NONE,
        NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PROGRAM_NONE,
        NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PROGRAM_NONE,
        NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PROGRAM_NONE,
    };

    // Update combiner stages
    uint32_t stage = 0;
    pb = pb_begin();
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];

        if (!texture_unit->texture_2d_enabled ||
            texture_unit->bound_texture_object == &texture_unit->unbound_texture_object) {
            continue;
        }

        const uint32_t s = i;
        shader_program[s] = NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_2D_PROJECTIVE;

        if (!texture_unit->texture_unit_dirty) {
            continue;
        }

        // Point sprites must be on stage 3. Let's just warn about it for now.
        const GLboolean is_point_sprite =
            texture_unit->coord_replace_oes_enabled && context->rasterization_state.point_sprite_oes_enabled;
        if (is_point_sprite && i != 3) {
            GLI_DEBUG_PRINT("Point sprite texture unit %u must be on texture unit 3\n", i);
        }

        DWORD STAGE_INPUT = (s == 0) ? REG_COLOR0 : REG_SPARE0;
        DWORD RGB_IN = 0;
        DWORD RGB_OUT = 0;
        DWORD ALPHA_IN = 0;
        DWORD ALPHA_OUT = 0;

        switch (texture_unit->tex_env_mode) {
            case GL_ADD:
                // ---------------------------------------------------------
                // ADD:
                //   C = clamp(Cf + Ct)
                //   A = Af
                //
                // Implemented as:
                //   AB = Cf * 1
                //   CD = Ct * 1
                //   SUM = AB + CD -> SPARE0
                // ---------------------------------------------------------

                // COLOR AB: A = STAGE_INPUT.rgb, B = 1.0
                RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
                RGB_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_INVERT;

                // COLOR CD: C = texture.rgb, D = 1.0
                RGB_IN |= PB_MASK(SRC_C, REG_TEXTURE0 + i);
                RGB_IN |= PB_MASK(SRC_D, REG_ZERO) | SRC_D_FLAG_INVERT;

                // SUM (AB + CD) -> SPARE0.rgb
                RGB_OUT |= PB_MASK(OP_AxB_PLUS_CxD, REG_SPARE0);

                // ALPHA: pass-through Af
                ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                ALPHA_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

                // SUM (AB + CD) -> SPARE0.a
                ALPHA_OUT |= PB_MASK(OP_AxB, REG_SPARE0);
                break;

            case GL_MODULATE:
                // ---------------------------------------------------------
                // MODULATE:
                //   C = Cf * Ct
                //   A = Af * At
                // ---------------------------------------------------------

                // COLOR: A = STAGE_INPUT.rgb, B = texture.rgb
                RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
                RGB_IN |= PB_MASK(SRC_B, REG_TEXTURE0 + i);

                // ALPHA: A = STAGE_INPUT.a, B = texture.a
                ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                ALPHA_IN |= PB_MASK(SRC_B, REG_TEXTURE0 + i) | SRC_B_FLAG_ALPHA;

                RGB_OUT |= PB_MASK(OP_AxB, REG_SPARE0); // SPARE0 = A*B
                ALPHA_OUT |= PB_MASK(OP_AxB, REG_SPARE0);
                break;

            case GL_REPLACE:
                // ---------------------------------------------------------
                // REPLACE:
                //   C = Ct
                //   A = At   (for RGBA textures; for RGB, A = Af)
                // ---------------------------------------------------------

                // COLOR: out.rgb = texture.rgb
                RGB_IN |= PB_MASK(SRC_A, REG_TEXTURE0 + i);
                RGB_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_INVERT; // B=1, so A*B = tex

                // ALPHA: out.a = texture.a
                ALPHA_IN |= PB_MASK(SRC_A, REG_TEXTURE0 + i) | SRC_A_FLAG_ALPHA;
                ALPHA_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

                RGB_OUT |= PB_MASK(OP_AxB, REG_SPARE0);
                ALPHA_OUT |= PB_MASK(OP_AxB, REG_SPARE0);
                break;

            case GL_DECAL:
                // ---------------------------------------------------------
                // DECAL (for RGBA textures):
                //   C = Cf * (1 - At) + Ct * At
                //   A = Af
                //
                // Using:
                //   AB = Cf * (1 - At)
                //   CD = Ct * At
                //   SUM = AB + CD -> SPARE0
                // ---------------------------------------------------------

                // COLOR AB: A = STAGE_INPUT.rgb, B = texture alpha with invert (1 - At)
                RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
                RGB_IN |= PB_MASK(SRC_B, REG_TEXTURE0 + i) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

                // COLOR CD: C = texture.rgb, D = texture alpha
                RGB_IN |= PB_MASK(SRC_C, REG_TEXTURE0 + i);
                RGB_IN |= PB_MASK(SRC_D, REG_TEXTURE0 + i) | SRC_D_FLAG_ALPHA;

                RGB_OUT |= PB_MASK(OP_AxB_PLUS_CxD, REG_SPARE0); // AB + CD

                // Alpha unchanged: Af
                ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                ALPHA_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;
                ALPHA_OUT |= PB_MASK(OP_AxB, REG_SPARE0);
                break;

            case GL_BLEND:
                // ---------------------------------------------------------
                // BLEND (RGBA textures):
                //   C = Cf * (1 - Ct) + Cc * Ct
                //   A = Af
                //
                // Cc is GL_TEXTURE_ENV_COLOR in REG_CONSTANT0.
                //
                // Implement:
                //   AB = Cf * (1 - Ct)
                //   CD = Cc * Ct
                //   SUM = AB + CD -> SPARE0
                // ---------------------------------------------------------

                // COLOR AB: A = STAGE_INPUT.rgb, B = (1 - Ct.rgb)
                RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
                RGB_IN |= PB_MASK(SRC_B, REG_TEXTURE0 + i) | SRC_B_FLAG_INVERT;

                // COLOR CD: C = CONSTANT0 (env color), D = texture.rgb
                RGB_IN |= PB_MASK(SRC_C, REG_CONSTANT0);
                RGB_IN |= PB_MASK(SRC_D, REG_TEXTURE0 + i);

                RGB_OUT |= PB_MASK(OP_AxB_PLUS_CxD, REG_SPARE0); // AB + CD -> SPARE0

                // Alpha unchanged: Af
                ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                ALPHA_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;
                ALPHA_OUT |= PB_MASK(OP_AxB, REG_SPARE0);
                break;

            case GL_COMBINE: {
                GLenum *sources[2] = {texture_unit->combine_rgb_source, texture_unit->combine_alpha_source};
                GLenum *ops[2] = {texture_unit->combine_rgb_operand, texture_unit->combine_alpha_operand};
                GLenum functions[2] = {texture_unit->combine_rgb_function, texture_unit->combine_alpha_function};
                DWORD *inputs[2] = {&RGB_IN, &ALPHA_IN};
                DWORD *outputs[2] = {&RGB_OUT, &ALPHA_OUT};

                for (GLuint j = 0; j < 2; j++) {
                    const GLboolean is_alpha = (j == 1);

                    const DWORD arg0 = get_source(sources[j][0], STAGE_INPUT, i);
                    const DWORD arg1 = get_source(sources[j][1], STAGE_INPUT, i);
                    const DWORD arg2 = get_source(sources[j][2], STAGE_INPUT, i);

                    const GLenum FUNCTION = functions[j];
                    const GLenum *OP = ops[j];
                    DWORD *INPUT = inputs[j];
                    DWORD *OUTPUT = outputs[j];

                    switch (FUNCTION) {
                        case GL_REPLACE:
                            // result = Arg0
                            // A = Arg0, B = 1, C = 0, D = 0, output = A*B
                            *INPUT |= PB_MASK(SRC_A, arg0) | get_op(OP[0], 'A', is_alpha);
                            *INPUT |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_INVERT;
                            *OUTPUT |= PB_MASK(OP_AxB, REG_SPARE0);
                            break;

                        case GL_MODULATE:
                            // result = Arg0 * Arg1
                            // A = Arg0, B = Arg1, C = 0, D = 0, output = A*B
                            *INPUT |= PB_MASK(SRC_A, arg0) | get_op(OP[0], 'A', is_alpha);
                            *INPUT |= PB_MASK(SRC_B, arg1) | get_op(OP[1], 'B', is_alpha);
                            *OUTPUT |= PB_MASK(OP_AxB, REG_SPARE0);
                            break;

                        case GL_ADD:
                            // result = Arg0 + Arg1
                            // A = Arg0, B = 1, C = Arg1, D = 1, output = A*B + C*D
                            *INPUT |= PB_MASK(SRC_A, arg0) | get_op(OP[0], 'A', is_alpha);
                            *INPUT |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_INVERT;
                            *INPUT |= PB_MASK(SRC_C, arg1) | get_op(OP[1], 'C', is_alpha);
                            *INPUT |= PB_MASK(SRC_D, REG_ZERO) | SRC_D_FLAG_INVERT;
                            *OUTPUT |= PB_MASK(OP_AxB_PLUS_CxD, REG_SPARE0);
                            break;

                        case GL_ADD_SIGNED:
                            // result = Arg0 + Arg1 - 0.5
                            // Implement as: Arg0 + (Arg1 - 0.5)
                            // A = Arg0, B = 1
                            // C = Arg1 (with HALF_BIAS), D = 1
                            *INPUT |= PB_MASK(SRC_A, arg0) | get_op(OP[0], 'A', is_alpha);
                            *INPUT |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_INVERT;
                            *INPUT |= PB_MASK(SRC_C, arg1) | get_op(OP[1], 'C', is_alpha) | SRC_C_FLAG_HALF_BIAS;
                            *INPUT |= PB_MASK(SRC_D, REG_ZERO) | SRC_D_FLAG_INVERT;
                            *OUTPUT |= PB_MASK(OP_AxB_PLUS_CxD, REG_SPARE0);
                            break;

                        case GL_INTERPOLATE: {
                            // result = Arg0 * Arg2 + Arg1 * (1 - Arg2)
                            // A = Arg0, B = Arg2, C = Arg1, D = (1 - Arg2), output = A*B + C*D
                            const GLenum OP2 = OP[2];
                            const GLenum _OP2 = invert_operand(OP2);
                            *INPUT |= PB_MASK(SRC_A, arg0) | get_op(OP[0], 'A', is_alpha);
                            *INPUT |= PB_MASK(SRC_B, arg2) | get_op(OP2, 'B', is_alpha);
                            *INPUT |= PB_MASK(SRC_C, arg1) | get_op(OP[1], 'C', is_alpha);
                            *INPUT |= PB_MASK(SRC_D, arg2) | get_op(_OP2, 'D', is_alpha);
                            *OUTPUT |= PB_MASK(OP_AxB_PLUS_CxD, REG_SPARE0);
                            break;
                        }

                        case GL_SUBTRACT:
                            // result = Arg0 - Arg1
                            // Arg0 − Arg1 = (Arg0 − 0.5) + (0.5 − Arg1)
                            *INPUT |= PB_MASK(SRC_A, arg0) | SRC_A_FLAG_HALF_BIAS | get_op(OP[0], 'A', is_alpha);
                            *INPUT |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_INVERT;
                            *INPUT |= PB_MASK(SRC_C, arg1) | SRC_C_FLAG_HALF_BIAS_NEGATE | get_op(OP[1], 'C', is_alpha);
                            *INPUT |= PB_MASK(SRC_D, REG_ZERO) | SRC_D_FLAG_INVERT;
                            *OUTPUT |= PB_MASK(OP_AxB_PLUS_CxD, REG_SPARE0);
                            break;

                        case GL_DOT3_RGB:
                        case GL_DOT3_RGBA:
                            // result = 4 * dot (Arg0.rgb - 0.5, Arg1.rgb - 0.5);
                            // We used EXPAND to map 0-1 to -1 to 1: ( x 2 - 1) which gives:
                            //        = dot (2 x Arg0 - 1) . (2 x Arg1 - 1). Factor out both 2's
                            //        = 4 * dot (Arg0 - 0.5, Arg1 - 0.5)) which is what we want!
                            if (!is_alpha) {
                                // For DOT3 specifically, GL only allows SRC_COLOR / ONE_MINUS_SRC_COLOR
                                DWORD OP0 =
                                    (OP[0] == GL_ONE_MINUS_SRC_COLOR) ? SRC_A_FLAG_EXPAND_NEGATE : SRC_A_FLAG_EXPAND;
                                DWORD OP1 =
                                    (OP[1] == GL_ONE_MINUS_SRC_COLOR) ? SRC_B_FLAG_EXPAND_NEGATE : SRC_B_FLAG_EXPAND;
                                *INPUT |= PB_MASK(SRC_A, arg0) | OP0;
                                *INPUT |= PB_MASK(SRC_B, arg1) | OP1;

                                *OUTPUT |= NV097_SET_COMBINER_COLOR_OCW_AB_DOT_ENABLE; // A·B instead of AxB
                                *OUTPUT |= PB_MASK(OP_AxB, REG_SPARE0);
                            } else {
                                // Alpha is just a pass-through of Arg0.a
                                *INPUT |= PB_MASK(SRC_A, arg0) | SRC_A_FLAG_ALPHA;
                                *INPUT |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;
                                *OUTPUT |= PB_MASK(OP_AxB, REG_SPARE0);
                            }
                            break;

                        default:
                            break;
                    }

                    // FIXME: Is this only applicable for GL_COMBINE
                    const GLint scale = (j == 0) ? (GLint)texture_unit->rgb_scale : (GLint)texture_unit->alpha_scale;
                    const DWORD SCALE_REG =
                        (j == 0) ? NV097_SET_COMBINER_COLOR_OCW_OP : NV097_SET_COMBINER_ALPHA_OCW_OP;
                    if (scale == 2) {
                        *OUTPUT |= PB_MASK(SCALE_REG, NV097_SET_COMBINER_COLOR_OCW_OP_SHIFTLEFTBY1);
                    } else if (scale == 4) {
                        *OUTPUT |= PB_MASK(SCALE_REG, NV097_SET_COMBINER_COLOR_OCW_OP_SHIFTLEFTBY2);
                    } else {
                        *OUTPUT |= PB_MASK(SCALE_REG, NV097_SET_COMBINER_COLOR_OCW_OP_NOSHIFT);
                    }
                }

                break;
            }
            default:
                break;
        }

        const DWORD packed_color = FLOAT4_TO_PACKED_ARGB32(texture_unit->tex_env_color);
        pb = pb_push1(pb, NV097_SET_COMBINER_FACTOR0 + (s * 4), packed_color);

        const DWORD COLOR_ICW_REGISTER = NV097_SET_COMBINER_COLOR_ICW + (s * 4);
        const DWORD ALPHA_ICW_REGISTER = NV097_SET_COMBINER_ALPHA_ICW + (s * 4);
        const DWORD COLOR_OCW_REGISTER = NV097_SET_COMBINER_COLOR_OCW + (s * 4);
        const DWORD ALPHA_OCW_REGISTER = NV097_SET_COMBINER_ALPHA_OCW + (s * 4);
        pb = pb_push1(pb, COLOR_ICW_REGISTER, RGB_IN);
        pb = pb_push1(pb, ALPHA_ICW_REGISTER, ALPHA_IN);
        pb = pb_push1(pb, COLOR_OCW_REGISTER, RGB_OUT);
        pb = pb_push1(pb, ALPHA_OCW_REGISTER, ALPHA_OUT);
        stage++;
    }
    pb_end(pb);

    // Deal with clip planes
    pb = pb_begin();
    const GLboolean any_clip_enabled =
        context->transformation_state.clip_plane_enabled[0] || context->transformation_state.clip_plane_enabled[1] ||
        context->transformation_state.clip_plane_enabled[2] || context->transformation_state.clip_plane_enabled[3];
    static GLint previous_clip_stage = -1;
    do {

        if (!any_clip_enabled) {
            break;
        }

        // Find a free unit for clip planes, search backwards should improve odds we reuse earlier units
        for (GLint i = GLI_MAX_TEXTURE_UNITS - 1; i >= 0; i--) {
            if (shader_program[i] != NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PROGRAM_NONE) {
                continue;
            }

            // Enable clip planes on this unit
            shader_program[i] = NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_CLIP_PLANE;
            previous_clip_stage = i;

            // Enable texgen for clip planes
            pb = pb_push1(pb, NV097_SET_TEXTURE_CONTROL0 + (i * 64), NV097_SET_TEXTURE_CONTROL0_ENABLE);

            if (!context->transformation_state.clip_plane_dirty && previous_clip_stage == i) {
                break;
            }
            context->transformation_state.clip_plane_dirty = GL_FALSE;

            pb = pb_push4(pb,
                          NV097_SET_TEXGEN_S + (i * 16),
                          NV097_SET_TEXGEN_S_EYE_LINEAR,  // S
                          NV097_SET_TEXGEN_S_EYE_LINEAR,  // T
                          NV097_SET_TEXGEN_S_EYE_LINEAR,  // R
                          NV097_SET_TEXGEN_S_EYE_LINEAR); // Q

            // Push clip plane equations
            vec4 *clip_plane = context->transformation_state.clip_plane;
            for (GLint j = 0; j < GLI_MAX_CLIP_PLANES; j++) {
                if (!context->transformation_state.clip_plane_enabled[j]) {
                    pb = pb_push4f(pb, NV097_SET_TEXGEN_PLANE_S + (i * 64 + j * 16), 0.0f, 0.0f, 0.0f, 0.0f);
                } else {
                    pb = pb_push4f(pb,
                                   NV097_SET_TEXGEN_PLANE_S + (i * 64 + j * 16),
                                   clip_plane[j][0],
                                   clip_plane[j][1],
                                   clip_plane[j][2],
                                   clip_plane[j][3]);
                }
            }
            break;
        }
    } while (0);
    pb_end(pb);

    // Push passthroughs to all unused stages
    pb = pb_begin();
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        if (shader_program[i] == NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PROGRAM_NONE) {
            DWORD STAGE_INPUT = (i == 0) ? REG_COLOR0 : REG_SPARE0;
            DWORD RGB_IN = 0;
            DWORD RGB_OUT = 0;
            DWORD ALPHA_IN = 0;
            DWORD ALPHA_OUT = 0;

            shader_program[i] = NV097_SET_SHADER_STAGE_PROGRAM_STAGEn_PASS_THROUGH;

            // Color: out.rgb = STAGE_INPUT * 1
            // Alpha: out.a = STAGE_INPUT.a * 1
            // Output to SPARE0
            RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
            RGB_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_INVERT; // Inverting zero gives 1

            ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
            ALPHA_IN |= PB_MASK(SRC_B, REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

            RGB_OUT |= PB_MASK(OP_AxB, REG_SPARE0);
            ALPHA_OUT |= PB_MASK(OP_AxB, REG_SPARE0);

            const DWORD COLOR_ICW_REGISTER = NV097_SET_COMBINER_COLOR_ICW + (i * 4);
            const DWORD ALPHA_ICW_REGISTER = NV097_SET_COMBINER_ALPHA_ICW + (i * 4);
            const DWORD COLOR_OCW_REGISTER = NV097_SET_COMBINER_COLOR_OCW + (i * 4);
            const DWORD ALPHA_OCW_REGISTER = NV097_SET_COMBINER_ALPHA_OCW + (i * 4);
            pb = pb_push1(pb, COLOR_ICW_REGISTER, RGB_IN);
            pb = pb_push1(pb, ALPHA_ICW_REGISTER, ALPHA_IN);
            pb = pb_push1(pb, COLOR_OCW_REGISTER, RGB_OUT);
            pb = pb_push1(pb, ALPHA_OCW_REGISTER, ALPHA_OUT);
        }
    }
    pb_end(pb);

    // Set shader stage programs
    pb = pb_begin();
    pb = pb_push1(pb,
                  NV097_SET_SHADER_STAGE_PROGRAM,
                  PB_MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE0, shader_program[0]) |
                      PB_MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE1, shader_program[1]) |
                      PB_MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE2, shader_program[2]) |
                      PB_MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE3, shader_program[3]));

    pb_end(pb);
}

uint32_t *combiner_specular_fog_config(uint32_t *p, GLboolean fog_enabled, GLboolean specular_enabled)
{
    DWORD cw0 = 0;
    DWORD cw1 = 0;
    if (fog_enabled) {
        // A = fog.alpha
        // B = specular.rgb or spare0.rgb
        // C = fog.rgb
        // D = 0
        cw0 |= PB_MASK(SRC_A, REG_FOG) | SRC_A_FLAG_ALPHA;
        cw0 |= PB_MASK(SRC_C, REG_FOG) | SRC_C_FLAG_RGB;
        cw0 |= PB_MASK(SRC_D, REG_ZERO);

        if (specular_enabled) {
            cw0 |= PB_MASK(SRC_B, REG_SPECULAR) | SRC_B_FLAG_RGB;
        } else {
            cw0 |= PB_MASK(SRC_B, REG_SPARE0) | SRC_B_FLAG_RGB;
        }
    } else {
        // A = 0
        // B = 0
        // C = 0
        // D = specular.rgb or spare0.rgb
        cw0 |= PB_MASK(SRC_A, REG_ZERO);
        cw0 |= PB_MASK(SRC_B, REG_ZERO);
        cw0 |= PB_MASK(SRC_C, REG_ZERO);

        if (specular_enabled) {
            cw0 |= PB_MASK(SRC_D, REG_SPECULAR) | SRC_D_FLAG_RGB;
        } else {
            cw0 |= PB_MASK(SRC_D, REG_SPARE0) | SRC_D_FLAG_RGB;
        }
    }

    p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, cw0);

    // E = 0
    // F = 0
    // G = spare0.alpha
    cw1 |= PB_MASK(SRC_E, REG_ZERO);
    cw1 |= PB_MASK(SRC_F, REG_ZERO);
    cw1 |= PB_MASK(SRC_G, REG_SPARE0) | SRC_G_FLAG_ALPHA;
    cw1 |= PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_CLAMP, specular_enabled ? 1 : 0);
    p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1, cw1);
    return p;
}
