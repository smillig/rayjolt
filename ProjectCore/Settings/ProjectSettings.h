#pragma once

#include "raylib.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

namespace Layers {
    static constexpr JPH::ObjectLayer STATIC = 0;
    static constexpr JPH::ObjectLayer DYNAMIC = 1;
    static constexpr JPH::ObjectLayer SENSOR = 2;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
};;

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS = 2;
}

class MyContactListener : public JPH::ContactListener
{
public:
    // This queue will hold the IDs of bodies that touched a sensor
    std::vector<JPH::BodyID> bodiesToProcess;
    std::mutex queueMutex; // Protects the vector from multi-threading crashes!

    // On contact is for initial collisions
    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    
    // Jolt requires these in the interface
    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {}
    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {}
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override { return JPH::ValidateResult::AcceptAllContactsForThisBodyPair; }
};

// some lighting stuff since I didn't want to mess with CMake anymore:
typedef struct {   
    int type;
    bool enabled;
    Vector3 position;
    Vector3 target;
    Color color;
    float attenuation;
    
    // Shader locations
    int enabledLoc;
    int typeLoc;
    int positionLoc;
    int targetLoc;
    int colorLoc;
    int attenuationLoc;
} Light;

// Light type
typedef enum {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT
} LightType;


class ProjectSettings
{
public:
    ProjectSettings();
    ~ProjectSettings();
    static const int screenWidth = 1280;
    static const int screenHeight = 720;
    static const int framerateTarget = 60;
    
    RenderTexture2D mainRenderCanvas;
    Shader lightingShader;
    Shader styleShader;

    Camera camera = {0};

    int depthLoc = {0};
    int timeLoc = {0};
    int viewEyeLoc = {0};
    int viewCenterLoc = {0};
    int ambientLoc = {0};
    int ambientLocStyle = {0};

    float cameraPos[3] = {0.0f, 0.0f, 0.0f};
    float cameraTarget[3]= {0.0f, 0.0f, 0.0f};
    float shaderTime = {0};

    Vector4 ambientStrength = {0.1f, 0.1f, 0.1f, 1.0f};
    
    JPH::TempAllocatorImpl* tempAllocator = nullptr;
    JPH::JobSystemThreadPool* jobSystem = nullptr;
    JPH::PhysicsSystem* physicsSystem = nullptr;
    MyContactListener* myContactListener = nullptr;
    
    void MoveCamera();
    void ResizeCanvas();
    Light CreateLight(int type, Vector3 position, Vector3 target, Color color, Shader shader);   // Create a light and get shader locations
    void UpdateLightValues(Shader shader, Light light);
    void updateDepthTexture();
    void updateLightShader();
    void toggleLights();
    void toggleToon();
    void toggleDepthBufferView();
    void updateNoiseAmount(float nAmount);
    void updateErrorPeriod(float ePeriodAmt);
    void updateErrorRange(float eRangeAmt);
    void updateLineColorBlend(float lineColorAmt);
    void updateLineColor(float NewColor[3]);
    void toggleSobel(bool toggledSobel);
    void toggleDepthLine(bool toggledDepth);
    void updateNearPlane(float nearPlaneVal);
    void updateFarPlane(float farPlaneVal);
    void updateSobelKernelSize(float sobelKernelSizeVal);
    void updatedpethSensitivity(float depthSensitivityVal);
    void updateEdgeThresh(float edgeThreshVal);
    void updateSobelEdgeThres(float sobelEdgeThresVal);
     
private:
    int toonLoc = {0};
    int edgeColorLoc = {0};
    int noiseAmountLoc = {0};
    int errorPeriodLoc = {0};
    int errorRangeLoc = {0};
    int lineColorBlendLoc = {0};
    int toggleDepthLoc = {0};
    int toggleSobelLoc = {0};
    int depthNearPlaneLoc = {0}; // depth contrast for near plane (close objects are black)
    int depthFarPlaneLoc = {0}; // depth contrast for far plane (horizon is white)
    int sobelKernelSizeLoc = {0};       // adjust line thickness of sobel
    int depthSensitivityLoc = {0};
    int depthEdgeThresholdLoc = {0};
    int sobelEdgeThresholdLoc = {0};
    int depthViewEnabledLoc = {0};

    int toonEnabled = {1};
    int depthViewEnabled = {0};
    Vector3 EdgeColor = Vector3{0.2f, 0.2f, 0.25f};
    float NoiseAmount = {0.001f}; // default 0.002 - how fuzzy the lines are
    float ErrorPeriod = {5.0f}; // default 30.0  - how wavy the lines are
    float ErrorRange = {0.0015f}; // default 0.003 - how offset the different lines are from each other
    float LineColorBlendAmount = {1.0f};
    int bIsDepthLineEnabled = {1};
    int bIsSobelLineEnabled = {1};
    float depthNearPlane = {0.09}; // depth contrast for near plane (close objects are black)
    float depthFarPlane = {100.0}; // depth contrast for far plane (horizon is white)
    float sobelKernelSize = {1.5};       // adjust line thickness of sobel
    float depthSensitivity = {2.0};
    float depthEdgeThreshold = {0.3}; // threshold before considering a line is present for depth
    float sobelEdgeThreshold = {0.15}; // threshold before considering a line is present for sobel

    float cameraMovementSpeed = 5.0f;
    float camreaZoomSpeed = 200.0f;
    
    bool rotateUp = false;
    float panSpeed = 0.5f;
    bool rotateAroundTarget = true;
    bool lockView = true;

    Light lights[1] = { 0 };
};
