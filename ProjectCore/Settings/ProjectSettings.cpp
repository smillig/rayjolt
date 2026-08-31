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
        mObjectToBroadPhase[Layers::STATIC] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::SENSOR] = BroadPhaseLayers::NON_MOVING;
        
        mObjectToBroadPhase[Layers::DYNAMIC] = BroadPhaseLayers::NON_MOVING;
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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Raylib and Jolt Shader Fun");
    SetTargetFPS(framerateTarget);
    
    // Setup for shaders
    mainRenderCanvas = LoadRenderTextureDepthTex(screenWidth, screenHeight);
    cavityShader = LoadShader(0, "Assets/Shaders/Toon/toon.frag");
    // TODO: Setup multi canvas rendering for fixing the debug draw issue of it not being culled
    //RenderTexture2D debugCanvas = LoadRenderTextureDepthTex(screenWidth, screenHeight);
    
    camera.position = Vector3( 0.0f, 80.0f, 17.0f);
    camera.target = Vector3( 0.0f, 0.0f, 0.0f );
    camera.up = Vector3( 0.0f, 1.0f, 0.0f );
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
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
    
    UnloadShader(cavityShader);
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
}
