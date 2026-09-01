
#include <raylib.h>
#include "rlImGui.h"
#include "imgui.h"

#include <iostream>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>

#include <memory>

#include "Settings/ProjectSettings.h"
#include "Render/BasicRender.h"

// using namespace JPH;


// Raylib expects a specific callback signature: void Func(void* buffer, unsigned int frames); not really using sound so it'll be a blank callback
static void RenderAudio(void* bufferData, unsigned int frames) 
{
    
}

int main() {
    // Init Jolt
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    // RaylibDrawTest DrawTest;
    
    // create a scope so cleanup happens in a predictable mannor
    // Init Raylib
    auto projectSettings = std::make_unique<ProjectSettings>();
    auto pool = std::make_unique<BasicRender>(projectSettings->physicsSystem, projectSettings->myContactListener);
    
    // pass the integer location (ID) of the depth as sampler2D
    int depthLoc = GetShaderLocation(projectSettings->cavityShader, "texture1");
    // pass the int location of the float time
    int timeLoc = GetShaderLocation(projectSettings->cavityShader, "time");
    int viewEyeLoc = GetShaderLocation(projectSettings->cavityShader, "viewEye");
    int viewCenterLoc = GetShaderLocation(projectSettings->cavityShader, "viewCenter");
    int ambientLoc = GetShaderLocation(projectSettings->cavityShader, "ambient");
    float amb[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
    SetShaderValue(projectSettings->cavityShader, ambientLoc, amb, SHADER_UNIFORM_VEC4);
    
    // raylib lighting
    Light lights[1] = { 0 }; // Use MAX_LIGHTS = 4
    lights[0] = projectSettings->CreateLight(LIGHT_POINT, Vector3{ 0.0f, 15.0f, 0.0f }, projectSettings->camera.target, WHITE, projectSettings->cavityShader);
    lights[0].enabled = true;
    projectSettings->UpdateLightValues(projectSettings->cavityShader, lights[0]);
    pool->updateShader(projectSettings->cavityShader);
    
    // Init Raylib's Audio Engine
    InitAudioDevice();

    // Init rlImGui
    rlImGuiSetup(true);
    
    // Create a Raylib Audio Stream (44100 Hz, 16-bit, 2 Channels/Stereo)
    AudioStream stream = LoadAudioStream(44100, 16, 2);
    
    // Attach our TSF callback to the Raylib stream
    SetAudioStreamCallback(stream, RenderAudio);
    
    // Start the stream (It will play silence until we press a note)
    PlayAudioStream(stream);
    bool isNotePlaying = false;
    double noteStartTime = 0.0;
    const double NOTE_DURATION = 1.0;
    
    while (!WindowShouldClose()) {
        if (IsWindowResized())
        {
            projectSettings->ResizeCanvas();
            depthLoc = GetShaderLocation(projectSettings->cavityShader, "texture1");
        }
        projectSettings->MoveCamera();
        float deltaTime = GetFrameTime();
        float shaderTime = GetTime();
        float cameraPos[3] = { projectSettings->camera.position.x, projectSettings->camera.position.y, projectSettings->camera.position.z };
        float cameraTarget[3] = { projectSettings->camera.target.x, projectSettings->camera.target.y, projectSettings->camera.target.z };
        if (deltaTime > 0.016f) deltaTime = 0.016f;
        pool->update(deltaTime);
        projectSettings->physicsSystem->Update(deltaTime, 1, projectSettings->tempAllocator, projectSettings->jobSystem);
        SetShaderValue(projectSettings->cavityShader, timeLoc, &shaderTime, SHADER_UNIFORM_FLOAT);
        SetShaderValue(projectSettings->cavityShader, viewEyeLoc, cameraPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(projectSettings->cavityShader, viewCenterLoc, cameraTarget, SHADER_UNIFORM_VEC3);
        BeginTextureMode(projectSettings->mainRenderCanvas);
        ClearBackground(DARKGRAY);
        BeginMode3D(projectSettings->camera);
            // DrawGrid(10, 1.0f);
            pool->renderGroundPlane();
            pool->renderDynamics();
        EndTextureMode();
            
        EndMode3D();
        BeginDrawing();
        ClearBackground(BLACK);
        BeginShaderMode(projectSettings->cavityShader);
        // Passes a unform sampler2D to the shader
        SetShaderValueTexture(projectSettings->cavityShader, depthLoc, projectSettings->mainRenderCanvas.depth);
        // Passes a uniform float to the shader
       
        DrawTextureRec(
            projectSettings->mainRenderCanvas.texture,
            Rectangle{0.0f, 0.0f, (float)projectSettings->mainRenderCanvas.texture.width,
                    (float)-projectSettings->mainRenderCanvas.texture.height},
            Vector2{0.0f, 0.0f},
            WHITE
        );
        EndShaderMode();
        BeginMode3D(projectSettings->camera);
        pool->renderDebug();
        EndMode3D();
        rlImGuiBegin();
        
        ImGui::Begin("Engine Debug");
        ImGui::Text("FPS: %i", GetFPS());
        
        
        if (ImGui::Button("Test Button.")) {
            
            
        }
        
        ImGui::End();
        rlImGuiEnd();

        EndDrawing();
    }
    
    pool.reset();
    projectSettings.reset();
    
    // Cleanup
    StopAudioStream(stream);
    UnloadAudioStream(stream);
    
    
    rlImGuiShutdown();
    CloseAudioDevice();
    
    CloseWindow();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    
    // delete[] JPH::RefTargetBase::sLeakDetector;

    return 0;
}