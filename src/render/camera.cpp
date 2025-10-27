#include "camera.hpp"
#include "../core/debug.hpp"
#include "src/math/math.hpp"

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
    //v4f camDir = to4f(pCamera->mZ, 0);
    // m4f rotY = rotation(rotateDir.x * pCamera->mDesc.mAngularSpeed * dt, pCamera->mY);
    // m4f rotX = rotation(rotateDir.y * pCamera->mDesc.mAngularSpeed * dt, pCamera->mX);
    // camDir = normalize(matMul(rotY, camDir));
    // camDir = normalize(matMul(rotX, camDir)); 

    // lookAt is +Z. Camera is right handed, pointing to -Z.
    //v3f z = to3f(camDir);
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

#if 0
    LOGF("[CAMERA] - Target (%.3f, %.3f, %.3f), Pos (%.3f, %.3f, %.3f), t = %.6f)",
            pCamera->mTargetPos.x,
            pCamera->mTargetPos.y,
            pCamera->mTargetPos.z,
            pCamera->mPos.x,
            pCamera->mPos.y,
            pCamera->mPos.z,
            dt / pCamera->mDesc.mSmoothing);
#endif
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
