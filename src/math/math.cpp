#include "math.hpp"

#define DEFINE_VECTOR2(NAME, TYPE) \
bool operator==(NAME a, NAME b) \
{ \
    return a.x == b.x && a.y == b.y; \
} \
 \
bool operator!=(NAME a, NAME b) \
{ \
    return a.x != b.x || a.y != b.y; \
} \
 \
NAME operator+(NAME a, NAME b) \
{ \
    return \
    { \
        a.x + b.x, \
        a.y + b.y \
    }; \
} \
 \
NAME operator-(NAME a, NAME b) \
{ \
    return \
    { \
        a.x - b.x, \
        a.y - b.y \
    }; \
} \
NAME operator-(NAME a) \
{ \
    return \
    { \
        -a.x, \
        -a.y \
    }; \
} \
NAME operator*(NAME a, NAME b) \
{ \
    return \
    { \
        a.x * b.x, \
        a.y * b.y \
    }; \
} \
NAME operator/(NAME a, NAME b) \
{ \
    return \
    { \
        a.x / b.x, \
        a.y / b.y \
    }; \
} \
NAME operator*(NAME a, TYPE b) \
{ \
    return \
    { \
        a.x * b, \
        a.y * b \
    }; \
} \

DEFINE_VECTOR2(v2i, int32);
DEFINE_VECTOR2(v2u, uint32);
DEFINE_VECTOR2(v2f, float);

float dot(v2f a, v2f b)
{
    return a.x * b.x + a.y * b.y;
}

float angle(v2f a, v2f b)
{
    return acosf(dot(normalize(a), normalize(b)));
}

float magn2(v2f a)
{
    return dot(a, a);
}

float magn(v2f a)
{
    return sqrtf(magn2(a));
}

v2f normalize(v2f a)
{
    float len = magn(a);
    if(len < EPSILON_FLOAT)
    {
        return {0, 0};
    }
    return a * (1.f / len);
}

bool operator==(v3f a, v3f b)
{
    return a.x == b.x
        && a.y == b.y
        && a.z == b.z;
}

bool operator!=(v3f a, v3f b)
{
    return a.x != b.x
        || a.y != b.y
        || a.z != b.z;
}

v3f operator+(v3f a, v3f b)
{
    return
    {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

v3f operator-(v3f a, v3f b)
{
    return
    {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

v3f operator-(v3f a)
{
    return
    {
        -a.x,
        -a.y,
        -a.z
    };
}

v3f operator*(v3f a, v3f b)
{
    return
    {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
}

v3f operator*(v3f a, float b)
{
    return
    {
        a.x * b,
        a.y * b,
        a.z * b
    };
}

float dot(v3f a, v3f b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

v3f cross(v3f a, v3f b)
{
    return 
    {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

float magn2(v3f v)
{
    return dot(v, v);
}

float magn(v3f v)
{
    return sqrtf(magn2(v));
}

v3f normalize(v3f v)
{
    float len = magn(v);
    if(len < EPSILON_FLOAT)
    {
        return {0, 0};
    }
    return v * (1.f / len);
}

bool operator==(v4f a, v4f b)
{
    return a.x == b.x
        && a.y == b.y
        && a.z == b.z
        && a.w == b.w;
}

bool operator!=(v4f a, v4f b)
{
    return a.x != b.x
        || a.y != b.y
        || a.z != b.z
        || a.w != b.w;
}

v4f operator+(v4f a, v4f b)
{
    return
    {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w
    };
}

v4f operator-(v4f a, v4f b)
{
    return
    {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w
    };
}

v4f operator-(v4f a)
{
    return
    {
        -a.x,
        -a.y,
        -a.z,
        -a.w
    };
}

v4f operator*(v4f a, v4f b)
{
    return
    {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
        a.w * b.w
    };
}

v4f operator*(v4f a, float b)
{
    return
    {
        a.x * b,
        a.y * b,
        a.z * b,
        a.w * b
    };
}

v4f to4f(v3f v, float w)
{
    return
    {
        v.x, v.y, v.z, w
    };
}

v3f to3f(v4f v)
{
    return
    {
        v.x, v.y, v.z
    };
}

float dot(v4f a, v4f b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float magn2(v4f v)
{
    return dot(v, v);
}

float magn(v4f v)
{
    return sqrtf(magn2(v));
}

v4f normalize(v4f v)
{
    float len = magn(v);
    if(len < EPSILON_FLOAT)
    {
        return {0, 0};
    }
    return v * (1.f / len);
}

quat quatAngleAxis(float angle, v3f axis)
{
    float s = sin(angle / 2.f);
    float c = cos(angle / 2.f);
    quat q =
    {
        s * axis.x,
        s * axis.y,
        s * axis.z,
        c
    };
    return normalize(q);    // Quaternions always normalized for rotations
}

quat quatConj(quat q)
{
    return
    {
        -q.x, -q.y, -q.z, q.w
    };
}

quat quatMul(quat a, quat b)
{
    return
    {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,  // i
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,  // j
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,  // k
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z   // 1
    };
}

v3f rotate(v3f p, quat q)
{
    quat pure = to4f(p, 0);
    quat result = quatMul(quatMul(q, pure), quatConj(q));
    return to3f(result);
}

v3f rotate(v3f p, float angle, v3f axis)
{
    return rotate(p, quatAngleAxis(angle, axis));
}

plane getPlane(v3f p, v3f n)
{
    plane result;
    n = normalize(n);
    result = to4f(n, 0);
    result.w = -dot(n, p);
    return result;
}

plane getPlane(v3f p, v3f a, v3f b)
{
    a = normalize(a);
    b = normalize(b);
    v3f n = normalize(cross(a, b));
    return getPlane(p, n);
}

m4f operator+(m4f a, m4f b)
{
    return 
    {
        a.m00 + b.m00,
        a.m10 + b.m10,
        a.m20 + b.m20,
        a.m30 + b.m30,

        a.m01 + b.m01,
        a.m11 + b.m11,
        a.m21 + b.m21,
        a.m31 + b.m31,

        a.m02 + b.m02,
        a.m12 + b.m12,
        a.m22 + b.m22,
        a.m32 + b.m32,

        a.m03 + b.m03,
        a.m13 + b.m13,
        a.m23 + b.m23,
        a.m33 + b.m33,
    };
}

m4f operator-(m4f a, m4f b)
{
    return 
    {
        a.m00 - b.m00,
        a.m10 - b.m10,
        a.m20 - b.m20,
        a.m30 - b.m30,

        a.m01 - b.m01,
        a.m11 - b.m11,
        a.m21 - b.m21,
        a.m31 - b.m31,

        a.m02 - b.m02,
        a.m12 - b.m12,
        a.m22 - b.m22,
        a.m32 - b.m32,

        a.m03 - b.m03,
        a.m13 - b.m13,
        a.m23 - b.m23,
        a.m33 - b.m33,
    };
}

m4f matMul(m4f m, float a)
{
    return
    {
        m.m00 * a,
        m.m10 * a,
        m.m20 * a,
        m.m30 * a,

        m.m01 * a,
        m.m11 * a,
        m.m21 * a,
        m.m31 * a,

        m.m02 * a,
        m.m12 * a,
        m.m22 * a,
        m.m32 * a,

        m.m03 * a,
        m.m13 * a,
        m.m23 * a,
        m.m33 * a,
    };
}

v4f matMul(m4f m, v4f v)
{
    return
    {
        m.m00 * v.x + m.m01 * v.y + m.m02 * v.z + m.m03 * v.w,
        m.m10 * v.x + m.m11 * v.y + m.m12 * v.z + m.m13 * v.w,
        m.m20 * v.x + m.m21 * v.y + m.m22 * v.z + m.m23 * v.w,
        m.m30 * v.x + m.m31 * v.y + m.m32 * v.z + m.m33 * v.w,
    };
}

m4f matMul(m4f a, m4f b)
{
    return
    {
        // Col 0
        a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20 + a.m03 * b.m30,
        a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20 + a.m13 * b.m30,
        a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20 + a.m23 * b.m30,
        a.m30 * b.m00 + a.m31 * b.m10 + a.m32 * b.m20 + a.m33 * b.m30,

        // Col 1
        a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21 + a.m03 * b.m31,
        a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31,
        a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31,
        a.m30 * b.m01 + a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31,

        // Col 2
        a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22 + a.m03 * b.m32,
        a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32,
        a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32,
        a.m30 * b.m02 + a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32,

        // Col 3
        a.m00 * b.m03 + a.m01 * b.m13 + a.m02 * b.m23 + a.m03 * b.m33,
        a.m10 * b.m03 + a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33,
        a.m20 * b.m03 + a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33,
        a.m30 * b.m03 + a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33,
    };
}

m4f identity()
{
    return
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
}

m4f transpose(m4f m)
{
    return
    {
        m.m00, m.m01, m.m02, m.m03,
        m.m10, m.m11, m.m12, m.m13,
        m.m20, m.m21, m.m22, m.m23,
        m.m30, m.m31, m.m32, m.m33,
    };
}

m4f inverse(m4f m)
{
    float A2323 = m.m22 * m.m33 - m.m23 * m.m32;
    float A1323 = m.m21 * m.m33 - m.m23 * m.m31;
    float A1223 = m.m21 * m.m32 - m.m22 * m.m31;
    float A0323 = m.m20 * m.m33 - m.m23 * m.m30;
    float A0223 = m.m20 * m.m32 - m.m22 * m.m30;
    float A0123 = m.m20 * m.m31 - m.m21 * m.m30;
    float A2313 = m.m12 * m.m33 - m.m13 * m.m32;
    float A1313 = m.m11 * m.m33 - m.m13 * m.m31;
    float A1213 = m.m11 * m.m32 - m.m12 * m.m31;
    float A2312 = m.m12 * m.m23 - m.m13 * m.m22;
    float A1312 = m.m11 * m.m23 - m.m13 * m.m21;
    float A1212 = m.m11 * m.m22 - m.m12 * m.m21;
    float A0313 = m.m10 * m.m33 - m.m13 * m.m30;
    float A0213 = m.m10 * m.m32 - m.m12 * m.m30;
    float A0312 = m.m10 * m.m23 - m.m13 * m.m20;
    float A0212 = m.m10 * m.m22 - m.m12 * m.m20;
    float A0113 = m.m10 * m.m31 - m.m11 * m.m30;
    float A0112 = m.m10 * m.m21 - m.m11 * m.m20;

    float det = m.m00 * (m.m11 * A2323 - m.m12 * A1323 + m.m13 * A1223)
    - m.m01 * (m.m10 * A2323 - m.m12 * A0323 + m.m13 * A0223)
    + m.m02 * (m.m10 * A1323 - m.m11 * A0323 + m.m13 * A0123)
    - m.m03 * (m.m10 * A1223 - m.m11 * A0223 + m.m12 * A0123);
    det = 1 / det;

    return
    {
        det *  (m.m11 * A2323 - m.m12 * A1323 + m.m13 * A1223), // m00
        det * -(m.m10 * A2323 - m.m12 * A0323 + m.m13 * A0223), // m10
        det *  (m.m10 * A1323 - m.m11 * A0323 + m.m13 * A0123), // m20
        det * -(m.m10 * A1223 - m.m11 * A0223 + m.m12 * A0123), // m30
        det * -(m.m01 * A2323 - m.m02 * A1323 + m.m03 * A1223), // m01
        det *  (m.m00 * A2323 - m.m02 * A0323 + m.m03 * A0223), // m11
        det * -(m.m00 * A1323 - m.m01 * A0323 + m.m03 * A0123), // m21
        det *  (m.m00 * A1223 - m.m01 * A0223 + m.m02 * A0123), // m31
        det *  (m.m01 * A2313 - m.m02 * A1313 + m.m03 * A1213), // m02
        det * -(m.m00 * A2313 - m.m02 * A0313 + m.m03 * A0213), // m12
        det *  (m.m00 * A1313 - m.m01 * A0313 + m.m03 * A0113), // m22
        det * -(m.m00 * A1213 - m.m01 * A0213 + m.m02 * A0113), // m32
        det * -(m.m01 * A2312 - m.m02 * A1312 + m.m03 * A1212), // m03
        det *  (m.m00 * A2312 - m.m02 * A0312 + m.m03 * A0212), // m13
        det * -(m.m00 * A1312 - m.m01 * A0312 + m.m03 * A0112), // m23
        det *  (m.m00 * A1212 - m.m01 * A0212 + m.m02 * A0112), // m33
    };
}

bool eqf(float a, float b)
{
    return fabsf(a - b) < 1e-5f;
}

m4f translation(v3f pos)
{
    return
    {
        1,      0,      0,      0,
        0,      1,      0,      0,
        0,      0,      1,      0,
        pos.x,  pos.y,  pos.z,  1
    };
}

m4f rotation(quat q)
{
    // https://www.songho.ca/opengl/gl_quaternion.html
    m4f result = identity();
    q = normalize(q); // Making sure
    result.m00 = 1 - (2 * q.y * q.y) - (2 * q.z * q.z);
    result.m01 = (2 * q.x * q.y) - (2 * q.w * q.z);
    result.m02 = (2 * q.x * q.z) + (2 * q.w * q.y);
    result.m10 = (2 * q.x * q.y) + (2 * q.w * q.z);
    result.m11 = 1 - (2 * q.x * q.x) - (2 * q.z * q.z);
    result.m12 = (2 * q.y * q.z) - (2 * q.w * q.x);
    result.m20 = (2 * q.x * q.z) - (2 * q.w * q.y);
    result.m21 = (2 * q.y * q.z) + (2 * q.w * q.x);
    result.m22 = 1 - (2 * q.x * q.x) - (2 * q.y * q.y);

    return result;
}

m4f rotation(float angle, v3f axis)
{
    return rotation(quatAngleAxis(angle, axis));
}

m4f scale(float scale)
{
    return
    {
        scale, 0, 0, 0,
        0, scale, 0, 0,
        0, 0, scale, 0,
        0, 0, 0, 1,
    };
}

m4f scale(v3f scale)
{
    return
    {
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        0, 0, 0, 1,
    };
}

m4f viewRH(v3f x, v3f y, v3f z, v3f center)
{
    //https://learnopengl.com/Getting-started/Camera
    // View matrix is inverse of composed camera coordinate space matrix and translation matrix, set directly here.
    m4f result = identity();
    result.m00 = x.x;
    result.m01 = x.y;
    result.m02 = x.z;

    result.m10 = y.x;
    result.m11 = y.y;
    result.m12 = y.z;

    result.m20 = z.x;
    result.m21 = z.y;
    result.m22 = z.z;

    result.m03 = -dot(x, center);
    result.m13 = -dot(y, center);
    result.m23 = -dot(z, center);
    return result;
}

m4f orthoRH(float l, float r, float t, float b, float n, float f)
{
    // Reference: GLM orthoRH_ZO
    m4f result = {};
    result.m00 = 2.f / (r - l);
    result.m11 = 2.f / (t - b);
#if 1
    // TODO_DW: MULTIPLATFORM Vulkan
    result.m11 = -result.m11;
#endif
    result.m22 = -1.f / (f - n);

    result.m03 = - (r + l) / (r - l);
    result.m13 = - (t + b) / (t - b);
    result.m23 = - n / (f - n);
    result.m33 = 1;

    return result;
}

m4f perspectiveRH(float fovY, float aspect, float zNear, float zFar)
{
    // https://www.vincentparizet.com/blog/posts/vulkan_perspective_matrix/
    // Perspective matrix for reverse depth, [0, 1] depth range
    // and Vulkan clip space (Y points down).

    // TODO_DW: VULKAN
    m4f result = {};
    float y = 1.f / tanf(fovY/2.f);
    float x = y / aspect;
    result.m00 = x;
    result.m11 = -y;
    result.m22 = zNear / (zFar - zNear);
    result.m23 = (zFar * zNear) / (zFar - zNear);
    result.m32 = -1;
    
    return result;
}

v3f clipToWorld(v3f p, m4f invView, m4f invProj)
{
    v4f P = to4f(p, 1.f);
    // To view space (multiply by inverse projection then revert perspective divide).
    P = matMul(invProj, P);
    P = P * (1.f / P.w);
    // To world space (multiply by inverse view).
    P = matMul(invView, P);
    return to3f(P);
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * CLAMP(t, 0, 1);
}

v2f lerp(v2f a, v2f b, float t)
{
    return
    {
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t)
    };
}

v3f lerp(v3f a, v3f b, float t)
{
    return
    {
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t)
    };
}

v3f fromPolar(float radius, float theta, float phi)
{
    return
    {
        radius * sinf(theta) * cosf(phi),
        radius * sinf(theta) * sinf(phi),
        radius * cosf(theta)
    };
}
