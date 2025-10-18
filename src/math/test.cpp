#include "math.hpp"
#include "volumes.hpp"
#include "../core/debug.hpp"

bool testVector()
{
    // v2f
    {
        v2f a = {1, 2};
        v2f b = {3, 4};
        v2f sum = a + b;
        ASSERT(sum.x == 4 && sum.y == 6);

        v2f diff = b - a;
        ASSERT(diff.x == 2 && diff.y == 2);

        ASSERT(dot(a, b) == 11);
        ASSERT(eqf(magn(v2f{3, 4}), 5.0f));
        ASSERT(eqf(magn(normalize(v2f{5, 0})), 1.0f));
    }

    // v3f
    {
        v3f a = {1, 0, 0};
        v3f b = {0, 1, 0};

        ASSERT(dot(a, b) == 0);
        v3f c = cross(a, b);
        ASSERT(c.x == 0 && c.y == 0 && c.z == 1);

        v3f v = {3, 4, 0};
        ASSERT(eqf(magn(v), 5.0f));
        ASSERT(eqf(magn(normalize(v)), 1.0f));
    }

    // v4f
    {
        v4f a = {1, 2, 3, 4};
        v4f b = {2, 0, 2, 0};
        v4f r = a + b;
        ASSERT(r.x == 3 && r.y == 2 && r.z == 5 && r.w == 4);
        ASSERT(eqf(dot(a, b), 8.0f));
        ASSERT(eqf(magn(normalize(a)), 1.0f));

        v3f c = to3f(a);
        ASSERT(c.x == 1 && c.y == 2 && c.z == 3);
    }

    return true;
}

bool testQuat()
{
    v3f axis = {0, 1, 0};
    quat q = quatAngleAxis(PI / 2, axis);
    v3f v = {1, 0, 0};
    v3f r = rotate(v, q);
    ASSERT(eqf(r.x, 0.0f));
    ASSERT(eqf(r.z, -1.0f));

    return true;
}

bool testPlane()
{
    v3f p = {0, 0, 1};
    v3f n = {0, 0, 1};
    plane pl = getPlane(p, n);
    ASSERT(eqf(pl.x, 0));
    ASSERT(eqf(pl.y, 0));
    ASSERT(eqf(pl.z, 1));
    ASSERT(eqf(pl.w, -1));

    return true;
}

bool testMatrix()
{
    m4f I = identity();
    ASSERT(I.m00 == 1 && I.m11 == 1 && I.m22 == 1 && I.m33 == 1);

    v4f v = {1, 2, 3, 1};
    v4f out = matMul(I, v);
    ASSERT(out.x == 1 && out.y == 2 && out.z == 3 && out.w == 1);

    m4f M = I;
    M.m03 = 5;
    M.m13 = 2;
    M.m23 = -3;
    m4f inv = inverse(M);
    v4f t = matMul(M, matMul(inv, v));
    ASSERT(eqf(t.x, v.x));
    ASSERT(eqf(t.y, v.y));
    ASSERT(eqf(t.z, v.z));
    ASSERT(eqf(t.w, v.w));

    return true;
}

bool testTransform()
{
    // Translation
    v3f pos = {5, -3, 2};
    m4f T = translation(pos);
    v4f p = {1, 1, 1, 1};
    v4f r = matMul(T, p);
    ASSERT(eqf(r.x, 6));
    ASSERT(eqf(r.y, -2));
    ASSERT(eqf(r.z, 3));

    // Scale (uniform)
    m4f S = scale(2.0f);
    v4f s = matMul(S, v4f{1, 2, 3, 1});
    ASSERT(eqf(s.x, 2));
    ASSERT(eqf(s.y, 4));
    ASSERT(eqf(s.z, 6));

    // Scale (non-uniform)
    m4f Sn = scale(v3f{2, 3, 4});
    v4f sn = matMul(Sn, v4f{1, 1, 1, 1});
    ASSERT(eqf(sn.x, 2));
    ASSERT(eqf(sn.y, 3));
    ASSERT(eqf(sn.z, 4));

    // Rotation (around Y axis)
    m4f R = rotation(PI / 2, v3f{0, 1, 0});
    v4f v = {1, 0, 0, 1};
    v4f rot = matMul(R, v);
    ASSERT(eqf(rot.x, 0.0f));
    ASSERT(eqf(rot.z, -1.0f));

    return true;
}

bool testViewProjection()
{
    v3f x = {1, 0, 0};
    v3f y = {0, 1, 0};
    v3f z = {0, 0, 1};
    v3f c = {0, 0, 0};

    m4f V = viewRH(x, y, z, c);
    ASSERT(eqf(V.m00, 1.0f));
    ASSERT(eqf(V.m11, 1.0f));
    ASSERT(eqf(V.m22, 1.0f));

    m4f O = orthoRH(-1, 1, 1, -1, 0.1f, 100.0f);
    ASSERT(eqf(O.m00, 1.0f));
#if 1
    // TODO_DW: VULKAN
    ASSERT(eqf(O.m11, -1.0f));
#endif

    // Perspective RH (reverse depth, Vulkan clip space)
    {
        float fovY   = PI / 2.0f;  // 90 degrees
        float aspect = 1.0f;
        float zNear  = 0.1f;
        float zFar   = 100.0f;

        m4f P = perspectiveRH(fovY, aspect, zNear, zFar);

        ASSERT(eqf(P.m00, 1.0f));
        ASSERT(eqf(P.m11, -1.0f));
#if 1
        // TODO_DW: VULKAN
        ASSERT(eqf(P.m22, 0.001001f));
        ASSERT(eqf(P.m23, 0.100100f));
        ASSERT(eqf(P.m32, -1.0f));
#endif
        ASSERT(eqf(P.m33, 0.0f));
    }

    return true;
}

bool testAABB()
{
    // Basic AABB from (0,0,0) to (1,2,3)
    AABB box;
    box.min = {0, 0, 0};
    box.max = {1, 2, 3};

    // getSize
    v3f size = getSize(box);
    ASSERT(eqf(size.x, 1.0f));
    ASSERT(eqf(size.y, 2.0f));
    ASSERT(eqf(size.z, 3.0f));

    // getCenter
    v3f center = getCenter(box);
    ASSERT(eqf(center.x, 0.5f));
    ASSERT(eqf(center.y, 1.0f));
    ASSERT(eqf(center.z, 1.5f));

    // Transform test: translate by (2,3,4)
    m4f T = translation({2, 3, 4});
    AABB moved = transform(box, T);
    ASSERT(eqf(moved.min.x, 2.0f));
    ASSERT(eqf(moved.min.y, 3.0f));
    ASSERT(eqf(moved.min.z, 4.0f));
    ASSERT(eqf(moved.max.x, 3.0f));
    ASSERT(eqf(moved.max.y, 5.0f));
    ASSERT(eqf(moved.max.z, 7.0f));

    // Transform test: uniform scale by 2
    m4f S = scale(2.0f);
    AABB scaled = transform(box, S);
    ASSERT(eqf(scaled.min.x, 0.0f));
    ASSERT(eqf(scaled.max.x, 2.0f));
    ASSERT(eqf(scaled.max.y, 4.0f));
    ASSERT(eqf(scaled.max.z, 6.0f));

    // Transform test: scale + translate
    m4f M = matMul(T, S);
    AABB st = transform(box, M);
    ASSERT(eqf(st.min.x, 2.0f));
    ASSERT(eqf(st.max.x, 4.0f));
    ASSERT(eqf(st.min.y, 3.0f));
    ASSERT(eqf(st.max.y, 7.0f)); 

    return true;
}

bool testMisc()
{
    ASSERT(eqf(lerp(0.0f, 10.0f, 0.5f), 5.0f));

    v2f a2 = {0, 0}, b2 = {10, 10};
    v2f r2 = lerp(a2, b2, 0.5f);
    ASSERT(eqf(r2.x, 5.0f));
    ASSERT(eqf(r2.y, 5.0f));

    v3f a3 = {0, 0, 0}, b3 = {2, 4, 6};
    v3f r3 = lerp(a3, b3, 0.5f);
    ASSERT(eqf(r3.x, 1.0f));
    ASSERT(eqf(r3.y, 2.0f));
    ASSERT(eqf(r3.z, 3.0f));

    return true;
}

bool testMath()
{
    LOG("[TEST-MATH] Testing vector...");
    testVector();

    LOG("[TEST-MATH] Testing quaternion...");
    testQuat();

    LOG("[TEST-MATH] Testing plane...");
    testPlane();

    LOG("[TEST-MATH] Testing matrix...");
    testMatrix();

    LOG("[TEST-MATH] Testing transform...");
    testTransform();

    LOG("[TEST-MATH] Testing view/projection...");
    testViewProjection();

    LOG("[TEST-MATH] Testing AABB...");
    testAABB();

    LOG("[TEST-MATH] All math tests passed.");
    return true;
}
