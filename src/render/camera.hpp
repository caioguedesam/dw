#pragma once
#include "../core/app.hpp"
#include "../math/math.hpp"

float fovHtoV(float fovX, float aspect);

struct CameraDesc
{
    float mFovY = 0;
    float mAspect = 0;
    float mNear = 0;
    float mFar = 0;

    float mSpeed = 20.f;
    float mAngularSpeed = 10.f;
    float mSmoothing = 10.f;
};

struct Camera
{
    CameraDesc mDesc = {};

    v3f mPos = {0, 0, 0};
    v3f mTargetPos = {0, 0, 0};
    v3f mX = {0, 0, 0};
    v3f mY = {0, 0, 0};
    v3f mZ = {0, 0, 0};
};

void initCamera(v3f pos, v3f lookAt, CameraDesc desc, Camera* pCamera);
void moveCamera(Camera* pCamera, v3f moveDir, float dt);
void rotateCamera(Camera* pCamera, v2f rotateDir, float dt);
void updateCamera(Camera* pCamera, float dt);

m4f getView(Camera* pCamera);
m4f getProj(Camera* pCamera);
