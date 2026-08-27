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
    
protected:
	void initializeObjects();
	void loadMeshes();
    
	JPH::PhysicsSystem* pSystem = nullptr;
	MyContactListener* pContactListener = nullptr; 
	
	Mesh bPlaneMesh;
	Model bPlaneRenderModel;
	JPH::BodyID bPlaneBodyID;
	
	std::vector<RenderObjects> physicsBodies;
	Color ball_color_[7] = {BLUE, BROWN, RED, YELLOW, ORANGE, PURPLE, GRAY};
};
