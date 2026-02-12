#include "camera.hpp"
#include "../core/debug.hpp"
#include "../math/math.hpp"

float fovHtoV(float fovX, float aspect)
{
    return 2.f * (atanf(tanf(fovX / 2.f) * (1.f / aspect)));
}

void initCamera(v3f pos, v3f lookAt, CameraDesc desc, Camera* pCamera)
{
    ASSERT(pCamera);
    pCamera->mDesc = desc;
    pCamera->mPos = pos;
    pCamera->mTargetPos = pos;

    // lookAt is +Z. Camera is right handed, pointing to -Z.
    v3f dir = -normalize(lookAt - pos);
    v3f z = dir;
    v3f x = normalize(cross({0, 1, 0}, z));
    v3f y = normalize(cross(z, x));
    pCamera->mX = x;
    pCamera->mY = y;
    pCamera->mZ = z;
}

void moveCamera(Camera* pCamera, v3f moveDir, float dt)
{
    ASSERT(pCamera);

    moveDir = normalize(moveDir);
    // Update pos
    v3f pos = pCamera->mTargetPos;
    pos = pos + pCamera->mX * moveDir.x * pCamera->mDesc.mSpeed * dt;
    pos = pos + pCamera->mZ * moveDir.z * pCamera->mDesc.mSpeed * dt;
    pCamera->mTargetPos = pos;
}

void rotateCamera(Camera* pCamera, v2f rotateDir, float dt)
{
    v3f camDir = pCamera->mZ;
    camDir = rotate(camDir, rotateDir.x * pCamera->mDesc.mAngularSpeed * dt, pCamera->mY);
    camDir = rotate(camDir, rotateDir.y * pCamera->mDesc.mAngularSpeed * dt, pCamera->mX);

    v3f z = camDir;
    v3f x = normalize(cross({0, 1, 0}, z));
    v3f y = normalize(cross(z, x));
    pCamera->mX = x;
    pCamera->mY = y;
    pCamera->mZ = z;
}

void updateCamera(Camera *pCamera, float dt)
{
    ASSERT(pCamera);

    pCamera->mPos = lerp(pCamera->mPos, pCamera->mTargetPos, dt * pCamera->mDesc.mSmoothing);
}

m4f getView(Camera* pCamera)
{
    ASSERT(pCamera);
    return viewRH(pCamera->mX, pCamera->mY, pCamera->mZ, pCamera->mPos);
}

m4f getProj(Camera* pCamera)
{
    ASSERT(pCamera);
    return perspectiveRH(pCamera->mDesc.mFovY, 
            pCamera->mDesc.mAspect, 
            pCamera->mDesc.mNear, 
            pCamera->mDesc.mFar);
}

void getFrustumPoints(Camera* pCamera, v3f* pOut)
{
    ASSERT(pCamera && pOut);

    float zNear = pCamera->mDesc.mNear;
    float zFar = pCamera->mDesc.mFar;
    float aspect = pCamera->mDesc.mAspect;
    v3f p = pCamera->mPos;
    v3f d = normalize(-pCamera->mZ);
    v3f nc = p + (zNear * d);
    v3f fc = p + (zFar  * d);
    float nh2 = zNear * tanf(pCamera->mDesc.mFovY / 2.f);
    float nw2 = aspect * nh2;
    float fh2 = zFar * tanf(pCamera->mDesc.mFovY / 2.f);
    float fw2 = aspect * fh2;

    v3f ntl = nc + (pCamera->mY * nh2) - (pCamera->mX * nw2);
    v3f ntr = nc + (pCamera->mY * nh2) + (pCamera->mX * nw2);
    v3f nbl = nc - (pCamera->mY * nh2) - (pCamera->mX * nw2);
    v3f nbr = nc - (pCamera->mY * nh2) + (pCamera->mX * nw2);

    v3f ftl = fc + (pCamera->mY * fh2) - (pCamera->mX * fw2);
    v3f ftr = fc + (pCamera->mY * fh2) + (pCamera->mX * fw2);
    v3f fbl = fc - (pCamera->mY * fh2) - (pCamera->mX * fw2);
    v3f fbr = fc - (pCamera->mY * fh2) + (pCamera->mX * fw2);

    pOut[0] = ntl;
    pOut[1] = ntr;
    pOut[2] = nbl;
    pOut[3] = nbr;
    pOut[4] = ftl;
    pOut[5] = ftr;
    pOut[6] = fbl;
    pOut[7] = fbr;
}

Frustum getFrustum(Camera* pCamera)
{
    v3f points[8];
    getFrustumPoints(pCamera, points);

    Frustum result = {};
    // Near plane
    result.planes[0] = getPlane(points[2], points[0], points[3]);
    // Far plane
    result.planes[1] = getPlane(points[7], points[5], points[6]);
    // Left plane
    result.planes[2] = getPlane(points[6], points[4], points[2]);
    // Right plane
    result.planes[3] = getPlane(points[3], points[1], points[7]);
    // Bottom plane
    result.planes[4] = getPlane(points[3], points[7], points[2]);
    // Top plane
    result.planes[5] = getPlane(points[0], points[4], points[1]);

    return result;
}
