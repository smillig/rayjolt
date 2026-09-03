
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
    
    // Init Raylib
    auto projectSettings = std::make_unique<ProjectSettings>();
    auto pool = std::make_unique<BasicRender>(projectSettings->physicsSystem, projectSettings->myContactListener);
    
    
    // in raylib for the lighting shader to work the shader must be updated in the model material properties
    pool->updateModelShader(projectSettings->lightingShader);
    // slider stuff for ImGui and playing with shaders:
    float ambientSlider = 0.3f;
    float noiseAmountSlider = {0.001f}; 
    float errorPeriodSlider = {5.0f}; 
    float errorRangeSlider = {0.0015f}; 
    float lineColorBlendAmountSlider = {1.0f};
    float edgeColorPicker[3] = {0.2f, 0.2f, 0.25f};
    bool bIsLineDepthEnabled = true;
    bool bIsLineSobelEnabled = true;
    
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
        }
        projectSettings->MoveCamera();
        float deltaTime = GetFrameTime();
        
        if (deltaTime > 0.016f) deltaTime = 0.016f;
        pool->update(deltaTime);
        projectSettings->physicsSystem->Update(deltaTime, 1, projectSettings->tempAllocator, projectSettings->jobSystem);
        projectSettings->updateLightShader();
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
        BeginShaderMode(projectSettings->styleShader);
        // Passes a unform sampler2D to the shader
        projectSettings->updateDepthTexture();
       
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
        
        ImGui::Begin("Shader Settings");
        ImGui::Text("FPS: %i", GetFPS());
        
        
        if (ImGui::Button("Toggle Lights!")) {
            projectSettings->toggleLights();
        }
        if (ImGui::Button("Toggle Toon")){
            projectSettings->toggleToon();
        }

        ImGui::SliderFloat("Adjust ambient", &ambientSlider, 0.0f, 1.0f);

        ImGui::Text("Line Style Shader Customization:");

        ImGui::Checkbox("Enable Line Depth Pass", &bIsLineDepthEnabled);
        ImGui::Checkbox("Enable Line Soble Detection", &bIsLineSobelEnabled);
        ImGui::SliderFloat("Adjust Noise Amount", &noiseAmountSlider, 0.0001f, 0.01f);
        ImGui::SliderFloat("Adjust Error Period", &errorPeriodSlider, 0.1f, 100.0f);
        ImGui::SliderFloat("Adjust Error Range", &errorRangeSlider, 0.00015f, 0.015f);
        ImGui::ColorPicker3("Line Color", edgeColorPicker);

        ImGui::End();
        rlImGuiEnd();

        EndDrawing();
        projectSettings->ambientStrength = Vector4{ambientSlider, ambientSlider, ambientSlider, 1.0f};
        projectSettings->updateNoiseAmount(noiseAmountSlider);
        projectSettings->updateErrorPeriod(errorPeriodSlider);
        projectSettings->updateErrorRange(errorRangeSlider);
        projectSettings->updateLineColor(edgeColorPicker);
        projectSettings->toggleDepthLine(bIsLineDepthEnabled);
        projectSettings->toggleSobel(bIsLineSobelEnabled);
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