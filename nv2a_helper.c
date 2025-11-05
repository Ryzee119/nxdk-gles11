#include "gles_private.h"

XguVertexArrayType _gl_enum_to_xgu_type(GLenum type)
{
    switch (type) {
        case GL_UNSIGNED_BYTE:
            return XGU_UNSIGNED_BYTE_OGL;
        case GL_SHORT:
            return XGU_SHORT;
        case GL_FLOAT:
            return XGU_FLOAT;
        // case GL_FIXED:
        //     return XGU_FLOAT; // FIXME
        default:
            return -1; // Fallback
    }
}

XguPrimitiveType _gl_enum_to_xgu_primitive(GLenum mode)
{
    switch (mode) {
        case GL_POINTS:
            return XGU_POINTS;
        case GL_LINES:
            return XGU_LINES;
        case GL_LINE_STRIP:
            return XGU_LINE_STRIP;
        case GL_LINE_LOOP:
            return XGU_LINE_LOOP;
        case GL_TRIANGLES:
            return XGU_TRIANGLES;
        case GL_TRIANGLE_STRIP:
            return XGU_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN:
            return XGU_TRIANGLE_FAN;
        default:
            return -1;
    }
}

// FIXME. No idea if these are correct.
DWORD _gl_enum_to_xgu_logic_op(GLenum opcode)
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

XguFuncType _gl_enum_to_xgu_func(GLenum func)
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

XguBlendFactor _gl_enum_to_xgu_blend_factor(GLenum factor)
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

XguCullFace _gl_enum_to_xgu_cull_face(GLenum mode)
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

XguFrontFace _gl_enum_to_xgu_front_face(GLenum mode)
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

XguStencilOp _gl_enum_to_xgu_stencilop(GLenum op)
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

XguShadeModel _gl_enum_to_xgu_shade_model(GLenum code)
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

XguFogMode _gl_enum_to_xgu_fog_mode(GLenum mode)
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

XguTextureAddress _gl_wrap_to_xgu_address_mode(GLenum wrap)
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

XguTexFilter _gl_filter_to_xgu_tex_filter(GLenum filter)
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

XguTexFormatColor _gl_enum_to_xgu_tex_format(GLenum format, GLenum type, GLuint *bytes_per_pixel)
{
    if (type == GL_UNSIGNED_BYTE) {
        switch (format) {
            case GL_ALPHA:
                *bytes_per_pixel = 1;
                return XGU_TEXTURE_FORMAT_A8_SWIZZLED;
            case GL_LUMINANCE:
                *bytes_per_pixel = 1;
                return XGU_TEXTURE_FORMAT_Y8_SWIZZLED;
            case GL_LUMINANCE_ALPHA:
                *bytes_per_pixel = 2;
                return XGU_TEXTURE_FORMAT_AY8_SWIZZLED;
            case GL_RGB:
            case GL_RGBA:
                *bytes_per_pixel = 4;
                return XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED;
            default:
                break;
        }
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        *bytes_per_pixel = 2;
        return XGU_TEXTURE_FORMAT_R5G6B5_SWIZZLED;
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        *bytes_per_pixel = 2;
        return XGU_TEXTURE_FORMAT_A4R4G4B4_SWIZZLED;
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        *bytes_per_pixel = 2;
        return XGU_TEXTURE_FORMAT_A1R5G5B5_SWIZZLED;
    }

    return (XguTexFormatColor)0;
}

#define NV_REG_ZERO      0x0
#define NV_REG_DISCARD   0x0
#define NV_REG_CONSTANT0 0x1
#define NV_REG_CONSTANT1 0x2
#define NV_REG_FOG       0x3
#define NV_REG_COLOR0    0x4
#define NV_REG_COLOR1    0x5

#define NV_REG_TEXTURE0 0x8
#define NV_REG_TEXTURE1 0x9
#define NV_REG_TEXTURE2 0xA
#define NV_REG_TEXTURE3 0xB
#define NV_REG_SPARE0   0xC
#define NV_REG_SPARE1   0xD
#define NV_REG_TEXTURE6 0xE
#define NV_REG_TEXTURE7 0xF
#define NV_REG_SPECLIT  0xE

#define NV_MAP_UNSIGNED_IDENTITY 0x0
#define NV_MAP_UNSIGNED_INVERT   0x1
#define NV_MAP_EXPAND_NORMAL     0x2
#define NV_MAP_EXPAND_NEGATE     0x3
#define NV_MAP_HALFBIAS_NORMAL   0x4
#define NV_MAP_HALFBIAS_NEGATE   0x5
#define NV_MAP_SIGNED_IDENTITY   0x6
#define NV_MAP_SIGNED_NEGATE     0x7

// Although these macros use COLOR combiner registers, ALPHA combiners and fog/specular happen to use the same bitfields
// so we can reuse them.
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

#define SRC_A_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_A_MAP, NV_MAP_UNSIGNED_INVERT)
#define SRC_B_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_B_MAP, NV_MAP_UNSIGNED_INVERT)
#define SRC_C_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_C_MAP, NV_MAP_UNSIGNED_INVERT)
#define SRC_D_FLAG_INVERT PB_MASK(NV097_SET_COMBINER_COLOR_ICW_D_MAP, NV_MAP_UNSIGNED_INVERT)

#define OP_AxB          NV097_SET_COMBINER_COLOR_OCW_AB_DST
#define OP_CxD          NV097_SET_COMBINER_COLOR_OCW_CD_DST
#define OP_AxB_PLUS_CxD NV097_SET_COMBINER_COLOR_OCW_SUM_DST

#define PB_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))
#define MASK               PB_MASK

void combiner_init(void)
{
    uint32_t *p = pb_begin();
    pb_push1(
        p,
        NV097_SET_SHADER_OTHER_STAGE_INPUT,
        PB_MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE1, NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE1_INSTAGE_0) |
            PB_MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE2, NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE2_INSTAGE_0) |
            PB_MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE3, NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE3_INSTAGE_0));
    p += 2;

    pb_push1(p,
             NV097_SET_COMBINER_CONTROL,
             PB_MASK(NV097_SET_COMBINER_CONTROL_FACTOR0, NV097_SET_COMBINER_CONTROL_FACTOR0_SAME_FACTOR_ALL) |
                 PB_MASK(NV097_SET_COMBINER_CONTROL_FACTOR1, NV097_SET_COMBINER_CONTROL_FACTOR1_SAME_FACTOR_ALL) |
                 PB_MASK(NV097_SET_COMBINER_CONTROL_ITERATION_COUNT, 1));
    p += 2;

    pb_end(p);
}

void combiner_set_texture_env(void)
{
    gli_context_t *context = gliGetContext();
    uint32_t *pb;

    DWORD shader_program[4] = {
        NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PROGRAM_NONE,
        NV097_SET_SHADER_STAGE_PROGRAM_STAGE1_PROGRAM_NONE,
        NV097_SET_SHADER_STAGE_PROGRAM_STAGE2_PROGRAM_NONE,
        NV097_SET_SHADER_STAGE_PROGRAM_STAGE3_PROGRAM_NONE,
    };

    // Update combiner stages
    pb = pb_begin();
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        texture_unit_t *texture_unit = &context->texture_environment.texture_units[i];
        texture_object_t *texture_object = texture_unit->bound_texture_object;

        DWORD STAGE_INPUT = (i == 0) ? NV_REG_COLOR0 : NV_REG_SPARE0;
        DWORD RGB_IN = 0;
        DWORD RGB_OUT = 0;
        DWORD ALPHA_IN = 0;
        DWORD ALPHA_OUT = 0;

        if (!texture_unit->texture_2d_enabled) {
            // Pass through
            // Color: out.rgb = STAGE_INPUT * 1 → pass-through
            // Alpha: out.a = STAGE_INPUT.a * 1 → pass-through
            RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
            RGB_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_INVERT;

            ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
            ALPHA_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

            RGB_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
            ALPHA_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
        } else {
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
                    RGB_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_INVERT; // 1 - 0 = 1

                    // COLOR CD: C = texture.rgb, D = 1.0
                    RGB_IN |= PB_MASK(SRC_C, NV_REG_TEXTURE0 + i);
                    RGB_IN |= PB_MASK(SRC_D, NV_REG_ZERO) | SRC_D_FLAG_INVERT;

                    // SUM (AB + CD) -> SPARE0.rgb
                    RGB_OUT |= PB_MASK(OP_AxB_PLUS_CxD, NV_REG_SPARE0);

                    // ALPHA: pass-through Af
                    ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                    ALPHA_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

                    // SUM (AB + CD) -> SPARE0.a
                    ALPHA_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
                    break;

                case GL_MODULATE:
                    // ---------------------------------------------------------
                    // MODULATE:
                    //   C = Cf * Ct
                    //   A = Af * At
                    // ---------------------------------------------------------

                    // COLOR: A = STAGE_INPUT.rgb, B = texture.rgb
                    RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
                    RGB_IN |= PB_MASK(SRC_B, NV_REG_TEXTURE0 + i);

                    // ALPHA: A = STAGE_INPUT.a, B = texture.a
                    ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                    ALPHA_IN |= PB_MASK(SRC_B, NV_REG_TEXTURE0 + i) | SRC_B_FLAG_ALPHA;

                    RGB_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0); // SPARE0 = A*B
                    ALPHA_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
                    break;

                case GL_REPLACE:
                    // ---------------------------------------------------------
                    // REPLACE:
                    //   C = Ct
                    //   A = At   (for RGBA textures; for RGB, A = Af)
                    // ---------------------------------------------------------

                    // COLOR: out.rgb = texture.rgb
                    RGB_IN |= PB_MASK(SRC_A, NV_REG_TEXTURE0 + i);
                    RGB_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_INVERT; // B=1, so A*B = tex

                    // ALPHA: out.a = texture.a
                    ALPHA_IN |= PB_MASK(SRC_A, NV_REG_TEXTURE0 + i) | SRC_A_FLAG_ALPHA;
                    ALPHA_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

                    RGB_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
                    ALPHA_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
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
                    RGB_IN |= PB_MASK(SRC_B, NV_REG_TEXTURE0 + i) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;

                    // COLOR CD: C = texture.rgb, D = texture alpha
                    RGB_IN |= PB_MASK(SRC_C, NV_REG_TEXTURE0 + i);
                    RGB_IN |= PB_MASK(SRC_D, NV_REG_TEXTURE0 + i) | SRC_D_FLAG_ALPHA;

                    RGB_OUT |= PB_MASK(OP_AxB_PLUS_CxD, NV_REG_SPARE0); // AB + CD

                    // Alpha unchanged: Af
                    ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                    ALPHA_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;
                    ALPHA_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
                    break;

                case GL_BLEND:
                    // ---------------------------------------------------------
                    // BLEND (RGBA textures):
                    //   C = Cf * (1 - Ct) + Cc * Ct
                    //   A = Af
                    //
                    // Cc is GL_TEXTURE_ENV_COLOR in NV_REG_CONSTANT0.
                    //
                    // Implement:
                    //   AB = Cf * (1 - Ct)
                    //   CD = Cc * Ct
                    //   SUM = AB + CD -> SPARE0
                    // ---------------------------------------------------------

                    // COLOR AB: A = STAGE_INPUT.rgb, B = (1 - Ct.rgb)
                    RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
                    RGB_IN |= PB_MASK(SRC_B, NV_REG_TEXTURE0 + i) | SRC_B_FLAG_INVERT;

                    // COLOR CD: C = CONSTANT0 (env color), D = texture.rgb
                    RGB_IN |= PB_MASK(SRC_C, NV_REG_CONSTANT0);
                    RGB_IN |= PB_MASK(SRC_D, NV_REG_TEXTURE0 + i);

                    RGB_OUT |= PB_MASK(OP_AxB_PLUS_CxD, NV_REG_SPARE0); // AB + CD -> SPARE0

                    // Alpha unchanged: Af
                    ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                    ALPHA_IN |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_ALPHA | SRC_B_FLAG_INVERT;
                    ALPHA_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
                    break;

                case GL_COMBINE:
                    // ---------------------------------------------------------
                    // COMBINE is the general case (COMBINE_RGB / COMBINE_ALPHA,
                    // SRC0/1/2, OPERAND0/1/2, SCALE, etc).
                    //
                    // Full implementation needs all the tex_env_combine state
                    // (texture_unit->combine_rgb, src0_rgb, op0_rgb, etc).
                    //
                    // For now, implement a safe fallback equivalent to MODULATE:
                    //   C = Cf * Ct
                    //   A = Af * At
                    // ---------------------------------------------------------
                    RGB_IN |= PB_MASK(SRC_A, STAGE_INPUT);
                    RGB_IN |= PB_MASK(SRC_B, NV_REG_TEXTURE0 + i);

                    ALPHA_IN |= PB_MASK(SRC_A, STAGE_INPUT) | SRC_A_FLAG_ALPHA;
                    ALPHA_IN |= PB_MASK(SRC_B, NV_REG_TEXTURE0 + i) | SRC_B_FLAG_ALPHA;

                    RGB_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);
                    ALPHA_OUT |= PB_MASK(OP_AxB, NV_REG_SPARE0);

                    // TODO: Replace this with full COMBINE implementation
                    break;

                default:
                    break;
            }
            shader_program[i] = NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_2D_PROJECTIVE;
            // NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_CUBE_MAP or NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_CLIP_PLANE
            // NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PASS_THROUGH?
        }

        DWORD COLOR_ICW_REGISTER = NV097_SET_COMBINER_COLOR_ICW + (i * 4);
        DWORD ALPHA_ICW_REGISTER = NV097_SET_COMBINER_ALPHA_ICW + (i * 4);
        DWORD COLOR_OCW_REGISTER = NV097_SET_COMBINER_COLOR_OCW + (i * 4);
        DWORD ALPHA_OCW_REGISTER = NV097_SET_COMBINER_ALPHA_OCW + (i * 4);

        pb = pb_push1(pb, COLOR_ICW_REGISTER, RGB_IN);
        pb = pb_push1(pb, ALPHA_ICW_REGISTER, ALPHA_IN);
        pb = pb_push1(pb, COLOR_OCW_REGISTER, RGB_OUT);
        pb = pb_push1(pb, ALPHA_OCW_REGISTER, ALPHA_OUT);
    }

    // Enable shader stages
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
    if (fog_enabled) {
        // A = fog.alpha
        // C = fog.rgb
        // D = 0
        // B = specular.rgb or spare0.rgb
        cw0 |= PB_MASK(SRC_A, NV_REG_FOG) | SRC_A_FLAG_ALPHA;
        cw0 |= PB_MASK(SRC_C, NV_REG_FOG) | SRC_C_FLAG_RGB;
        cw0 |= PB_MASK(SRC_D, NV_REG_ZERO) | SRC_D_FLAG_RGB;

        if (specular_enabled) {
            cw0 |= PB_MASK(SRC_B, NV_REG_SPECLIT) | SRC_B_FLAG_RGB;
        } else {
            cw0 |= PB_MASK(SRC_B, NV_REG_SPARE0) | SRC_B_FLAG_RGB;
        }
    } else {
        // A = 0
        // B = 0
        // C = 0
        // D = specular.rgb or spare0.rgb
        cw0 |= PB_MASK(SRC_A, NV_REG_ZERO) | SRC_A_FLAG_RGB;
        cw0 |= PB_MASK(SRC_B, NV_REG_ZERO) | SRC_B_FLAG_RGB;
        cw0 |= PB_MASK(SRC_C, NV_REG_ZERO) | SRC_C_FLAG_RGB;

        if (specular_enabled) {
            cw0 |= PB_MASK(SRC_D, NV_REG_SPECLIT) | SRC_D_FLAG_RGB;
        } else {
            cw0 |= PB_MASK(SRC_D, NV_REG_SPARE0) | SRC_D_FLAG_RGB;
        }
    }

    pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, cw0);
    p += 2;

    // E = 0
    // F = 0
    // G = spare0.alpha
    pb_push1(p,
             NV097_SET_COMBINER_SPECULAR_FOG_CW1,
             PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_SOURCE, NV_REG_ZERO) |
                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_ALPHA, 0) |
                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_INVERSE, 0) |

                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_SOURCE, NV_REG_ZERO) |
                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_ALPHA, 0) |
                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_INVERSE, 0) |

                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_SOURCE, NV_REG_SPARE0) |
                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_ALPHA, 1) |
                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_INVERSE, 0) |
                 PB_MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_CLAMP, 1));
    p += 2;
    return p;
}
