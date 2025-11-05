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
            light_object->spot_exponent_dirty = GL_TRUE;
            break;

        case GL_SPOT_CUTOFF:
            if (!(params[0] == 180.0f || (params[0] >= 0.0f && params[0] <= 90.0f))) {
                gliSetError(GL_INVALID_VALUE);
                return;
            }
            light_object->spot_cutoff = params[0];
            light_object->spot_cutoff_dirty = GL_TRUE;
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
                light_object->constant_attenuation_dirty = GL_TRUE;
            }
            if (pname == GL_LINEAR_ATTENUATION) {
                light_object->linear_attenuation = params[0];
                light_object->linear_attenuation_dirty = GL_TRUE;
            }
            if (pname == GL_QUADRATIC_ATTENUATION) {
                light_object->quadratic_attenuation = params[0];
                light_object->quadratic_attenuation_dirty = GL_TRUE;
            }
            break;

        case GL_AMBIENT:
            glm_vec4_copy((float *)params, light_object->ambient);
            glm_vec4_clamp(light_object->ambient, 0.0f, 1.0f);
            light_object->ambient_dirty = GL_TRUE;
            break;

        case GL_DIFFUSE:
            glm_vec4_copy((float *)params, light_object->diffuse);
            glm_vec4_clamp(light_object->diffuse, 0.0f, 1.0f);
            light_object->diffuse_dirty = GL_TRUE;
            break;

        case GL_SPECULAR:
            glm_vec4_copy((float *)params, light_object->specular);
            glm_vec4_clamp(light_object->specular, 0.0f, 1.0f);
            light_object->specular_dirty = GL_TRUE;
            break;

        case GL_POSITION: {
            mat4 *model_view = gliCurrentModelView();

            // The position is transformed by the modelview matrix when glLight is called
            glm_mat4_mulv(*model_view, (vec4){params[0], params[1], params[2], params[3]}, light_object->position);
            light_object->position_dirty = GL_TRUE;
            break;
        }

        case GL_SPOT_DIRECTION: {
            mat4 *model_view = gliCurrentModelView();

            // The spot direction is transformed by the upper 3x3 of the modelview matrix when glLight is called,
            vec3 direction = {0};
            glm_mat4_mulv3(*model_view, (vec3){params[0], params[1], params[2]}, 0.0f, direction);
            glm_vec3_normalize(direction);

            glm_vec3_copy(direction, light_object->spot_direction);
            light_object->spot_direction_dirty = GL_TRUE;
            break;
        }
        default:
            glLightf(light, pname, params[0]);
            break;
    }
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
            context->lighting_state.light_model_ambient_dirty = GL_TRUE;
            break;
        case GL_LIGHT_MODEL_TWO_SIDE:
            context->lighting_state.light_model_two_side = params[0] ? GL_TRUE : GL_FALSE;
            context->lighting_state.light_model_two_side_dirty = GL_TRUE;
            break;
        default:
            glLightModelf(pname, params[0]);
    }
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
    GLint param_count = 1;

    switch (pname) {
        case GL_LIGHT_MODEL_AMBIENT:
            param_count = 4;
            break;
        default:
            glLightModelf(pname, params[0]);
            return;
    }
    GLfloat paramsf[4];
    for (GLint i = 0; i < param_count; i++) {
        paramsf[i] = (GLfloat)gliFixedtoFloat(params[i]);
    }
    glLightModelfv(pname, paramsf);
}

GL_API void GL_APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    gli_context_t *context = gliGetContext();
    material_t *material[2] = {0};

    if (!params) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (face) {
        case GL_FRONT:
            material[0] = &context->lighting_state.material_front;
            break;
        case GL_BACK:
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
                glm_vec4_clamp(material[i]->ambient, 0.0f, 1.0f);
                material[i]->ambient_dirty = GL_TRUE;
                break;
            case GL_DIFFUSE:
                glm_vec4_copy((float *)params, material[i]->diffuse);
                glm_vec4_clamp(material[i]->diffuse, 0.0f, 1.0f);
                material[i]->diffuse_dirty = GL_TRUE;
                break;
            case GL_SPECULAR:
                glm_vec4_copy((float *)params, material[i]->specular);
                glm_vec4_clamp(material[i]->specular, 0.0f, 1.0f);
                material[i]->specular_dirty = GL_TRUE;
                break;
            case GL_EMISSION:
                glm_vec4_copy((float *)params, material[i]->emission);
                glm_vec4_clamp(material[i]->emission, 0.0f, 1.0f);
                material[i]->emission_dirty = GL_TRUE;
                break;
            case GL_SHININESS:
                if (params[0] < 0.0f || params[0] > 128.0f) {
                    gliSetError(GL_INVALID_VALUE);
                    return;
                }
                material[i]->shininess = params[0];
                material[i]->shininess_dirty = GL_TRUE;
                break;
            case GL_AMBIENT_AND_DIFFUSE:
                glm_vec4_copy((float *)params, material[i]->ambient);
                glm_vec4_clamp(material[i]->ambient, 0.0f, 1.0f);
                material[i]->ambient_dirty = GL_TRUE;

                glm_vec4_copy((float *)params, material[i]->diffuse);
                glm_vec4_clamp(material[i]->diffuse, 0.0f, 1.0f);
                material[i]->diffuse_dirty = GL_TRUE;
                break;
            default:
                gliSetError(GL_INVALID_ENUM);
                return;
        }
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
        ;
    }
}

void gliLightingFlush(void)
{
    gli_context_t *context = gliGetContext();
    lighting_state_t *lighting = &context->lighting_state;

    if (lighting->light_model_two_side_dirty) {
        uint32_t *pb = pb_begin();
        pb = xgu_set_two_side_light_enable(pb, lighting->light_model_two_side ? true : false);
        pb_end(pb);
    }

    material_t *materials[2] = {&context->lighting_state.material_front, &context->lighting_state.material_back};

    uint32_t *pb = pb_begin();
    for (GLint i = 0; i < 2; i++) {
        material_t *material = materials[i];

        if (material->ambient_dirty || material->emission_dirty || lighting->light_model_ambient_dirty) {
            vec3 ambient;
            ambient[0] = lighting->light_model_ambient[0] * material->ambient[0] + material->emission[0];
            ambient[1] = lighting->light_model_ambient[1] * material->ambient[1] + material->emission[1];
            ambient[2] = lighting->light_model_ambient[2] * material->ambient[2] + material->emission[2];

            // Emission regs doesnt appear to do anything. XDK seems to set it to zero and its baked into ambient
            vec3 emission;
            emission[0] = 0.0f;
            emission[1] = 0.0f;
            emission[2] = 0.0f;

            if (i == 0) {
                pb = xgu_set_scene_ambient_color(pb, ambient[0], ambient[1], ambient[2]);
                pb = xgu_set_material_emission(pb, emission[0], emission[1], emission[2]);
            } else {
                pb = xgu_set_back_scene_ambient_color(pb, ambient[0], ambient[1], ambient[2]);
                pb = xgu_set_back_material_emission(pb, emission[0], emission[1], emission[2]);
            }
        }
    }
    pb_end(pb);

    XguLightMask light_mask[GLI_MAX_LIGHTS];
    for (int i = 0; i < GLI_MAX_LIGHTS; ++i) {
        light_mask[i] = XGU_LMASK_OFF;
    }

    for (int i = 0; i < GLI_MAX_LIGHTS; i++) {
        light_t *light = &lighting->lights[i];
        if (!light->enabled) {
            continue;
        }

        // If w == 0, it's a directional light
        if (light->position[3] == 0) {
            light_mask[i] = XGU_LMASK_INFINITE;
            if (light->position_dirty) {
                light->enabled_dirty = GL_TRUE; // We might need to update the light mask

                // L = normalize(-posEye.xyz)  (eye-space light direction)
                vec3 L;
                glm_vec3_copy((vec3){light->position[0], light->position[1], light->position[2]}, L);
                glm_vec3_normalize(L);

                // H = normalize(L + V) (Blinn–Phong half-vector)
                vec3 V = {0.0f, 0.0f, 1.0f};
                vec3 H;
                glm_vec3_add(L, V, H);
                glm_vec3_normalize(H);

                pb = pb_begin();
                pb = xgu_set_light_local_range(pb, i, FLT_MAX);
                pb = xgu_set_light_infinite_half_vector(pb, i, (XguVec3){H[0], H[1], H[2]});
                pb = xgu_set_light_infinite_direction(pb, i, (XguVec3){L[0], L[1], L[2]});
                pb_end(pb);
            }
        } else {
            light_mask[i] = (light->spot_cutoff == 180.0f) ? XGU_LMASK_LOCAL : XGU_LMASK_SPOT;

            if (light->position_dirty) {
                pb = pb_begin();
                pb = xgu_set_light_local_range(pb, i, FLT_MAX); // FIXME: Calculate range based on attenuation
                pb = xgu_set_light_local_position(
                    pb, i, (XguVec3){light->position[0], light->position[1], light->position[2]});
                pb_end(pb);
            }

            if (light->constant_attenuation_dirty || light->linear_attenuation_dirty ||
                light->quadratic_attenuation_dirty) {
                pb = pb_begin();
                pb = xgu_set_light_local_attenuation(
                    pb, i, light->constant_attenuation, light->linear_attenuation, light->quadratic_attenuation);
                pb_end(pb);
            }

            if (light_mask[i] == XGU_LMASK_SPOT &&
                (light->spot_direction_dirty || light->spot_cutoff_dirty || light->spot_exponent_dirty)) {
                vec3 norm_spot_direction;
                glm_vec3_normalize_to(light->spot_direction, norm_spot_direction);

                xgux_set_light_spot_gl(
                    i,
                    light->spot_exponent,
                    light->spot_cutoff * M_PI / 180.0f,
                    (XguVec3){norm_spot_direction[0], norm_spot_direction[1], norm_spot_direction[2]});

                light->enabled_dirty = GL_TRUE; // We might need to update the light mask
            }
        }

        XguVec3 xgu_v;
        pb = pb_begin();
        for (int j = 0; j < 2; j++) {
            material_t *material = materials[j];
            // Apply light colour and material color
            if (light->diffuse_dirty || material->diffuse_dirty) {
                // Fixme: only multiply by material if material diffuse comes from material (Could be from diffuse
                // vertex) See GL_COLOR_MATERIAL
                xgu_v.r = light->diffuse[0] * material->diffuse[0];
                xgu_v.g = light->diffuse[1] * material->diffuse[1];
                xgu_v.b = light->diffuse[2] * material->diffuse[2];
                if (j == 0) {
                    pb = xgu_set_light_diffuse_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
                } else {
                    pb = xgu_set_back_light_diffuse_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
                }
            }

            // Light ambient and material ambient
            if (light->ambient_dirty || material->ambient_dirty) {
                // Fixme: only multiply by material if material ambient comes from material (Could be from diffuse
                // vertex) See GL_COLOR_MATERIAL
                xgu_v.r = light->ambient[0] * material->ambient[0];
                xgu_v.g = light->ambient[1] * material->ambient[1];
                xgu_v.b = light->ambient[2] * material->ambient[2];
                if (j == 0) {
                    pb = xgu_set_light_ambient_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
                } else {
                    pb = xgu_set_back_light_ambient_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
                }
            }

            // Light specular and material specular
            if (light->specular_dirty || material->specular_dirty) {
                xgu_v.r = light->specular[0] * material->specular[0];
                xgu_v.g = light->specular[1] * material->specular[1];
                xgu_v.b = light->specular[2] * material->specular[2];
                if (j == 0) {
                    pb = xgu_set_light_specular_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
                } else {
                    pb = xgu_set_back_light_specular_color(pb, i, xgu_v.r, xgu_v.g, xgu_v.b);
                }
            }

            // The alpha component of the resulting lighted color is set to the alpha value of the material diffuse
            // reflectance.
            if (material->diffuse_dirty && i == 0) {
                pb = xgu_set_material_alpha(pb, material->diffuse[3]);
            }

            if (material->diffuse_dirty && i == 1) {
                pb = xgu_set_back_material_alpha(pb, material->diffuse[3]);
            }
        }
        pb_end(pb);
    }

    GLboolean light_enable_dirty = GL_FALSE;
    for (int i = 0; i < GLI_MAX_LIGHTS; i++) {
        if (lighting->lights[i].enabled_dirty) {
            light_enable_dirty = GL_TRUE;
            break;
        }
    }
    if (light_enable_dirty) {
        pb = pb_begin();
        pb = xgu_set_light_enable_mask(pb,
                                       light_mask[0],
                                       light_mask[1],
                                       light_mask[2],
                                       light_mask[3],
                                       light_mask[4],
                                       light_mask[5],
                                       light_mask[6],
                                       light_mask[7]);
        pb_end(pb);
    }

    if (materials[0]->shininess_dirty) {
        xgux_set_specular_gl(materials[0]->shininess);
    }
    if (materials[1]->shininess_dirty) {
        xgux_set_back_specular_gl(materials[1]->shininess);
    }

    lighting->light_model_ambient_dirty = GL_FALSE;
    lighting->light_model_two_side_dirty = GL_FALSE;

    for (GLint i = 0; i < GLI_MAX_LIGHTS; i++) {
        light_t *light = &lighting->lights[i];
        light->ambient_dirty = GL_FALSE;
        light->diffuse_dirty = GL_FALSE;
        light->specular_dirty = GL_FALSE;
        light->position_dirty = GL_FALSE;
        light->constant_attenuation_dirty = GL_FALSE;
        light->linear_attenuation_dirty = GL_FALSE;
        light->quadratic_attenuation_dirty = GL_FALSE;
        light->spot_direction_dirty = GL_FALSE;
        light->spot_exponent_dirty = GL_FALSE;
        light->spot_cutoff_dirty = GL_FALSE;
        light->enabled_dirty = GL_FALSE;
    }

    for (int j = 0; j < 2; ++j) {
        materials[j]->ambient_dirty = GL_FALSE;
        materials[j]->diffuse_dirty = GL_FALSE;
        materials[j]->specular_dirty = GL_FALSE;
        materials[j]->emission_dirty = GL_FALSE;
        materials[j]->shininess_dirty = GL_FALSE;
    }
}
