#pragma once

#include <cstdlib>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::vec3;
using glm::vec4;

#if defined(_MSC_VER)
#	pragma warning( push )
#	pragma warning( disable:4305 )
#endif

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <LinearMath/btVector3.h>

#if defined(_MSC_VER)
#	pragma warning( pop )
#endif

struct Physics
{
	Physics();

	void addBox(const vec3& halfSize, const btQuaternion& orientation, const vec3& position, float mass);

	void update(float deltaSeconds);

	std::vector<mat4> boxTransforms;

private:
	std::vector<std::unique_ptr<btRigidBody>> rigidBodies;

	btDefaultCollisionConfiguration collisionConfiguration;
	btCollisionDispatcher collisionDispatcher;
	btDbvtBroadphase broadphase;
	btSequentialImpulseConstraintSolver solver;
	btDiscreteDynamicsWorld dynamicsWorld;
};