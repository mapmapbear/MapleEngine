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
				glm::vec3 initialVel;
				bool dynamic = false;
				bool kinematic = false;
				float mass = 1.0;
			};
		}
	}
}