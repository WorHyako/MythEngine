#include <Renderer/Sea.hpp>
#include <Utils/UtilsMath.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera/TestCamera.hpp"

glm::mat4 matrixReflect(glm::vec4 plane) {

    return glm::mat4{
        1 - 2 * plane.x * plane.x,  -2 * plane.x * plane.y,  -2 * plane.x * plane.z, -2 * plane.x * plane.w,
         -2 * plane.y * plane.x, 1 - 2 * plane.y * plane.y,  -2 * plane.y * plane.z, -2 * plane.y * plane.w,
         -2 * plane.z * plane.x,  -2 * plane.z * plane.y, 1 - 2 * plane.z * plane.z, -2 * plane.z * plane.w,
                          0,                   0,                   0,                  1
    };
}

void Sea::EnvMap_GetSideMatrix(CubemapFaces Face, glm::mat4x4& mView)
{
    glm::vec3 lookTo, up;

    switch (Face)
    {
    case CubemapFaces::CUBEMAP_FACE_POSITIVE_X:
        lookTo = glm::vec3(1.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    case CubemapFaces::CUBEMAP_FACE_NEGATIVE_X:
        lookTo = glm::vec3(-1.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    case CubemapFaces::CUBEMAP_FACE_POSITIVE_Y:
        lookTo = glm::vec3(0.0f, 1.0f, 0.0f);
        up = glm::vec3(0.0f, 0.0f, -1.0f);
        break;
    case CubemapFaces::CUBEMAP_FACE_NEGATIVE_Y:
        lookTo = glm::vec3(0.0f, -1.0f, 0.0f);
        up = glm::vec3(0.0f, 0.0f, 1.0f);
        break;
    case CubemapFaces::CUBEMAP_FACE_POSITIVE_Z:
        lookTo = glm::vec3(0.0f, 0.0f, 1.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    case CubemapFaces::CUBEMAP_FACE_NEGATIVE_Z:
        lookTo = glm::vec3(0.0f, 0.0f, -1.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    default:
        lookTo = glm::vec3(1.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    mView = glm::lookAt(glm::vec3(0.0f), lookTo, up);
}

bool Sea::SunRoad_Render2()
{
    //auto mOldView = rs->GetView();
    //auto mOldProjection = rs->GetProjection();

    //rs->EndScene();
    //rs->PushRenderTarget();

    //// rs->SetProjection( CMatrix().BuildProjectionMatrix(PI / 2.0f, 256.0f, 256.0f, 1.0f, 4000.0f) );

    //auto PlaneHeight = 0.5f;

    //const glm::vec3 point(0, PlaneHeight, 0), normal(0, 1, 0);
    //Plane plane = Plane(normal, point);

    //glm::mat4 invertor = matrixReflect(glm::vec4(plane.normal, plane.distance));

    //auto mView = rs->GetView();
    //auto mViewNew = mView;

    //mViewNew = invertor * mViewNew;

    //rs->SetView(mViewNew);

    //uint32_t Colors[6] = { 0xd934c8, 0x2FFF1F, 0x0000FF, 0xFF00, 0xb28e11, 0x0 };
    //// for (uint32_t i=0; i<6; i++)
    //{
    //    IDirect3DSurface9* pReflectionSurface;
    //    pReflectionSunroad->GetSurfaceLevel(0, &pReflectionSurface);

    //    rs->BeginScene();
    //    bool bCool = rs->SetRenderTarget(pReflectionSurface, pZStencil);
    //    bCool = D3D_OK != rs->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0x0, 1.0f, 0);

    //    // CMatrix mView;
    //    // EnvMap_GetSideMatrix((D3DCUBEMAP_FACES)i, mView);

    //    // mView.m[3][0] = -(mView.m[0][0] * m_CamPos.x - mView.m[1][0] * m_CamPos.y + mView.m[2][0] * m_CamPos.z);
    //    // mView.m[3][1] = -(mView.m[0][1] * m_CamPos.x - mView.m[1][1] * m_CamPos.y + mView.m[2][1] * m_CamPos.z);
    //    // mView.m[3][2] = -(mView.m[0][2] * m_CamPos.x - mView.m[1][2] * m_CamPos.y + mView.m[2][2] * m_CamPos.z);

    //    // rs->SetView(mView);

    //    // Render scene here.


    //    rs->EndScene();
    //    pReflectionSurface->Release();
    //}

    //rs->PopRenderTarget();
    //rs->SetView(mOldView);
    //rs->SetProjection(mOldProjection);

    //rs->BeginScene();
    //mOldView = rs->GetView();

    return true;
}

bool Sea::EnvMap_Render2()
{
    //rs->EndScene();
    //rs->PushRenderTarget();

    //// rs->SetProjection( CMatrix().BuildProjectionMatrix(PI / 2.0f, 256.0f, 256.0f, 1.0f, 4000.0f) );

    //float PlaneHeight = 0.5f;

    //const glm::vec3 point(0, PlaneHeight, 0), normal(0, 1, 0);
    //Plane plane = Plane(normal, point);

    //glm::mat4 invertor = matrixReflect(glm::vec4(plane.normal, plane.distance));

    //glm::mat4 mViewNew = GetCamera()->getViewMatrix();

    //mViewNew = invertor * mViewNew;

    //rs->SetView(mViewNew);

    //// rs->SetEffect("FlatSeaReverseCull");
    //// Event("SeaReflection");

    //uint32_t Colors[6] = { 0xd934c8, 0x2FFF1F, 0x0000FF, 0xFF00, 0xb28e11, 0x0 };
    //// for (uint32_t i=0; i<6; i++)
    //{
    //    IDirect3DSurface9* pReflectionSurface;
    //    pReflection->GetSurfaceLevel(0, &pReflectionSurface);

    //    rs->BeginScene();
    //    // bool bCool = rs->SetRenderTarget(pEnvMap, i, 0, pZStencil);
    //    // bool bCool = rs->SetRenderTarget(pEnvMap, i, 0, pZStencil);
    //    bool bCool = rs->SetRenderTarget(pReflectionSurface, pReflectionSurfaceDepth);
    //    bCool = D3D_OK != rs->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0x0, 1.0f, 0);

    //    // CMatrix mView;
    //    // EnvMap_GetSideMatrix((D3DCUBEMAP_FACES)i, mView);

    //    // mView.m[3][0] = -(mView.m[0][0] * m_CamPos.x - mView.m[1][0] * m_CamPos.y + mView.m[2][0] * m_CamPos.z);
    //    // mView.m[3][1] = -(mView.m[0][1] * m_CamPos.x - mView.m[1][1] * m_CamPos.y + mView.m[2][1] * m_CamPos.z);
    //    // mView.m[3][2] = -(mView.m[0][2] * m_CamPos.x - mView.m[1][2] * m_CamPos.y + mView.m[2][2] * m_CamPos.z);

    //    // core.Trace("sea: %.3f, %.3f, %.3f", mView.m[3][0], mView.m[3][1], mView.m[3][2]);
    //    // rs->SetView(mView);

    //    // Render scene here.
    //    

    //    rs->EndScene();
    //    pReflectionSurface->Release();
    //}

    //rs->PopRenderTarget();
    //rs->SetView(mOldView);
    //rs->SetProjection(mOldProjection);

    //rs->BeginScene();

    //glm::mat4 mTex = glm::identity<glm::mat4>();
    //mTex = glm::scale(mTex, glm::vec3(0.5f, -0.5f, 0.5f));
    //mTex = glm::translate(mTex, glm::vec3(0.5f, 0.5f, 0.5f));

    //glm::mat4 mProj = rs->GetProjection();
    //mProj = mProj * mTex;

    //glm::mat4 mCurWorld = rs->GetWorld();
    //glm::mat4 mCurView = rs->GetView();

    //m_TexProjection = (mCurWorld * mCurView) * mProj;

    return true;
}

bool Sea::SunRoad_Render()
{
 //   glm::mat4 mOldView = rs->GetView();
 //   glm::mat4 mOldProjection = rs->GetProjection();

 //   rs->EndScene();
 //   rs->PushRenderTarget();

 //   glm::mat4 projMatrix = glm::perspective(Math::PI / 2.0f, 1.0f, 1.0f, 4000.0f);
	//rs->SetProjection(glm::mat4().BuildProjectionMatrix(PI / 2.0f, 256.0f, 256.0f, 1.0f, 4000.0f));

 //   for (uint32_t i = 0; i < 6; i++)
 //   {
 //       if (!m_UnderSea && i == CubemapFaces::CUBEMAP_FACE_NEGATIVE_Y)
 //           continue;

 //       rs->BeginScene();
 //       bool bCool = rs->SetRenderTarget(pSunRoadMap, i, 0, pZStencil);
 //       bCool = D3D_OK != rs->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0x0, 1.0f, 0);

 //       glm::mat4 mView;
 //       EnvMap_GetSideMatrix(static_cast<CubemapFaces>(i), mView);

 //       mView[3][0] = -(mView[0][0] * m_CamPos.x - mView[1][0] * m_CamPos.y + mView[2][0] * m_CamPos.z);
 //       mView[3][1] = -(mView[0][1] * m_CamPos.x - mView[1][1] * m_CamPos.y + mView[2][1] * m_CamPos.z);
 //       mView[3][2] = -(mView[0][2] * m_CamPos.x - mView[1][2] * m_CamPos.y + mView[2][2] * m_CamPos.z);

 //       rs->SetView(mView);

 //       // Render scene here.
 //       

 //       rs->EndScene();
 //   }

 //   rs->PopRenderTarget();
 //   rs->SetView(mOldView);
 //   rs->SetProjection(mOldProjection);

 //   rs->BeginScene();
 //   mOldView = rs->GetView();

    return true;
}

bool Sea::EnvMap_Render()
{
    //rs->EndScene();
    //rs->PushRenderTarget();

    //glm::mat4 projMatrix = glm::perspective(Math::PI / 2.0f, 1.0f, 1.0f, 4000.0f);

    //uint32_t Colors[6] = { 0xd934c8, 0x2FFF1F, 0x0000FF, 0xFF00, 0xb28e11, 0x0 };
    //for (uint32_t i = 0; i < 6; i++)
    //{
    //    if (!m_UnderSea && i == CubemapFaces::CUBEMAP_FACE_NEGATIVE_Y)
    //        continue;
    //    // if (i != D3DCUBEMAP_FACE_NEGATIVE_Z) continue;

    //    rs->BeginScene();
    //    bool bCool = rs->SetRenderTarget(pEnvMap, i, 0, pZStencil);
    //    bCool = D3D_OK != rs->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, Colors[i] /*0x0*/, 1.0f, 0);

    //    glm::mat4 mView;
    //    EnvMap_GetSideMatrix(static_cast<CubemapFaces>(i), mView);

    //    mView[3][0] = -(mView[0][0] * m_CamPos.x - mView[1][0] * m_CamPos.y + mView[2][0] * m_CamPos.z);
    //    mView[3][1] = -(mView[0][1] * m_CamPos.x - mView[1][1] * m_CamPos.y + mView[2][1] * m_CamPos.z);
    //    mView[3][2] = -(mView[0][2] * m_CamPos.x - mView[1][2] * m_CamPos.y + mView[2][2] * m_CamPos.z);

    //    glm::mat4 mProjView = projMatrix * mView;
    //    rs->SetView(mView);

    //    // Render scene here.

    //    rs->EndScene();
    //}

    //rs->PopRenderTarget();

    //rs->BeginScene();

    return true;
}
