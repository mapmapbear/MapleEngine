//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

class btRigidBody;

namespace maple
{
	namespace physics
	{
		namespace component
		{
			struct RigidBody
			{
				btRigidBody* rigidbody = nullptr;
				bool dynamic = false;
				bool kinematic = false;
				float mass = 1.0;
//----------------------------------------------------//
				glm::vec3 localInertia;
				glm::vec3 worldCenterPositionMass;
				glm::vec3 velocity;
				glm::vec3 angularVelocity;
			};
		}
	}
}