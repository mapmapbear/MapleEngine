//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btTransform.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

namespace glm
{
	template <class Archive>
	void serialize(Archive &archive, vec2 &v)
	{
		archive(v.x, v.y);
	}
	template <class Archive>
	void serialize(Archive &archive, vec3 &v)
	{
		archive(v.x, v.y, v.z);
	}
	template <class Archive>
	void serialize(Archive &archive, vec4 &v)
	{
		archive(v.x, v.y, v.z, v.w);
	}
	template <class Archive>
	void serialize(Archive &archive, ivec2 &v)
	{
		archive(v.x, v.y);
	}
	template <class Archive>
	void serialize(Archive &archive, ivec3 &v)
	{
		archive(v.x, v.y, v.z);
	}
	template <class Archive>
	void serialize(Archive &archive, ivec4 &v)
	{
		archive(v.x, v.y, v.z, v.w);
	}
	template <class Archive>
	void serialize(Archive &archive, uvec2 &v)
	{
		archive(v.x, v.y);
	}
	template <class Archive>
	void serialize(Archive &archive, uvec3 &v)
	{
		archive(v.x, v.y, v.z);
	}
	template <class Archive>
	void serialize(Archive &archive, uvec4 &v)
	{
		archive(v.x, v.y, v.z, v.w);
	}
	template <class Archive>
	void serialize(Archive &archive, dvec2 &v)
	{
		archive(v.x, v.y);
	}
	template <class Archive>
	void serialize(Archive &archive, dvec3 &v)
	{
		archive(v.x, v.y, v.z);
	}
	template <class Archive>
	void serialize(Archive &archive, dvec4 &v)
	{
		archive(v.x, v.y, v.z, v.w);
	}
	// glm matrices serialization
	template <class Archive>
	void serialize(Archive &archive, mat2 &m)
	{
		archive(m[0], m[1]);
	}
	template <class Archive>
	void serialize(Archive &archive, dmat2 &m)
	{
		archive(m[0], m[1]);
	}
	template <class Archive>
	void serialize(Archive &archive, mat3 &m)
	{
		archive(m[0], m[1], m[2]);
	}
	template <class Archive>
	void serialize(Archive &archive, mat4 &m)
	{
		archive(m[0], m[1], m[2], m[3]);
	}
	template <class Archive>
	void serialize(Archive &archive, dmat4 &m)
	{
		archive(m[0], m[1], m[2], m[3]);
	}

	template <class Archive>
	void serialize(Archive &archive, quat &q)
	{
		archive(q.x, q.y, q.z, q.w);
	}
	template <class Archive>
	void serialize(Archive &archive, dquat &q)
	{
		archive(q.x, q.y, q.z, q.w);
	}

};        // namespace glm

namespace maple
{
	class Scene;
	class Material;

	namespace Serialization
	{
		auto MAPLE_EXPORT serialize(Scene *scene) -> void;
		auto MAPLE_EXPORT loadScene(Scene *scene, const std::string &file) -> void;
		auto MAPLE_EXPORT loadMaterial(Material *material, const std::string &path) -> void;
		auto MAPLE_EXPORT serialize(Material *material) -> void;

		inline auto bulletToGlm(const btVector3 &v)
		{
			return glm::vec3(v.getX(), v.getY(), v.getZ());
		}
		inline auto glmToBullet(const glm::vec3 &v)
		{
			return btVector3(v.x, v.y, v.z);
		}
		inline auto bulletToGlm(const btQuaternion &q)
		{
			return glm::quat(q.getW(), q.getX(), q.getY(), q.getZ());
		}
		inline auto glmToBullet(const glm::quat &q)
		{
			return btQuaternion(q.x, q.y, q.z, q.w);
		}
		inline auto glmToBullet(const glm::mat3 &m)
		{
			return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]);
		}
		inline auto glmToBullet(const glm::mat4 &m)
		{
			glm::mat3 m3(m);
			return btTransform(glmToBullet(m3), glmToBullet(glm::vec3(m[3][0], m[3][1], m[3][2])));
		}

		inline auto bulletToGlm(const btTransform &t)
		{
			glm::mat4 m(1);

			const btMatrix3x3 &basis = t.getBasis();
			// rotation
			for (int32_t r = 0; r < 3; r++)
			{
				for (int32_t c = 0; c < 3; c++)
				{
					m[c][r] = basis[r][c];
				}
			}
			// translation
			btVector3 origin = t.getOrigin();
			m[3][0]          = origin.getX();
			m[3][1]          = origin.getY();
			m[3][2]          = origin.getZ();
			// unit scale
			m[0][3] = 0.0f;
			m[1][3] = 0.0f;
			m[2][3] = 0.0f;
			m[3][3] = 1.0f;
			return m;
		}
	};        // namespace Serialization
};            // namespace maple
