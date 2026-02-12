#include "volumes.hpp"
#include "../core/debug.hpp"
#include "../math/math.hpp"

AABB transformAABB(AABB aabb, m4f transform)
{
    // TODO_DW: MATH
    // This performs a lot of calculations that could be optimized to less
    // transforming, and no idea if this works for non uniform scaling.

    // Compute all 8 vertices of AABB
    v4f aabbVertices[8] =
    {
        {aabb.min.x, aabb.min.y, aabb.min.z, 1.f},
        {aabb.max.x, aabb.min.y, aabb.min.z, 1.f},
        {aabb.min.x, aabb.max.y, aabb.min.z, 1.f},
        {aabb.max.x, aabb.max.y, aabb.min.z, 1.f},
        {aabb.min.x, aabb.min.y, aabb.max.z, 1.f},
        {aabb.max.x, aabb.min.y, aabb.max.z, 1.f},
        {aabb.min.x, aabb.max.y, aabb.max.z, 1.f},
        {aabb.max.x, aabb.max.y, aabb.max.z, 1.f},
    };

    // Transform all vertices
    v4f aabbTransformedVertices[8] =
    {
        matMul(transform, aabbVertices[0]),
        matMul(transform, aabbVertices[1]),
        matMul(transform, aabbVertices[2]),
        matMul(transform, aabbVertices[3]),
        matMul(transform, aabbVertices[4]),
        matMul(transform, aabbVertices[5]),
        matMul(transform, aabbVertices[6]),
        matMul(transform, aabbVertices[7]),
    };

    // Find min/max of new vertices and create new AABB
    AABB result = {};
    result.min = to3f(aabbTransformedVertices[0]);
    result.max = to3f(aabbTransformedVertices[0]);
    for(int32 i = 1; i < 8; i++)
    {
        result.min.x = MIN(result.min.x, aabbTransformedVertices[i].x);
        result.min.y = MIN(result.min.y, aabbTransformedVertices[i].y);
        result.min.z = MIN(result.min.z, aabbTransformedVertices[i].z);

        result.max.x = MAX(result.max.x, aabbTransformedVertices[i].x);
        result.max.y = MAX(result.max.y, aabbTransformedVertices[i].y);
        result.max.z = MAX(result.max.z, aabbTransformedVertices[i].z);
    }
    return result;
}

v3f getSize(AABB aabb)
{
    return (aabb.max - aabb.min);
}

v3f getCenter(AABB aabb)
{
    return aabb.min + getSize(aabb) * 0.5;
}

void sphere(float radius, uint32 stacks, uint32 slices,
        float* pVertices, uint16* pIndices, 
        uint32* pIndexCount, uint32* pVertexCount)
{
    ASSERT(stacks >= 2 && slices >= 3);
    ASSERT(pVertices && pIndices);
    uint32 vertexCount = 0;
    uint32 indexCount = 0;
    
    // Vertices
    uint32 vi = 0;
    for(uint32 i = 0; i <= stacks; i++)
    {
        float v = (float)i / (float)stacks;
        float phi = v * (float)PI;

        for(uint32 j = 0; j <= slices; j++)
        {
            float u = (float)j / (float)slices;
            float theta = u * 2.f * (float)PI;

            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);

            pVertices[vi++] = x;
            pVertices[vi++] = y;
            pVertices[vi++] = z;
            vertexCount++;
        }
    }

    // Indices
    uint32 ii = 0;
    for(uint32 i = 0; i < stacks; i++)
    {
        for(uint32 j = 0; j < slices; j++)
        {
            uint16 a = (uint16)(i       * (slices + 1) + j);
            uint16 b = (uint16)((i + 1) * (slices + 1) + j);
            uint16 c = (uint16)((i + 1) * (slices + 1) + (j + 1));
            uint16 d = (uint16)(i       * (slices + 1) + (j + 1));

            if(i != stacks - 1)
            {
                pIndices[ii++] = a;
                pIndices[ii++] = c;
                pIndices[ii++] = b;
                indexCount += 3;
            }

            if(i != 0)
            {
                pIndices[ii++] = a;
                pIndices[ii++] = d;
                pIndices[ii++] = c;
                indexCount += 3;
            }
        }
    }

    if(pIndexCount) *pIndexCount = indexCount;
    if(pVertexCount) *pVertexCount = vertexCount;
}

bool inFrustum(v3f p, Frustum f)
{
    for(uint32 i = 0; i < 6; i++)
    {
        float sdf = distanceToPlane(p, f.planes[i]);
        if(sdf < 0.f)
        {
            return false;
        }
    }
    return true;
}

bool inFrustum(AABB aabb, Frustum f)
{
    // AABB is in frustum if, for each plane, the point furthest along the plane's normal
    // is inside its half-space.
    for(uint32 i = 0; i < 6; i++)
    {
        // Select point on AABB that's furthest along plane normal
        v3f p;
        p.x = f.planes[i].x < 0.f ? aabb.min.x : aabb.max.x;
        p.y = f.planes[i].y < 0.f ? aabb.min.y : aabb.max.y;
        p.z = f.planes[i].z < 0.f ? aabb.min.z : aabb.max.z;

        // Positive distance means in frustum half-space.
        float sdf = distanceToPlane(p, f.planes[i]);
        if(sdf < 0.f)
        {
            return false;
        }
    }
    return true;
}
