#include "BasicRender.h"
#include "Settings/ProjectSettings.h"
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include "Jolt/Physics/Body/BodyInterface.h"
#include "raymath.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"

BasicRender::BasicRender(JPH::PhysicsSystem* instancedPhysicsSystem, MyContactListener* instanceContactListener)
{
	pSystem = instancedPhysicsSystem;
	pContactListener = instanceContactListener;
	initializeObjects();
}

BasicRender::~BasicRender()
{
	UnloadModel(bPlaneRenderModel);
	
	pSystem->GetBodyInterface().RemoveBody(bPlaneBodyID);
	pSystem->GetBodyInterface().DestroyBody(bPlaneBodyID);
	if (physicsBodies.size() > 0)
	{
		for (RenderObjects& pBody : physicsBodies)
		{
			pSystem->GetBodyInterface().RemoveBody(pBody.bodyID);
			pSystem->GetBodyInterface().DestroyBody(pBody.bodyID);
			UnloadModel(pBody.model);
		}
	}
	pSystem->GetBodyInterface().RemoveBody(bWallNorthBodyID);
	pSystem->GetBodyInterface().DestroyBody(bWallNorthBodyID);
	pSystem->GetBodyInterface().RemoveBody(bWallSouthBodyID);
	pSystem->GetBodyInterface().DestroyBody(bWallSouthBodyID);
	pSystem->GetBodyInterface().RemoveBody(bWallEastBodyID);
	pSystem->GetBodyInterface().DestroyBody(bWallEastBodyID);
	pSystem->GetBodyInterface().RemoveBody(bWallWestBodyID);
	pSystem->GetBodyInterface().DestroyBody(bWallWestBodyID);
	pSystem->GetBodyInterface().RemoveBody(bAgitatorBodyID);
	pSystem->GetBodyInterface().DestroyBody(bAgitatorBodyID);

	UnloadModel(NorhtWallModel);
	UnloadModel(SouthWallModel);
	UnloadModel(EastWallModel);
	UnloadModel(WestWallModel);
}

void BasicRender::initializeObjects()
{
	JPH::BodyInterface& bodyInterface = pSystem->GetBodyInterface();
	// generate a square mesh with raylib then a model from the mesh
	bPlaneMesh = GenMeshCube(100.0f, 5.0f, 100.0f);
	bPlaneRenderModel = LoadModelFromMesh(bPlaneMesh);
	// create floor plane physics object at the same location (jolt uses half extents)
	JPH::BoxShapeSettings bPlaneShapeSettings(JPH::Vec3(50.0f, 2.5f, 50.0f));
	JPH::ShapeRefC bPlaneShape = bPlaneShapeSettings.Create().Get();
	// create the object in world space and assign it as a static object on the static layer
	JPH::BodyCreationSettings bPlaneSettings(bPlaneShape, JPH::Vec3{0.0f, -2.5f, 0.0f}, JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::STATIC);
	bPlaneBodyID = bodyInterface.CreateAndAddBody(bPlaneSettings, JPH::EActivation::DontActivate);
	// Create front and back walls
	NorthWallMesh = GenMeshCube(100.0f, 105.0f, 2.0f);
	NorhtWallModel = LoadModelFromMesh(NorthWallMesh);
	JPH::BoxShapeSettings bWallShapeSettings(JPH::Vec3(50.0f, 52.5f, 1.0f));
	JPH::ShapeRefC bWallNorthShape = bWallShapeSettings.Create().Get();
	JPH::BodyCreationSettings bWallNorthSettings(bWallNorthShape, JPH::Vec3{0.0f, 50.0f, 51.0f}, JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::STATIC);
	bWallNorthBodyID = bodyInterface.CreateAndAddBody(bWallNorthSettings, JPH::EActivation::DontActivate);
	SouthWallMesh = GenMeshCube(100.0f, 105.0f, 2.0f);
	SouthWallModel = LoadModelFromMesh(SouthWallMesh);
	JPH::ShapeRefC bWallSouthShape = bWallShapeSettings.Create().Get();
	JPH::BodyCreationSettings bWallSouthSettings(bWallSouthShape, JPH::Vec3{0.0f, 50.0f, -51.0f}, JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::STATIC);
	bWallSouthBodyID = bodyInterface.CreateAndAddBody(bWallSouthSettings, JPH::EActivation::DontActivate);
	// Walls to the left and right
	EastWallMesh = GenMeshCube(2.0f, 105.0f, 104.0f);
	EastWallModel = LoadModelFromMesh(EastWallMesh);
	JPH::BoxShapeSettings bWallEWShapeSettings(JPH::Vec3(1.0f, 52.5f, 52.0f));
	JPH::ShapeRefC bWallEastShape = bWallEWShapeSettings.Create().Get();
	JPH::BodyCreationSettings bWallEastSettings(bWallEastShape, JPH::Vec3{51.0f, 50.0f, 0.0f}, JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::STATIC);
	bWallEastBodyID = bodyInterface.CreateAndAddBody(bWallEastSettings, JPH::EActivation::DontActivate);
	WestWallMesh = GenMeshCube(2.0f, 105.0f, 104.f);
	WestWallModel = LoadModelFromMesh(WestWallMesh);
	JPH::ShapeRefC bWallWestShape = bWallEWShapeSettings.Create().Get();
	JPH::BodyCreationSettings bWallWestSettings(bWallWestShape, JPH::Vec3{-51.0f, 50.0f, 0.0f}, JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::STATIC);
	bWallWestBodyID = bodyInterface.CreateAndAddBody(bWallWestSettings, JPH::EActivation::DontActivate);

	AgitatorMesh = GenMeshCube(145.0f, 0.5f, 1.0f);
	AgitatorModel = LoadModelFromMesh(AgitatorMesh);
	JPH::BoxShapeSettings bAgitatorShapeSettings(JPH::Vec3(72.5f, 0.25f, 0.5f));
	JPH::ShapeRefC bAgitatorShape = bAgitatorShapeSettings.Create().Get();
	JPH::BodyCreationSettings bAgitatorSettings(bAgitatorShape, JPH::Vec3{0.0f, 0.5f, 0.0f}, JPH::Quat::sIdentity(), JPH::EMotionType::Kinematic, Layers::STATIC);
	bAgitatorBodyID = bodyInterface.CreateAndAddBody(bAgitatorSettings, JPH::EActivation::Activate);
	bodyInterface.SetGravityFactor(bAgitatorBodyID, 0.0f);
	bodyInterface.SetAngularVelocity(bAgitatorBodyID, JPH::Vec3(0.0f, 1.5f, 0.0f));
	bodyInterface.ActivateBody(bAgitatorBodyID);
	
	
	for (Color curColor : ball_color_)
	{
		Vector3 spawnPos = Vector3((float)GetRandomValue(1, 10), (float)GetRandomValue(1, 10), (float)GetRandomValue(1, 10));
		Mesh ColoredSphereMesh = GenMeshSphere(2.0f, 16, 16);
		Model ColoredSphereModel = LoadModelFromMesh(ColoredSphereMesh);
		
		JPH::SphereShapeSettings ColoredSphereShapeSettings(2.0f);
		JPH::ShapeRefC ColoredSphereShape = ColoredSphereShapeSettings.Create().Get();
		JPH::BodyCreationSettings ColoredSphereSettings(ColoredSphereShape, JPH::Vec3{spawnPos.x, spawnPos.y, spawnPos.z}, JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::DYNAMIC);
		
		JPH::BodyID SphereBodyID = bodyInterface.CreateAndAddBody(ColoredSphereSettings, JPH::EActivation::Activate);
		RenderObjects CurrentSphere = RenderObjects{ SphereBodyID, ColoredSphereModel, curColor };
		physicsBodies.push_back(CurrentSphere);
	}
}

// for when assets are loaded from gltf files
void BasicRender::loadMeshes()
{
	
}

void BasicRender::updateModelShader(Shader newShader)
{
	for (RenderObjects body : physicsBodies)
	{
		body.model.materials[0].shader = newShader;
	}
	bPlaneRenderModel.materials[0].shader = newShader;
}

// draw outline of physic objects to make sure they aligh with render objects
void BasicRender::renderDebug() const
{
	if (IsKeyDown(KEY_F3)) 
	{
		JPH::BodyInterface& bodyInterface = pSystem->GetBodyInterface();
		
		JPH::Vec3 bPlanePos = bodyInterface.GetPosition(bPlaneBodyID);
		DrawModelWires(bPlaneRenderModel, Vector3{bPlanePos.GetX(), bPlanePos.GetY(), bPlanePos.GetZ()}, 1.0f, RED);
		// draw invisable walls
		JPH::Vec3 bWallNorthPos = bodyInterface.GetPosition(bWallNorthBodyID);
		DrawModelWires(NorhtWallModel, Vector3{bWallNorthPos.GetX(), bWallNorthPos.GetY(), bWallNorthPos.GetZ()}, 1.0f, RED);
		JPH::Vec3 bWallSouthPos = bodyInterface.GetPosition(bWallSouthBodyID);
		DrawModelWires(SouthWallModel, Vector3{bWallSouthPos.GetX(), bWallSouthPos.GetY(), bWallSouthPos.GetZ()}, 1.0f, RED);
		JPH::Vec3 bWallEasthPos = bodyInterface.GetPosition(bWallEastBodyID);
		DrawModelWires(EastWallModel, Vector3{bWallEasthPos.GetX(), bWallEasthPos.GetY(), bWallEasthPos.GetZ()}, 1.0f, RED);
		JPH::Vec3 bWallWestPos = bodyInterface.GetPosition(bWallWestBodyID);
		DrawModelWires(WestWallModel, Vector3{bWallWestPos.GetX(), bWallWestPos.GetY(), bWallWestPos.GetZ()}, 1.0f, RED);
		JPH::Vec3 bAgitatorPos = bodyInterface.GetPosition(bAgitatorBodyID);
		JPH::Quat bAgitatorRot = bodyInterface.GetRotation(bAgitatorBodyID);
		Vector3 raylibPos = { bAgitatorPos.GetX(), bAgitatorPos.GetY(), bAgitatorPos.GetZ() };
		Quaternion raylibQuat = { bAgitatorRot.GetX(), bAgitatorRot.GetY(), bAgitatorRot.GetZ(), bAgitatorRot.GetW() };
		
		Vector3 rotationAxis = Vector3{0.0f, 1.0f, 0.0f};
		float AgitatorRotDegrees;
		QuaternionToAxisAngle(raylibQuat, &rotationAxis, &AgitatorRotDegrees);
		AgitatorRotDegrees *= RAD2DEG;
		DrawModelWiresEx(AgitatorModel, raylibPos, rotationAxis, AgitatorRotDegrees, Vector3{1.0f, 1.0f, 1.0f}, BLUE);
		
		for (const RenderObjects& pBodies : physicsBodies)
		{
			JPH::RefConst<JPH::Shape> pBodyShape = bodyInterface.GetShape(pBodies.bodyID);
			JPH::Vec3 pBodyPos = bodyInterface.GetPosition(pBodies.bodyID);

			
			JPH::Quat pBodyRot = bodyInterface.GetRotation(pBodies.bodyID);

			if (pBodyShape.GetPtr()->GetSubType() == JPH::EShapeSubType::Sphere)
			{
				const JPH::SphereShape* sphereShape = static_cast<const JPH::SphereShape*>(pBodyShape.GetPtr());

				float radius = sphereShape->GetRadius();

				DrawSphereWires(Vector3{ pBodyPos.GetX(), pBodyPos.GetY(), pBodyPos.GetZ() }, radius, 8, 8, GREEN);
			}
			else if (pBodyShape.GetPtr()->GetSubType() == JPH::EShapeSubType::Box)
			{
				const JPH::BoxShape* boxShape = static_cast<const JPH::BoxShape*>(pBodyShape.GetPtr());

				JPH::Vec3 halfExtents = boxShape->GetHalfExtent();

				DrawCubeWires(
					Vector3{ pBodyPos.GetX(), pBodyPos.GetY(), pBodyPos.GetZ() },
					halfExtents.GetX() * 2.0f, 
					halfExtents.GetY() * 2.0f, 
					halfExtents.GetZ() * 2.0f, 
					GREEN
				);
			}
		}
	}
}

// function to render the ground plane
void BasicRender::renderGroundPlane() const
{
	DrawModel(bPlaneRenderModel, Vector3{0.0f, -2.5f, 0.0f}, 1.0f, GREEN);
}

// render objects on the dynamic layer
void BasicRender::renderDynamics() const
{
    for (const RenderObjects& pBodies : physicsBodies)
    {
        JPH::BodyInterface& bodyInterface = pSystem->GetBodyInterface();
        JPH::Vec3 joltPos = bodyInterface.GetPosition(pBodies.bodyID);
        JPH::Quat joltRot = bodyInterface.GetRotation(pBodies.bodyID);

        Vector3 raylibPos = { joltPos.GetX(), joltPos.GetY(), joltPos.GetZ() };
        Quaternion raylibQuat = { joltRot.GetX(), joltRot.GetY(), joltRot.GetZ(), joltRot.GetW() };
    
        Vector3 rotationAxis;
        float rotationAngleDegrees;
        QuaternionToAxisAngle(raylibQuat, &rotationAxis, &rotationAngleDegrees);
        rotationAngleDegrees *= RAD2DEG;
        
        DrawModelEx(pBodies.model, raylibPos, rotationAxis, rotationAngleDegrees, Vector3{1.0f, 1.0f, 1.0f}, pBodies.color);
    }
}

// listen for physics collisions
void BasicRender::update(float deltaTime)
{
	if (!pContactListener->bodiesToProcess.empty())
	{
		JPH::BodyInterface& bodyInterface = pSystem->GetBodyInterface();
		for (JPH::BodyID contactID : pContactListener->bodiesToProcess)
		{
			 TraceLog(LOG_INFO, "Body ID: %i has changed hit a sensor.", contactID);
		}
		pContactListener->bodiesToProcess.clear();
	}
}

// callback function for sensors
void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	// Check which body is the Sensor and which is the Ball
	bool isBody1Sensor = inBody1.GetObjectLayer() == Layers::SENSOR;
	bool isBody2Sensor = inBody2.GetObjectLayer() == Layers::SENSOR;

	if (isBody1Sensor || isBody2Sensor) 
	{
		// Identify the BALL (the one that ISN'T the sensor)
		JPH::BodyID bID = isBody1Sensor ? inBody2.GetID() : inBody1.GetID();
        
		// Ensure we are actually grabbing a ball (and not the table slate accidentally)
		JPH::ObjectLayer dynamicLayer = isBody1Sensor ? inBody2.GetObjectLayer() : inBody1.GetObjectLayer();
        
		if (dynamicLayer == Layers::DYNAMIC)
		{
			// Thread-Safe Lock! We lock the mutex so two threads don't write to the vector at the exact same time
			std::lock_guard<std::mutex> lock(queueMutex);
            
			// Add the ball to our queue to be processed later!
			bodiesToProcess.push_back(bID);
		}
	}
}