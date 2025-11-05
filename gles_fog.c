#include "gles_private.h"

GL_API void GL_APIENTRY glFogf(GLenum pname, GLfloat param)
{
    gli_context_t *context = gliGetContext();
    uint32_t *pb;

    switch (pname) {
        case GL_FOG_MODE:
            GLenum mode = (GLenum)(GLuint)param;
            if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            context->coloring_state.fog_mode = mode;
            context->coloring_state.fog_mode_dirty = GL_TRUE;
            break;
        case GL_FOG_DENSITY:
            if (param < 0.0f) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            context->coloring_state.fog_density = param;
            context->coloring_state.fog_density_dirty = GL_TRUE;
            break;
        case GL_FOG_START:
            context->coloring_state.fog_start = param;
            context->coloring_state.fog_start_dirty = GL_TRUE;
            break;
        case GL_FOG_END:
            context->coloring_state.fog_end = param;
            context->coloring_state.fog_end_dirty = GL_TRUE;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            break;
    }
}

GL_API void GL_APIENTRY glFogfv(GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();
    uint32_t *pb;

    switch (pname) {
        case GL_FOG_COLOR:
            glm_vec4_copy((GLfloat *)params, context->coloring_state.fog_color);
            glm_vec4_clamp(context->coloring_state.fog_color, 0.0f, 1.0f);
            context->coloring_state.fog_color_dirty = GL_TRUE;
            break;
        default:
            glFogf(pname, params[0]);
            break;
    }
}

GL_API void GL_APIENTRY glFogx(GLenum pname, GLfixed param)
{
    GLfloat paramf = gliFixedtoFloat(param);
    glFogf(pname, paramf);
}

GL_API void GL_APIENTRY glFogxv(GLenum pname, const GLfixed *params)
{
    GLfloat paramsf[4];
    GLuint param_count = (pname == GL_FOG_COLOR) ? 4 : 1;
    for (int i = 0; i < param_count; i++) {
        paramsf[i] = gliFixedtoFloat(params[i]);
    }
    glFogfv(pname, paramsf);
}

void gliFogFlush(void)
{
    gli_context_t *context = gliGetContext();
    coloring_state_t *cs = &context->coloring_state;
    uint32_t *pb = pb_begin();
    pb = xgu_set_fog_gen_mode(pb, XGU_FOG_GEN_MODE_RADIAL);
    pb_end(pb);

    if (cs->fog_color_dirty) {

        uint32_t r = (uint32_t)(cs->fog_color[0] * 255.0f) & 0xFF;
        uint32_t g = (uint32_t)(cs->fog_color[1] * 255.0f) & 0xFF;
        uint32_t b = (uint32_t)(cs->fog_color[2] * 255.0f) & 0xFF;
        uint32_t a = (uint32_t)(cs->fog_color[3] * 255.0f) & 0xFF;
        uint32_t color = (a << 24) | (b << 16) | (g << 8) | r;
        pb = pb_begin();
        pb = xgu_set_fog_color(pb, color);
        pb_end(pb);
        cs->fog_color_dirty = GL_FALSE;
    }

    if (cs->fog_density_dirty || cs->fog_start_dirty || cs->fog_end_dirty || cs->fog_mode_dirty) {

        if (cs->fog_mode_dirty) {
            uint32_t *pb = pb_begin();
            pb = xgu_set_fog_mode(pb, _gl_enum_to_xgu_fog_mode(cs->fog_mode));
            pb_end(pb);
            cs->fog_mode_dirty = GL_FALSE;
        }

        const GLenum mode = cs->fog_mode;
        const float density = cs->fog_density;
        const float start = cs->fog_start;
        const float end = cs->fog_end;

        const float LN256 = 5.5452f;
        const float SQRT_LN256 = 2.354f; // sqrt(5.5452)

        float bias = 0.0f, scale = 0.0f;
        // https://github.com/xemu-project/xemu/blob/5eb603b28d926bc145dcaca63cce173e1008a144/hw/xbox/nv2a/pgraph/glsl/vsh.c#L324
        // https://registry.khronos.org/OpenGL-Refpages/es1.1/xhtml/glFog.xml (equations are wrong here?)
        switch (mode) {
            case GL_EXP:
                // f = 1 / exp(density * d)
                bias = 1.5f;
                scale = -density * (1.0f / (2.0f * LN256));
                break;

            case GL_EXP2:
                // f = 1 / exp((density * d)^2)
                bias = 1.5f;
                scale = -(density) * (1.0f / (2.0f * SQRT_LN256));
                break;

            case GL_LINEAR: {
                // f = (end - c) / (end - start)
                const GLfloat range = end - start;
                if (range == 0.0f) {
                    bias = 1.0f;
                    scale = 1.0f;
                    break;
                }
                bias = 1.0f + (end / (range));
                scale = -1.0f / (range);
                break;
            }
            default:
                assert(0 && "Unhandled fog mode in gliFogFlush");
                return;
        }

        uint32_t *pb = pb_begin();
        pb = xgu_set_fog_params(pb, bias, scale);
        pb_end(pb);
    }
}
