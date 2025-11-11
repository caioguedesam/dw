#pragma once
#include "math.hpp"

// AABB
struct AABB
{
    v3f min = {0,0,0};
    v3f max = {0,0,0};
};

AABB transform(AABB aabb, m4f transform);
v3f getSize(AABB aabb);
v3f getCenter(AABB aabb);

// Frustum
struct Frustum
{
    // TODO_DW: MATH Not sure if I need all this point data.
    // Frustum points:
    //
    //                  4 ------------- 5
    //  0 ----- 1       |               |
    //  |   N   |       |       F       |
    //  3 ----- 2       |               |
    //                  7 ------------- 6
    //
    v3f points[8];
    // Frustum planes (in order): left, right, bottom, top, near, far
    plane planes[6];
};

Frustum getFrustum(m4f view, m4f proj);
bool inFrustum(v3f p, Frustum f);
bool inFrustum(AABB aabb, Frustum f);

// Misc
void sphere(float radius, uint32 stacks, uint32 slices,
        float* pVertices, uint16* pIndices, 
        uint32* pIndexCount = NULL, uint32* pVertexCount = NULL);
