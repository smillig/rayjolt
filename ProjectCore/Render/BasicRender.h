#pragma once

#include <raylib.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

class MyContactListener;

struct RenderObjects
{
	JPH::BodyID bodyID {};
	Model model {};
	Color color {};
};

class BasicRender
{
public:
	BasicRender(JPH::PhysicsSystem* instancedPhysicsSystem, MyContactListener* instanceContactListener);
	~BasicRender();
	void update(float deltaTime);
	void renderGroundPlane() const;
	void renderDynamics() const;
	void renderDebug() const;
	void updateModelShader(Shader newShader);
    
protected:
	void initializeObjects();
	void loadMeshes();
    
	JPH::PhysicsSystem* pSystem = nullptr;
	MyContactListener* pContactListener = nullptr; 
	
	Mesh bPlaneMesh;
	Model bPlaneRenderModel;
	Mesh NorthWallMesh;
	Model NorhtWallModel;
	Mesh SouthWallMesh;
	Model SouthWallModel;
	Mesh EastWallMesh;
	Model EastWallModel;
	Mesh WestWallMesh;
	Model WestWallModel;
	JPH::BodyID bPlaneBodyID;
	JPH::BodyID bWallNorthBodyID;
	JPH::BodyID bWallSouthBodyID;
	JPH::BodyID bWallEastBodyID;
	JPH::BodyID bWallWestBodyID;
	
	std::vector<RenderObjects> physicsBodies;
	Color ball_color_[7] = {BLUE, BROWN, RED, YELLOW, ORANGE, PURPLE, GRAY};
};
