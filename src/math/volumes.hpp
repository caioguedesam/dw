#pragma once
#include "math.hpp"
struct Camera;

// AABB
struct AABB
{
    v3f min = {0,0,0};
    v3f max = {0,0,0};
};

AABB transformAABB(AABB aabb, m4f transform);
v3f getSize(AABB aabb);
v3f getCenter(AABB aabb);

// Frustum
struct Frustum
{
    // Planes (in order): left, right, bottom, top, near, far
    plane planes[6];
};

bool inFrustum(v3f p, Frustum f);
bool inFrustum(AABB aabb, Frustum f);
void frustumCorners(m4f view, m4f proj, v3f* pCorners, float zOffset = 0);
Frustum frustum(m4f vp);

// Misc
void sphere(float radius, uint32 stacks, uint32 slices,
        float* pVertices, uint16* pIndices, 
        uint32* pIndexCount = NULL, uint32* pVertexCount = NULL);
