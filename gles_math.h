#ifndef GLES_MATH_H
#define GLES_MATH_H

#include <float.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef float vec3[3];
typedef float vec4[4];
typedef vec4 mat4[4];

extern const vec4 GLM_VEC4_ZERO;
extern const vec4 GLM_VEC4_ONE;
extern const vec4 GLM_VEC4_BLACK;

extern const vec3 GLM_FORWARD;
extern const vec3 GLM_ZUP;

static inline float glm_rad(float deg)
{
    return deg * ((float)M_PI / 180.0f);
}

static inline float glm_clamp(float val, float minVal, float maxVal)
{
    if (val < minVal) {
        return minVal;
    }
    if (val > maxVal) {
        return maxVal;
    }
    return val;
}

// Vector operations
static inline void glm_vec3_copy(const float *v, vec3 dest)
{
    dest[0] = v[0];
    dest[1] = v[1];
    dest[2] = v[2];
}

static inline void glm_vec4_copy(const float *v, vec4 dest)
{
    dest[0] = v[0];
    dest[1] = v[1];
    dest[2] = v[2];
    dest[3] = v[3];
}

static inline void glm_vec4_zero(vec4 v)
{
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
}

static inline void glm_vec3_add(const float *a, const float *b, vec3 dest)
{
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
}

static inline void glm_vec4_clamp(vec4 v, float minVal, float maxVal)
{
    v[0] = glm_clamp(v[0], minVal, maxVal);
    v[1] = glm_clamp(v[1], minVal, maxVal);
    v[2] = glm_clamp(v[2], minVal, maxVal);
    v[3] = glm_clamp(v[3], minVal, maxVal);
}

static inline void glm_vec4_make(const float *src, vec4 dest)
{
    glm_vec4_copy(src, dest);
}

void glm_vec3_normalize(vec3 v);
void glm_vec3_normalize_to(const float *v, vec3 dest);

// Matrix operations
void glm_mat4_identity(mat4 mat);
void glm_mat4_make(const float *src, mat4 dest);
void glm_mat4_copy(const mat4 mat, mat4 dest);
void glm_mat4_mul(const mat4 m1, const mat4 m2, mat4 dest);
void glm_mat4_mulN(const mat4 *matrices[], int count, mat4 dest);
void glm_mat4_inv(const mat4 mat, mat4 dest);
void glm_mat4_transpose(mat4 mat);
void glm_mat4_mulv(const mat4 m, const vec4 v, vec4 dest);
void glm_mat4_mulv3(const mat4 m, const vec3 v, float w, vec3 dest);

// Transformations
void glm_translate(mat4 m, const vec3 v);
void glm_rotate(mat4 m, float angle, const vec3 v);
void glm_scale(mat4 m, const vec3 v);
void glm_frustum(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest);
void glm_ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest);

#endif // GLES_MATH_H