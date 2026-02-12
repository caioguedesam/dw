#pragma once
#include "../core/base.hpp"
#include "math.h"

// Constants
#define PI 3.14159265358979323846

#define TO_DEG(X) (X * (180.0 / PI))
#define TO_RAD(X) (X * (PI / 180.0))

// Vectors
#define DECLARE_VECTOR2(NAME, TYPE) \
struct NAME                         \
{                                   \
    union                           \
    {                               \
        struct                      \
        {                           \
            TYPE x = 0;             \
            TYPE y = 0;             \
        };                          \
        TYPE mData[2];              \
    };                              \
};                                  \
bool operator==(NAME a, NAME b);    \
bool operator!=(NAME a, NAME b);    \
NAME operator+(NAME a, NAME b);     \
NAME operator-(NAME a, NAME b);     \
NAME operator-(NAME a);             \
NAME operator*(NAME a, NAME b);     \
NAME operator/(NAME a, NAME b);     \
NAME operator*(NAME a, TYPE b);

DECLARE_VECTOR2(v2i, int32);
DECLARE_VECTOR2(v2u, uint32);
DECLARE_VECTOR2(v2f, float);

float   dot(v2f a, v2f b);
float   angle(v2f a, v2f b);
float   magn2(v2f a);
float   magn(v2f a);
v2f     normalize(v2f a);

struct v3f
{
    union
    {
        struct
        {
            union { float x = 0; float r; };
            union { float y = 0; float g; };
            union { float z = 0; float b; };
        };
        float mData[3];
    };
};

bool operator==(v3f a, v3f b);
bool operator!=(v3f a, v3f b);
v3f  operator+ (v3f a, v3f b);
v3f  operator- (v3f a, v3f b);
v3f  operator- (v3f a);
v3f  operator* (v3f a, v3f b);
v3f  operator* (v3f a, float b);
v3f  operator* (float a, v3f b);

v3f to3f(float* f);

float   dot       (v3f a, v3f b);
v3f     cross     (v3f a, v3f b);
float   magn2     (v3f v);
float   magn      (v3f v);
v3f     normalize (v3f v);

struct v4f
{
    union
    {
        struct
        {
            union { float x = 0; float r; };
            union { float y = 0; float g; };
            union { float z = 0; float b; };
            union { float w = 0; float a; };
        };
        float mData[4];
    };
};

bool operator==(v4f a, v4f b);
bool operator!=(v4f a, v4f b);
v4f  operator+ (v4f a, v4f b);
v4f  operator- (v4f a, v4f b);
v4f  operator* (v4f a, v4f b);
v4f  operator* (v4f a, float b);
v4f  operator* (float a, v4f b);

v4f to4f(v3f v, float w);
v3f to3f(v4f v);
v4f to4f(float* f);

float   dot       (v4f a, v4f b);
float   magn2     (v4f v);
float   magn      (v4f v);
v4f     normalize (v4f v);

// Quaternions (v4f -> basis coefficients)
typedef v4f quat;
quat quatAngleAxis(float angle, v3f axis);
quat quatConj(quat q);
quat quatMul(quat a, quat b);
v3f rotate(v3f p, quat q);
v3f rotate(v3f p, float angle, v3f axis);

// Planes (v4f -> coefficients of plane equation)
typedef v4f plane;
plane getPlane(v3f p, v3f n);
plane getPlane(v3f p0, v3f p1, v3f p2);
float distanceToPlane(v3f o, plane p);

// Matrices
struct m4f
{
    union
    {
        struct
        {
            // (i, j) -> i = row, j = column
            // Data laid out in column-major order.
            float m00 = 0; float m10 = 0; float m20 = 0; float m30 = 0;
            float m01 = 0; float m11 = 0; float m21 = 0; float m31 = 0;
            float m02 = 0; float m12 = 0; float m22 = 0; float m32 = 0;
            float m03 = 0; float m13 = 0; float m23 = 0; float m33 = 0;
        };
        float mData[16];
    };
};
m4f  operator+ (m4f a, m4f b);
m4f  operator- (m4f a, m4f b);

m4f matMul(m4f m, float a);
v4f matMul(m4f m, v4f v);
m4f matMul(m4f a, m4f b);

m4f identity();
m4f transpose(m4f m);
m4f inverse(m4f m);

// Transform
m4f translation(v3f pos);
m4f rotation(quat q);
m4f rotation(float angle, v3f axis);
m4f scale(float scale);
m4f scale(v3f scale);

m4f viewRH(v3f x, v3f y, v3f z, v3f center);
m4f orthoRH(float l, float r, float t, float b, float n, float f);
m4f perspectiveRH(float fovY, float aspect, float zNear, float zFar);

v3f clipToWorld(v3f p, m4f invView, m4f invProj);

// Misc
bool    eqf(float a, float b);
float   lerp(float a, float b, float t);
v2f     lerp(v2f a, v2f b, float t);
v3f     lerp(v3f a, v3f b, float t);
v3f     fromPolar(float radius, float theta, float phi);
