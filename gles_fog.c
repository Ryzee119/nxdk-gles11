#include "gles_private.h"

GL_API void GL_APIENTRY glFogf(GLenum pname, GLfloat param)
{
    gli_context_t *context = gliGetContext();

    switch (pname) {
        case GL_FOG_MODE:
            GLenum mode = (GLenum)(GLuint)param;
            if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2) {
                gliSetError(GL_INVALID_ENUM);
                return;
            }
            context->coloring_state.fog_mode = mode;
            break;
        case GL_FOG_DENSITY:
            if (param < 0.0f) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            context->coloring_state.fog_density = param;
            break;
        case GL_FOG_START:
            context->coloring_state.fog_start = param;
            break;
        case GL_FOG_END:
            context->coloring_state.fog_end = param;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
    context->coloring_state.fog_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glFogfv(GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();

    switch (pname) {
        case GL_FOG_COLOR:
            glm_vec4_copy((GLfloat *)params, context->coloring_state.fog_color);
            // All color components are clamped to the range 0 to 1 
            glm_vec4_clamp(context->coloring_state.fog_color, 0.0f, 1.0f);
            break;
        default:
            glFogf(pname, params[0]);
            return;
    }
    context->coloring_state.fog_dirty = GL_TRUE;
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
    for (GLuint i = 0; i < param_count; i++) {
        paramsf[i] = gliFixedtoFloat(params[i]);
    }
    glFogfv(pname, paramsf);
}

void gliFogFlush(void)
{
    gli_context_t *context = gliGetContext();
    coloring_state_t *cs = &context->coloring_state;
    uint32_t *pb;

    if (cs->fog_dirty) {
        cs->fog_dirty = GL_FALSE;

        pb = pb_begin();
        pb = xgu_set_fog_mode(pb, gliEnumToNvFogMode(cs->fog_mode));
        pb = xgu_set_fog_color(pb, FLOAT4_TO_PACKED_ABGR32(cs->fog_color));

        const GLenum mode = cs->fog_mode;
        const GLfloat density = cs->fog_density;
        const GLfloat start = cs->fog_start;
        const GLfloat end = cs->fog_end;

        const GLfloat LN256 = 5.5452f;
        const GLfloat SQRT_LN256 = 2.354f; // sqrt(5.5452)

        GLfloat bias = 0.0f, scale = 0.0f;
        // https://github.com/xemu-project/xemu/blob/5eb603b28d926bc145dcaca63cce173e1008a144/hw/xbox/nv2a/pgraph/glsl/vsh.c#L324
        switch (mode) {
            case GL_EXP:
                // f = 1 / exp(density * d)
                // fogParam.x = 1.5
                // fogParam.y = -density / (2 * ln(256))
                bias = 1.5f;
                scale = -density / (2.0f * LN256);
                break;

            case GL_EXP2:
                // f = 1 / exp((d * density)^2)
                // fogParam.x = 1.5
                // fogParam.y = -density / (2 * sqrt(ln(256)))
                bias = 1.5f;
                scale = -density / (2.0f * SQRT_LN256);
                break;

            case GL_LINEAR: {
                // f = (end - c) / (end - start)
                // fogParam.y = -1 / (end - start)    // scale
                // fogParam.x = 1 - end * fogParam.y; // bias
                const GLfloat range = end - start;
                if (range == 0.0f) {
                    bias = 1.0f;
                    scale = 1.0f;
                    break;
                }
                scale = -1.0f / (range);
                bias = 1.0f - (end * scale);
                break;
            }
            default:
                assert(0 && "Unhandled fog mode in gliFogFlush");
                return;
        }

        pb = xgu_set_fog_params(pb, bias, scale);
        pb_end(pb);
    }
}
