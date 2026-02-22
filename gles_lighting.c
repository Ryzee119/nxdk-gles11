#include "gles_private.h"

GL_API void GL_APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();

    if (light < GL_LIGHT0 || light >= GL_LIGHT0 + GLI_MAX_LIGHTS) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    light_t *light_object = &context->lighting_state.lights[light - GL_LIGHT0];

    switch (pname) {
        case GL_SPOT_EXPONENT:
            if (!(params[0] >= 0.0f && params[0] <= 128.0f)) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            light_object->spot_exponent = params[0];
            break;

        case GL_SPOT_CUTOFF:
            if (!(params[0] == 180.0f || (params[0] >= 0.0f && params[0] <= 90.0f))) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            light_object->spot_cutoff = params[0];
            break;

        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
            if (!(params[0] >= 0.0f)) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            if (pname == GL_CONSTANT_ATTENUATION) {
                light_object->constant_attenuation = params[0];
            }
            if (pname == GL_LINEAR_ATTENUATION) {
                light_object->linear_attenuation = params[0];
            }
            if (pname == GL_QUADRATIC_ATTENUATION) {
                light_object->quadratic_attenuation = params[0];
            }
            break;

        case GL_AMBIENT:
            glm_vec4_copy((float *)params, light_object->ambient);
            glm_vec4_clamp(light_object->ambient, 0.0f, 1.0f);
            break;

        case GL_DIFFUSE:
            glm_vec4_copy((float *)params, light_object->diffuse);
            glm_vec4_clamp(light_object->diffuse, 0.0f, 1.0f);
            break;

        case GL_SPECULAR:
            glm_vec4_copy((float *)params, light_object->specular);
            glm_vec4_clamp(light_object->specular, 0.0f, 1.0f);
            break;

        case GL_POSITION: {
            mat4 *model_view = gliCurrentModelView();

            // The position is transformed by the modelview matrix when glLight is called
            glm_mat4_mulv(*model_view, (vec4){params[0], params[1], params[2], params[3]}, light_object->position);
            break;
        }

        case GL_SPOT_DIRECTION: {
            mat4 *model_view = gliCurrentModelView();

            // The spot direction is transformed by the upper 3x3 of the modelview matrix when glLight is called,
            vec3 direction = {0};
            glm_mat4_mulv3(*model_view, (vec3){params[0], params[1], params[2]}, 0.0f, direction);
            glm_vec3_normalize(direction);

            glm_vec3_copy(direction, light_object->spot_direction);
            break;
        }
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
    light_object->light_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
    glLightfv(light, pname, &param);
}

GL_API void GL_APIENTRY glLightx(GLenum light, GLenum pname, GLfixed param)
{
    glLightf(light, pname, gliFixedtoFloat(param));
}

GL_API void GL_APIENTRY glLightxv(GLenum light, GLenum pname, const GLfixed *params)
{
    GLint param_count = 1;

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            param_count = 4;
            break;
        case GL_SPOT_DIRECTION:
            param_count = 3;
            break;
        default:
            glLightx(light, pname, params[0]);
            return;
    }
    GLfloat paramsf[4];
    for (GLint i = 0; i < param_count; i++) {
        paramsf[i] = (GLfloat)gliFixedtoFloat(params[i]);
    }
    glLightfv(light, pname, paramsf);
}

GL_API void GL_APIENTRY glLightModelfv(GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
        case GL_LIGHT_MODEL_AMBIENT:
            glm_vec4_copy((float *)params, context->lighting_state.light_model_ambient);
            break;
        case GL_LIGHT_MODEL_TWO_SIDE:
            context->lighting_state.light_model_two_side = params[0] ? GL_TRUE : GL_FALSE;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
    context->lighting_state.lighting_model_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glLightModelf(GLenum pname, GLfloat param)
{
    glLightModelfv(pname, &param);
}

GL_API void GL_APIENTRY glLightModelx(GLenum pname, GLfixed param)
{
    GLfloat paramf = gliFixedtoFloat(param);
    glLightModelf(pname, paramf);
}

GL_API void GL_APIENTRY glLightModelxv(GLenum pname, const GLfixed *params)
{
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
        case GL_LIGHT_MODEL_AMBIENT: {
            GLfloat paramsf[4];
            for (int i = 0; i < 4; ++i) {
                paramsf[i] = gliFixedtoFloat(params[i]);
            }
            glLightModelfv(pname, paramsf);
            return;
        }
        case GL_LIGHT_MODEL_TWO_SIDE:
            glLightModelf(pname, gliFixedtoFloat(params[0]));
            return;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();
    material_t *material[2];

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (face) {
        case GL_FRONT:
            material[0] = &context->lighting_state.material_front;
            material[1] = NULL;
            break;
        case GL_BACK:
            material[0] = NULL;
            material[1] = &context->lighting_state.material_back;
            break;
        case GL_FRONT_AND_BACK:
            material[0] = &context->lighting_state.material_front;
            material[1] = &context->lighting_state.material_back;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }

    for (GLint i = 0; i < 2; i++) {
        if (material[i] == NULL) {
            continue;
        }
        switch (pname) {
            case GL_AMBIENT:
                glm_vec4_copy((float *)params, material[i]->ambient);
                break;
            case GL_DIFFUSE:
                glm_vec4_copy((float *)params, material[i]->diffuse);
                break;
            case GL_SPECULAR:
                glm_vec4_copy((float *)params, material[i]->specular);
                break;
            case GL_EMISSION:
                glm_vec4_copy((float *)params, material[i]->emission);
                break;
            case GL_SHININESS:
                if (params[0] < 0.0f || params[0] > 128.0f) {
                    gliSetError(GL_INVALID_VALUE);
                    return;
                }
                material[i]->shininess = params[0];
                xgux_set_specular_gl(material[i]->shininess);
                break;
            case GL_AMBIENT_AND_DIFFUSE:
                glm_vec4_copy((float *)params, material[i]->ambient);
                glm_vec4_copy((float *)params, material[i]->diffuse);
                break;
            default:
                gliSetError(GL_INVALID_ENUM);
                return;
        }
        material[i]->material_dirty = GL_TRUE;
    }
}

GL_API void GL_APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    glMaterialfv(face, pname, &param);
}

GL_API void GL_APIENTRY glMaterialx(GLenum face, GLenum pname, GLfixed param)
{
    glMaterialf(face, pname, gliFixedtoFloat(param));
}

GL_API void GL_APIENTRY glMaterialxv(GLenum face, GLenum pname, const GLfixed *params)
{
    GLfloat paramsf[4];
    GLint param_count = 1;

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (pname) {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_AMBIENT_AND_DIFFUSE:
            param_count = 4;
            break;
        default:
            glMaterialx(face, pname, params[0]);
            return;
    }
    for (GLint i = 0; i < param_count; i++) {
        paramsf[i] = (GLfloat)gliFixedtoFloat(params[i]);
    }
    glMaterialfv(face, pname, paramsf);
}

GL_API void GL_APIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    gli_context_t *context = gliGetContext();

    if (light < GL_LIGHT0 || light >= GL_LIGHT0 + GLI_MAX_LIGHTS) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    light_t *light_object = &context->lighting_state.lights[light - GL_LIGHT0];

    switch (pname) {
        case GL_AMBIENT:
            glm_vec4_copy(light_object->ambient, params);
            break;
        case GL_DIFFUSE:
            glm_vec4_copy(light_object->diffuse, params);
            break;
        case GL_SPECULAR:
            glm_vec4_copy(light_object->specular, params);
            break;
        case GL_POSITION:
            glm_vec4_copy(light_object->position, params);
            break;
        case GL_SPOT_DIRECTION:
            glm_vec3_copy(light_object->spot_direction, params);
            break;
        case GL_SPOT_EXPONENT:
            params[0] = light_object->spot_exponent;
            break;
        case GL_SPOT_CUTOFF:
            params[0] = light_object->spot_cutoff;
            break;
        case GL_CONSTANT_ATTENUATION:
            params[0] = light_object->constant_attenuation;
            break;
        case GL_LINEAR_ATTENUATION:
            params[0] = light_object->linear_attenuation;
            break;
        case GL_QUADRATIC_ATTENUATION:
            params[0] = light_object->quadratic_attenuation;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glGetLightxv(GLenum light, GLenum pname, GLfixed *params)
{
    GLfloat paramsf[4];

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLint param_count = 1;
    switch (pname) {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            param_count = 4;
            break;
        case GL_SPOT_DIRECTION:
            param_count = 3;
            break;
        default:
            break;
    }

    glGetLightfv(light, pname, paramsf);
    for (GLint i = 0; i < param_count; i++) {
        params[i] = gliFloattoFixed(paramsf[i]);
    }
}

GL_API void GL_APIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    gli_context_t *context = gliGetContext();

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    material_t *material;
    switch (face) {
        case GL_FRONT:
            material = &context->lighting_state.material_front;
            break;
        case GL_BACK:
            material = &context->lighting_state.material_back;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }

    switch (pname) {
        case GL_AMBIENT:
            glm_vec4_copy(material->ambient, params);
            break;
        case GL_DIFFUSE:
            glm_vec4_copy(material->diffuse, params);
            break;
        case GL_SPECULAR:
            glm_vec4_copy(material->specular, params);
            break;
        case GL_EMISSION:
            glm_vec4_copy(material->emission, params);
            break;
        case GL_SHININESS:
            params[0] = material->shininess;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API void GL_APIENTRY glGetMaterialxv(GLenum face, GLenum pname, GLfixed *params)
{
    GLfloat paramsf[4];

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLint param_count = 1;
    switch (pname) {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
            param_count = 4;
            break;
        default:
            break;
    }
    glGetMaterialfv(face, pname, paramsf);
    for (GLint i = 0; i < param_count; i++) {
        params[i] = gliFloattoFixed(paramsf[i]);
    }
}

void gliLightingFlush(void)
{
    gli_context_t *context = gliGetContext();
    lighting_state_t *lighting = &context->lighting_state;

    material_t *materials[2] = {&lighting->material_front, &lighting->material_back};

    // github.com/abaire/nxdk_pgraph_tests/blob/5920c89548e47675f28c7e347f07fc3ee54a4709/src/tests/material_color_tests.cpp
    uint32_t *pb = pb_begin();
    for (GLint i = 0; i < 2; i++) {
        material_t *material = materials[i];

        if (!material->material_dirty && !lighting->lighting_model_dirty) {
            continue;
        }

        vec3 ambient;
        vec3 emission;
        if (!lighting->color_material_enabled) {
            ambient[0] = lighting->light_model_ambient[0] * material->ambient[0] + material->emission[0];
            ambient[1] = lighting->light_model_ambient[1] * material->ambient[1] + material->emission[1];
            ambient[2] = lighting->light_model_ambient[2] * material->ambient[2] + material->emission[2];

            emission[0] = 0.0f;
            emission[1] = 0.0f;
            emission[2] = 0.0f;
        } else {
            // material ambient comes from vertex color so dont add them here
            // In this case the emission register is used?
            ambient[0] = lighting->light_model_ambient[0];
            ambient[1] = lighting->light_model_ambient[1];
            ambient[2] = lighting->light_model_ambient[2];

            emission[0] = material->emission[0];
            emission[1] = material->emission[1];
            emission[2] = material->emission[2];
        }

        if (i == 0) {
            pb = xgu_set_scene_ambient_color(pb, ambient[0], ambient[1], ambient[2]);
            pb = xgu_set_material_emission(pb, emission[0], emission[1], emission[2]);
            pb = xgu_set_material_alpha(pb, material->diffuse[3]);
        } else {
            pb = xgu_set_back_scene_ambient_color(pb, ambient[0], ambient[1], ambient[2]);
            pb = xgu_set_back_material_emission(pb, emission[0], emission[1], emission[2]);
            pb = xgu_set_back_material_alpha(pb, material->diffuse[3]);
        }
    }
    pb_end(pb);

    static uint32_t light_mask;

    for (int i = 0; i < GLI_MAX_LIGHTS; i++) {
        uint32_t light_mask_shift = (i * 2);
        light_t *light = &lighting->lights[i];
        if (!light->enabled) {
            light_mask &= ~(0x03 << light_mask_shift);
            light_mask |= XGU_LMASK_OFF << light_mask_shift;
            continue;
        }
        if (!light->light_dirty && !lighting->lighting_model_dirty && !materials[0]->material_dirty &&
            !materials[1]->material_dirty) {
            continue;
        }

        if (light->light_dirty || lighting->lighting_model_dirty) {
            // If w == 0, it's a directional light
            if (light->position[3] == 0) {
                light_mask &= ~(0x03 << light_mask_shift);
                light_mask |= XGU_LMASK_INFINITE << light_mask_shift;

                // Blinn–Phong lighting
                // Direction of light
                vec3 L;
                glm_vec3_copy((vec3){light->position[0], light->position[1], light->position[2]}, L);
                glm_vec3_normalize(L);

                // The direction from the surface toward the viewer
                vec3 V = {0.0f, 0.0f, 1.0f};

                // H = normalize(L + V)
                vec3 H;
                glm_vec3_add(L, V, H);
                glm_vec3_normalize(H);

                pb = pb_begin();
                pb = xgu_set_light_local_range(pb, i, FLT_MAX);
                pb = xgu_set_light_infinite_half_vector(pb, i, (XguVec3){H[0], H[1], H[2]});
                pb = xgu_set_light_infinite_direction(pb, i, (XguVec3){L[0], L[1], L[2]});
                pb_end(pb);

            } else {
                light_mask &= ~(0x03 << light_mask_shift);
                light_mask |= (light->spot_cutoff == 180.0f) ? (XGU_LMASK_LOCAL << light_mask_shift)
                                                             : (XGU_LMASK_SPOT << light_mask_shift);

                pb = pb_begin();
                pb = xgu_set_light_local_range(pb, i, FLT_MAX); // FIXME: Calculate range based on attenuation?
                pb = xgu_set_light_local_position(
                    pb, i, (XguVec3){light->position[0], light->position[1], light->position[2]});
                pb_end(pb);

                pb = pb_begin();
                pb = xgu_set_light_local_attenuation(
                    pb, i, light->constant_attenuation, light->linear_attenuation, light->quadratic_attenuation);
                pb_end(pb);

                vec3 norm_spot_direction;
                glm_vec3_normalize_to(light->spot_direction, norm_spot_direction);
                xgux_set_light_spot_gl(
                    i,
                    light->spot_exponent,
                    light->spot_cutoff * M_PI / 180.0f,
                    (XguVec3){norm_spot_direction[0], norm_spot_direction[1], norm_spot_direction[2]});
            }
        }

        XguVec3 xgu_v;
        pb = pb_begin();
        for (int j = 0; j < 2; j++) {
            material_t *material = materials[j];
            if (!light->light_dirty && !material->material_dirty) {
                continue;
            }

            // Apply light colour and material color
            if (!lighting->color_material_enabled) {
                xgu_v.r = light->diffuse[0] * material->diffuse[0];
                xgu_v.g = light->diffuse[1] * material->diffuse[1];
                xgu_v.b = light->diffuse[2] * material->diffuse[2];
            } else {
                // Material diffuse comes from vertex color
                xgu_v.r = light->diffuse[0];
                xgu_v.g = light->diffuse[1];
                xgu_v.b = light->diffuse[2];
            }
            if (j == 0) {
                pb = xgu_set_light_diffuse_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
            } else {
                pb = xgu_set_back_light_diffuse_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
            }

            // Light ambient 
            xgu_v.r = light->ambient[0] * lighting->light_model_ambient[0];
            xgu_v.g = light->ambient[1] * lighting->light_model_ambient[1];
            xgu_v.b = light->ambient[2] * lighting->light_model_ambient[2];
            if (j == 0) {
                pb = xgu_set_light_ambient_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
            } else {
                pb = xgu_set_back_light_ambient_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
            }

            // Light specular and material specular
            xgu_v.r = light->specular[0] * material->specular[0];
            xgu_v.g = light->specular[1] * material->specular[1];
            xgu_v.b = light->specular[2] * material->specular[2];
            if (j == 0) {
                pb = xgu_set_light_specular_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
            } else {
                pb = xgu_set_back_light_specular_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
            }
        }
        pb_end(pb);
        light->light_dirty = GL_FALSE;
    }
    materials[0]->material_dirty = GL_FALSE;
    materials[1]->material_dirty = GL_FALSE;

    pb = pb_begin();
    pb = push_command_parameter(pb, NV097_SET_LIGHT_ENABLE_MASK, light_mask);
    pb_end(pb);

    if (lighting->lighting_model_dirty) {
        uint32_t *pb = pb_begin();
        pb = xgu_set_two_side_light_enable(pb, lighting->light_model_two_side ? true : false);

        if (lighting->color_material_enabled) {
            pb = push_command_parameter(pb,
                                        NV097_SET_COLOR_MATERIAL,
                                        NV097_SET_COLOR_MATERIAL_AMBIENT_FROM_VERTEX_DIFFUSE |
                                            NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE |
                                            NV097_SET_COLOR_MATERIAL_BACK_AMBIENT_FROM_VERTEX_DIFFUSE |
                                            NV097_SET_COLOR_MATERIAL_BACK_DIFFUSE_FROM_VERTEX_DIFFUSE);
        } else {
            pb = push_command_parameter(pb, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
        }
        pb_end(pb);
        lighting->lighting_model_dirty = GL_FALSE;
    }
}
