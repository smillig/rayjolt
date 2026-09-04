#include "ProjectSettings.h"

#include <thread> 

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include "rcamera.h"
#include "rlgl.h"

// BroadPhaseLayers: Non-Moving and Moving

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        mObjectToBroadPhase[Layers::STATIC] = BroadPhaseLayers::MOVING;
        mObjectToBroadPhase[Layers::SENSOR] = BroadPhaseLayers::NON_MOVING;
        
        mObjectToBroadPhase[Layers::DYNAMIC] = BroadPhaseLayers::MOVING;
    }
    virtual JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override { return mObjectToBroadPhase[inLayer]; }
    
    // New naming convention in 5.6
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char * GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:          return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:              return "MOVING";
            default:                                                                return "INVALID";
        }
    }
#endif
private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// What object layers collide with which broadphase layers
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
        case Layers::STATIC:        return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::SENSOR:        return inLayer2 == BroadPhaseLayers::MOVING;
            
        case Layers::DYNAMIC:         return true;
        default:                    return false;
        }
    }
};

// Which object layers interact with what other object layers
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
        case Layers::STATIC:        return inObject2 == Layers::DYNAMIC;
        case Layers::SENSOR:        return inObject2 == Layers::DYNAMIC;
            
        case Layers::DYNAMIC:       return true;
        default:                    return false;
        }
    }
};

// Global instances of our boilerplate filters
static BPLayerInterfaceImpl broadPhaseLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl objectVsBroadphaseLayerFilter;
static ObjectLayerPairFilterImpl objectVsObjectLayerFilter;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static int lightsCount = 0;    // Current amount of created lights
constexpr int MAX_LIGHTS = 4;

// have to write a custom depth texture handler since RenderTexture2D's depth outputs to the buffer
static RenderTexture2D LoadRenderTextureDepthTex(int width, int height) {
    RenderTexture2D target = { 0 };
    target.id = rlLoadFramebuffer();
    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.texture.id = rlLoadTexture(0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        target.texture.mipmaps = 1;

        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19; // DEPTH_COMPONENT_24BIT
        target.depth.mipmaps = 1;

        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
        rlDisableFramebuffer();
    }
    return target;
}

static void UnloadRenderTextureDepthTex(RenderTexture2D target) {
    if (target.id > 0) {
        rlUnloadTexture(target.texture.id);
        rlUnloadTexture(target.depth.id);
        rlUnloadFramebuffer(target.id);
    }
}

ProjectSettings::ProjectSettings()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Raylib and Jolt Shader Fun");
    SetTargetFPS(framerateTarget);
    
    // Setup for shaders
    mainRenderCanvas = LoadRenderTextureDepthTex(screenWidth, screenHeight);
    lightingShader = LoadShader("Assets/Shaders/Light/light.vert", "Assets/Shaders/Light/light.frag");
    styleShader = LoadShader(0, "Assets/Shaders/Pencil/pencil.frag");
    // pass the integer location (ID) of the depth as sampler2D
    depthLoc = GetShaderLocation(styleShader, "texture1");
    // pass the int location of the float time
    timeLoc = GetShaderLocation(styleShader, "time");
    viewEyeLoc = GetShaderLocation(lightingShader, "viewEye");
    viewCenterLoc = GetShaderLocation(lightingShader, "viewCenter");
    ambientLoc = GetShaderLocation(lightingShader, "ambient");
    ambientLocStyle = GetShaderLocation(styleShader, "ambient");
    toonLoc = GetShaderLocation(lightingShader, "toon");

    edgeColorLoc = GetShaderLocation(styleShader, "edgeColor");;
    noiseAmountLoc = GetShaderLocation(styleShader, "noiseAmount");
    errorPeriodLoc = GetShaderLocation(styleShader, "errorPeriod");
    errorRangeLoc = GetShaderLocation(styleShader, "errorRange");
    lineColorBlendLoc = GetShaderLocation(styleShader, "lineColorBlend");
    toggleSobelLoc = GetShaderLocation(styleShader, "sobelEnabled");
    toggleDepthLoc = GetShaderLocation(styleShader, "depthLineEnabled");
    depthNearPlaneLoc = GetShaderLocation(styleShader, "depthNear");
    depthFarPlaneLoc = GetShaderLocation(styleShader, "depthFar");
    sobelKernelSizeLoc = GetShaderLocation(styleShader, "sobelKernelSize");
    depthSensitivityLoc = GetShaderLocation(styleShader, "depthSensitivity");
    depthEdgeThresholdLoc = GetShaderLocation(styleShader, "depthEdgeThres");
    sobelEdgeThresholdLoc = GetShaderLocation(styleShader, "sobelEdgeThres");
    depthViewEnabledLoc = GetShaderLocation(styleShader, "depthViewEnabled");

    SetShaderValue(lightingShader, ambientLoc, &ambientStrength, SHADER_UNIFORM_VEC4);
    SetShaderValue(styleShader, ambientLocStyle, &ambientStrength, SHADER_UNIFORM_VEC4);
    SetShaderValue(lightingShader, toonLoc, &toonEnabled, SHADER_UNIFORM_INT);

    SetShaderValue(styleShader, edgeColorLoc, &EdgeColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(styleShader, noiseAmountLoc, &NoiseAmount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, errorPeriodLoc, &ErrorPeriod, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, errorRangeLoc, &ErrorRange, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, toggleDepthLoc, &bIsDepthLineEnabled, SHADER_UNIFORM_INT);
    SetShaderValue(styleShader, toggleSobelLoc, &bIsSobelLineEnabled, SHADER_UNIFORM_INT);
    SetShaderValue(styleShader, depthNearPlaneLoc, &depthNearPlane, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, depthFarPlaneLoc, &depthFarPlane, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, sobelKernelSizeLoc, &sobelKernelSize, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, depthSensitivityLoc, &depthSensitivity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, depthEdgeThresholdLoc, &depthEdgeThreshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, sobelEdgeThresholdLoc, &sobelEdgeThreshold, SHADER_UNIFORM_FLOAT);
    SetShaderValue(styleShader, depthViewEnabledLoc, &depthViewEnabled, SHADER_UNIFORM_INT);
    
    // raylib lighting
     // Use MAX_LIGHTS = 4
    lights[0] = CreateLight(LIGHT_POINT, Vector3{ 0.0f, 5.0f, 0.0f }, camera.target, WHITE, lightingShader);
    lights[0].enabled = true;
    UpdateLightValues(lightingShader, lights[0]);
    // TODO: Setup multi canvas rendering for fixing the debug draw issue of it not being culled
    //RenderTexture2D debugCanvas = LoadRenderTextureDepthTex(screenWidth, screenHeight);
    
    camera.position = Vector3( 0.0f, 80.0f, 17.0f);
    camera.target = Vector3( 0.0f, 0.0f, 0.0f );
    camera.up = Vector3( 0.0f, 1.0f, 0.0f );
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    cameraPos[0] = camera.position.x;
    cameraPos[1] = camera.position.y;
    cameraPos[2] = camera.position.z;
    cameraTarget[0] = camera.target.x;
    cameraTarget[1] = camera.target.y;
    cameraTarget[2] = camera.target.z;
    shaderTime = GetTime();
    
    tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); 
    jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
    if (!physicsSystem)
    {
        physicsSystem = new JPH::PhysicsSystem();
    }
    
    physicsSystem->Init(1024, 0, 1024, 1024, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);
    physicsSystem->SetGravity(JPH::Vec3(0.0f, -98.1f, 0.0f));
    myContactListener = new MyContactListener();
    physicsSystem->SetContactListener(myContactListener);
}

ProjectSettings::~ProjectSettings()
{
    delete myContactListener;
    delete physicsSystem;
    delete jobSystem;
    delete tempAllocator;
    
    UnloadShader(lightingShader);
    UnloadShader(styleShader);
    UnloadRenderTextureDepthTex(mainRenderCanvas);
}

// basic camera controls
void ProjectSettings::MoveCamera()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        float dt = GetFrameTime();
        float moveAmount = panSpeed * dt; 
        
        const Vector2 mouseDelta = GetMouseDelta();
        if (mouseDelta.x != 0.0f) 
        {
            CameraYaw(&camera,-mouseDelta.x * moveAmount, rotateAroundTarget);
        }
        
        if (mouseDelta.y != 0.0f)
        {
            CameraPitch(&camera, mouseDelta.y * moveAmount, lockView, rotateAroundTarget, rotateUp);
        }
    }
    if (GetMouseWheelMove() < 0) CameraMoveToTarget(&camera, camreaZoomSpeed * GetFrameTime());
    if (GetMouseWheelMove() > 0) CameraMoveToTarget(&camera, -camreaZoomSpeed * GetFrameTime());
}

void ProjectSettings::ResizeCanvas()
{
    UnloadRenderTextureDepthTex(mainRenderCanvas);
    
    mainRenderCanvas = LoadRenderTextureDepthTex(GetScreenWidth(), GetScreenHeight());
    depthLoc = GetShaderLocation(styleShader, "texture1");
}

// Create a light and get shader locations
Light ProjectSettings::CreateLight(int type, Vector3 position, Vector3 target, Color color, Shader shader)
{
    Light light = { 0 };

    if (lightsCount < MAX_LIGHTS)
    {
        light.enabled = true;
        light.type = type;
        light.position = position;
        light.target = target;
        light.color = color;

        // NOTE: Lighting shader naming must be the provided ones
        light.enabledLoc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", lightsCount));
        light.typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", lightsCount));
        light.positionLoc = GetShaderLocation(shader, TextFormat("lights[%i].position", lightsCount));
        light.targetLoc = GetShaderLocation(shader, TextFormat("lights[%i].target", lightsCount));
        light.colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", lightsCount));

        UpdateLightValues(shader, light);
        
        lightsCount++;
    }

    return light;
}

// Send light properties to shader
// NOTE: Light shader locations should be available 
void ProjectSettings::UpdateLightValues(Shader shader, Light light)
{
    // Send to shader light enabled state and type
    SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

    // Send to shader light position values
    float position[3] = { light.position.x, light.position.y, light.position.z };
    SetShaderValue(shader, light.positionLoc, position, SHADER_UNIFORM_VEC3);

    // Send to shader light target position values
    float target[3] = { light.target.x, light.target.y, light.target.z };
    SetShaderValue(shader, light.targetLoc, target, SHADER_UNIFORM_VEC3);

    // Send to shader light color values
    float color[4] = { (float)light.color.r/(float)255, (float)light.color.g/(float)255, 
                       (float)light.color.b/(float)255, (float)light.color.a/(float)255 };
    SetShaderValue(shader, light.colorLoc, color, SHADER_UNIFORM_VEC4);
}
void ProjectSettings::updateDepthTexture()
{
    SetShaderValueTexture(styleShader, depthLoc, mainRenderCanvas.depth);
}

void ProjectSettings::updateLightShader()
{
    cameraPos[0] = camera.position.x;
    cameraPos[1] = camera.position.y;
    cameraPos[2] = camera.position.z;
    cameraTarget[0] = camera.target.x;
    cameraTarget[1] = camera.target.y;
    cameraTarget[2] = camera.target.z;
    shaderTime = GetTime();

    SetShaderValue(lightingShader, timeLoc, &shaderTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightingShader, viewEyeLoc, cameraPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(lightingShader, viewCenterLoc, cameraTarget, SHADER_UNIFORM_VEC3);
    SetShaderValue(lightingShader, ambientLoc, &ambientStrength, SHADER_UNIFORM_VEC4);
    SetShaderValue(lightingShader, toonLoc, &toonEnabled, SHADER_UNIFORM_INT);
}

void ProjectSettings::toggleLights()
{
    lights[0].enabled = !lights[0].enabled;
    UpdateLightValues(lightingShader, lights[0]);
}

void ProjectSettings::toggleToon()
{
    toonEnabled = toonEnabled ? 0 : 1;
}

void ProjectSettings::toggleDepthBufferView()
{
    depthViewEnabled = depthViewEnabled ? 0 : 1;
    SetShaderValue(styleShader, depthViewEnabledLoc, &depthViewEnabled, SHADER_UNIFORM_INT);
}

void ProjectSettings::updateNoiseAmount(float nAmount)
{
    NoiseAmount = nAmount;
    SetShaderValue(styleShader, noiseAmountLoc, &NoiseAmount, SHADER_UNIFORM_FLOAT);
}
void ProjectSettings::updateErrorPeriod(float ePeriodAmt)
{
    ErrorPeriod = ePeriodAmt;
    SetShaderValue(styleShader, errorPeriodLoc, &ErrorPeriod, SHADER_UNIFORM_FLOAT);
}
void ProjectSettings::updateErrorRange(float eRangeAmt)
{
    ErrorRange = eRangeAmt;
    SetShaderValue(styleShader, errorRangeLoc, &ErrorRange, SHADER_UNIFORM_FLOAT);
}
void ProjectSettings::updateLineColorBlend(float lineColorAmt)
{
    LineColorBlendAmount = lineColorAmt;
    SetShaderValue(styleShader, lineColorBlendLoc, &LineColorBlendAmount, SHADER_UNIFORM_FLOAT);
}

void ProjectSettings::updateLineColor(float NewColor[3])
{
    Vector3 AdjustedColor = Vector3{NewColor[0], NewColor[1], NewColor[2]};
    SetShaderValue(styleShader, edgeColorLoc, &AdjustedColor, SHADER_UNIFORM_VEC3);
}

void ProjectSettings::toggleSobel(bool toggledSobel)
{
    bIsSobelLineEnabled = toggledSobel ? 1 : 0;
    SetShaderValue(styleShader, toggleSobelLoc, &bIsSobelLineEnabled, SHADER_UNIFORM_INT);
}

void ProjectSettings::toggleDepthLine(bool toggledDepth)
{
    bIsDepthLineEnabled = toggledDepth ? 1 : 0;
    SetShaderValue(styleShader, toggleDepthLoc, &bIsDepthLineEnabled, SHADER_UNIFORM_INT);
}

void ProjectSettings::updateNearPlane(float nearPlaneVal)
{
    depthNearPlane = nearPlaneVal;
    SetShaderValue(styleShader, depthNearPlaneLoc, &depthNearPlane, SHADER_UNIFORM_FLOAT);
}

void ProjectSettings::updateFarPlane(float farPlaneVal)
{
    depthFarPlane = farPlaneVal;
    SetShaderValue(styleShader, depthFarPlaneLoc, &depthFarPlane, SHADER_UNIFORM_FLOAT);
}

void ProjectSettings::updateSobelKernelSize(float sobelKernelSizeVal)
{
    sobelKernelSize = sobelKernelSizeVal;
    SetShaderValue(styleShader, sobelKernelSizeLoc, &sobelKernelSize, SHADER_UNIFORM_FLOAT);
}

void ProjectSettings::updatedpethSensitivity(float depthSensitivityVal)
{
    depthSensitivity = depthSensitivityVal;
    SetShaderValue(styleShader, depthSensitivityLoc, &depthSensitivity, SHADER_UNIFORM_FLOAT);
}

void ProjectSettings::updateEdgeThresh(float edgeThreshVal)
{
    depthEdgeThreshold = edgeThreshVal;
    SetShaderValue(styleShader, depthEdgeThresholdLoc, &depthEdgeThreshold, SHADER_UNIFORM_FLOAT);
}

void ProjectSettings::updateSobelEdgeThres(float sobelEdgeThresVal)
{
    sobelEdgeThreshold = sobelEdgeThresVal;
    SetShaderValue(styleShader, sobelEdgeThresholdLoc, &sobelEdgeThreshold, SHADER_UNIFORM_FLOAT);
}