#include "volumes.hpp"

AABB transform(AABB aabb, m4f transform)
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
