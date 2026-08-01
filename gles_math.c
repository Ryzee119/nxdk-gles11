#include "gles_private.h"

const vec4 GLM_VEC4_ZERO = {0.0f, 0.0f, 0.0f, 0.0f};
const vec4 GLM_VEC4_ONE = {1.0f, 1.0f, 1.0f, 1.0f};
const vec4 GLM_VEC4_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
const vec3 GLM_FORWARD = {0.0f, 0.0f, -1.0f};
const vec3 GLM_ZUP = {0.0f, 0.0f, 1.0f};

void glm_vec3_normalize(vec3 v)
{
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1e-6f) {
        float inv = 1.0f / len;
        v[0] *= inv;
        v[1] *= inv;
        v[2] *= inv;
    }
}

void glm_vec3_normalize_to(const float *v, vec3 dest)
{
    glm_vec3_copy(v, dest);
    glm_vec3_normalize(dest);
}

void glm_mat4_identity(mat4 mat)
{
    mat[0][0] = 1.0f;
    mat[0][1] = 0.0f;
    mat[0][2] = 0.0f;
    mat[0][3] = 0.0f;
    mat[1][0] = 0.0f;
    mat[1][1] = 1.0f;
    mat[1][2] = 0.0f;
    mat[1][3] = 0.0f;
    mat[2][0] = 0.0f;
    mat[2][1] = 0.0f;
    mat[2][2] = 1.0f;
    mat[2][3] = 0.0f;
    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = 0.0f;
    mat[3][3] = 1.0f;
}

void glm_mat4_make(const float *src, mat4 dest)
{
    if (src != (const float *)dest) {
        gli_memcpy(dest, src, 16 * sizeof(float));
    }
}

void glm_mat4_copy(const mat4 mat, mat4 dest)
{
    if (mat != dest) {
        gli_memcpy(dest, mat, 16 * sizeof(float));
    }
}

void glm_mat4_mul(const mat4 m1, const mat4 m2, mat4 dest)
{
    mat4 res;
    for (int i = 0; i < 4; i++) {     // col of m2
        for (int j = 0; j < 4; j++) { // row of m1
            res[i][j] = m1[0][j] * m2[i][0] + m1[1][j] * m2[i][1] + m1[2][j] * m2[i][2] + m1[3][j] * m2[i][3];
        }
    }
    glm_mat4_copy(res, dest);
}

void glm_mat4_mulN(const mat4 *matrices[], int count, mat4 dest)
{
    if (count <= 0) {
        glm_mat4_identity(dest);
        return;
    }
    mat4 temp;
    glm_mat4_copy(*(matrices[0]), temp);
    for (int i = 1; i < count; i++) {
        glm_mat4_mul(temp, *(matrices[i]), temp);
    }
    glm_mat4_copy(temp, dest);
}

void glm_mat4_inv(const mat4 mat, mat4 dest)
{
    // Fast path for affine transformations (bottom row is 0 0 0 1)
    if (mat[0][3] == 0.0f && mat[1][3] == 0.0f && mat[2][3] == 0.0f && mat[3][3] == 1.0f) {
        float a = mat[0][0], b = mat[0][1], c = mat[0][2], e = mat[1][0], f = mat[1][1], g = mat[1][2], i = mat[2][0],
              j = mat[2][1], k = mat[2][2], m = mat[3][0], n = mat[3][1], o = mat[3][2];

        float co00 = f * k - g * j;
        float co10 = g * i - e * k;
        float co20 = e * j - f * i;

        float det = a * co00 + b * co10 + c * co20;
        if (fabsf(det) < 1e-6f) {
            glm_mat4_identity(dest);
            return;
        }

        float idt = 1.0f / det;

        dest[0][0] = co00 * idt;
        dest[0][1] = (c * j - b * k) * idt;
        dest[0][2] = (b * g - c * f) * idt;
        dest[0][3] = 0.0f;

        dest[1][0] = co10 * idt;
        dest[1][1] = (a * k - c * i) * idt;
        dest[1][2] = (c * e - a * g) * idt;
        dest[1][3] = 0.0f;

        dest[2][0] = co20 * idt;
        dest[2][1] = (b * i - a * j) * idt;
        dest[2][2] = (a * f - b * e) * idt;
        dest[2][3] = 0.0f;

        dest[3][0] = -(dest[0][0] * m + dest[1][0] * n + dest[2][0] * o);
        dest[3][1] = -(dest[0][1] * m + dest[1][1] * n + dest[2][1] * o);
        dest[3][2] = -(dest[0][2] * m + dest[1][2] * n + dest[2][2] * o);
        dest[3][3] = 1.0f;
        return;
    }

    float a = mat[0][0], b = mat[0][1], c = mat[0][2], d = mat[0][3], e = mat[1][0], f = mat[1][1], g = mat[1][2],
          h = mat[1][3], i = mat[2][0], j = mat[2][1], k = mat[2][2], l = mat[2][3], m = mat[3][0], n = mat[3][1],
          o = mat[3][2], p = mat[3][3];

    float c1 = k * p - l * o, c2 = c * h - d * g, c3 = i * p - l * m, c4 = a * h - d * e, c5 = j * p - l * n,
          c6 = b * h - d * f, c7 = i * n - j * m, c8 = a * f - b * e, c9 = j * o - k * n, c10 = b * g - c * f,
          c11 = i * o - k * m, c12 = a * g - c * e;

    float det = c8 * c1 + c4 * c9 + c10 * c3 + c2 * c7 - c12 * c5 - c6 * c11;
    if (fabsf(det) < 1e-6f) {
        // Fallback to identity matrix if matrix is singular (non-invertible)
        glm_mat4_identity(dest);
        return;
    }

    float idt = 1.0f / det;
    float ndt = -idt;

    dest[0][0] = (f * c1 - g * c5 + h * c9) * idt;
    dest[0][1] = (b * c1 - c * c5 + d * c9) * ndt;
    dest[0][2] = (n * c2 - o * c6 + p * c10) * idt;
    dest[0][3] = (j * c2 - k * c6 + l * c10) * ndt;

    dest[1][0] = (e * c1 - g * c3 + h * c11) * ndt;
    dest[1][1] = (a * c1 - c * c3 + d * c11) * idt;
    dest[1][2] = (m * c2 - o * c4 + p * c12) * ndt;
    dest[1][3] = (i * c2 - k * c4 + l * c12) * idt;

    dest[2][0] = (e * c5 - f * c3 + h * c7) * idt;
    dest[2][1] = (a * c5 - b * c3 + d * c7) * ndt;
    dest[2][2] = (m * c6 - n * c4 + p * c8) * idt;
    dest[2][3] = (i * c6 - j * c4 + l * c8) * ndt;

    dest[3][0] = (e * c9 - f * c11 + g * c7) * ndt;
    dest[3][1] = (a * c9 - b * c11 + c * c7) * idt;
    dest[3][2] = (m * c10 - n * c12 + o * c8) * ndt;
    dest[3][3] = (i * c10 - j * c12 + k * c8) * idt;
}

void glm_mat4_transpose(mat4 mat)
{
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            float temp = mat[i][j];
            mat[i][j] = mat[j][i];
            mat[j][i] = temp;
        }
    }
}

void glm_mat4_mulv(const mat4 m, const vec4 v, vec4 dest)
{
    vec4 res;
    res[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3];
    res[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3];
    res[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3];
    res[3] = m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3];
    dest[0] = res[0];
    dest[1] = res[1];
    dest[2] = res[2];
    dest[3] = res[3];
}

void glm_mat4_mulv3(const mat4 m, const vec3 v, float w, vec3 dest)
{
    vec3 res;
    res[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * w;
    res[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * w;
    res[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * w;
    dest[0] = res[0];
    dest[1] = res[1];
    dest[2] = res[2];
}

void glm_translate(mat4 m, const vec3 v)
{
    m[3][0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0];
    m[3][1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1];
    m[3][2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2];
    m[3][3] = m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3];
}

void glm_rotate(mat4 m, float angle, const vec3 v)
{
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len <= 1e-6f) {
        return; // Ignore rotation if axis vector length is ~0
    }

    float inv = 1.0f / len;
    vec3 axis = {v[0] * inv, v[1] * inv, v[2] * inv};

    float c = cosf(angle);
    float s = sinf(angle);
    float temp[3] = {(1.0f - c) * axis[0], (1.0f - c) * axis[1], (1.0f - c) * axis[2]};

    mat4 rot;
    rot[0][0] = c + temp[0] * axis[0];
    rot[0][1] = temp[0] * axis[1] + s * axis[2];
    rot[0][2] = temp[0] * axis[2] - s * axis[1];
    rot[0][3] = 0.0f;

    rot[1][0] = temp[1] * axis[0] - s * axis[2];
    rot[1][1] = c + temp[1] * axis[1];
    rot[1][2] = temp[1] * axis[2] + s * axis[0];
    rot[1][3] = 0.0f;

    rot[2][0] = temp[2] * axis[0] + s * axis[1];
    rot[2][1] = temp[2] * axis[1] - s * axis[0];
    rot[2][2] = c + temp[2] * axis[2];
    rot[2][3] = 0.0f;

    rot[3][0] = 0.0f;
    rot[3][1] = 0.0f;
    rot[3][2] = 0.0f;
    rot[3][3] = 1.0f;

    mat4 res;
    glm_mat4_mul(m, rot, res);
    glm_mat4_copy(res, m);
}

void glm_scale(mat4 m, const vec3 v)
{
    m[0][0] *= v[0];
    m[0][1] *= v[0];
    m[0][2] *= v[0];
    m[0][3] *= v[0];
    m[1][0] *= v[1];
    m[1][1] *= v[1];
    m[1][2] *= v[1];
    m[1][3] *= v[1];
    m[2][0] *= v[2];
    m[2][1] *= v[2];
    m[2][2] *= v[2];
    m[2][3] *= v[2];
}

void glm_frustum(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest)
{
    float rl = right - left;
    float tb = top - bottom;
    float fn = farZ - nearZ;

    glm_mat4_identity(dest);
    if (fabsf(rl) < 1e-6f || fabsf(tb) < 1e-6f || fabsf(fn) < 1e-6f) {
        return;
    }

    dest[0][0] = (2.0f * nearZ) / rl;
    dest[1][1] = (2.0f * nearZ) / tb;
    dest[2][0] = (right + left) / rl;
    dest[2][1] = (top + bottom) / tb;
    dest[2][2] = -(farZ + nearZ) / fn;
    dest[2][3] = -1.0f;
    dest[3][2] = -(2.0f * farZ * nearZ) / fn;
    dest[3][3] = 0.0f;
}

void glm_ortho(float left, float right, float bottom, float top, float nearZ, float farZ, mat4 dest)
{
    float rl = right - left;
    float tb = top - bottom;
    float fn = farZ - nearZ;

    glm_mat4_identity(dest);
    if (fabsf(rl) < 1e-6f || fabsf(tb) < 1e-6f || fabsf(fn) < 1e-6f) {
        return;
    }

    dest[0][0] = 2.0f / rl;
    dest[1][1] = 2.0f / tb;
    dest[2][2] = -2.0f / fn;
    dest[3][0] = -(right + left) / rl;
    dest[3][1] = -(top + bottom) / tb;
    dest[3][2] = -(farZ + nearZ) / fn;
}
