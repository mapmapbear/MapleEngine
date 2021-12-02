//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////


#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Engine/Core.h"

namespace glm
{
	template<class Archive> void serialize(Archive& archive, vec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, vec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, vec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, ivec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, ivec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, ivec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, uvec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, uvec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, uvec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, dvec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, dvec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, dvec4& v) { archive(v.x, v.y, v.z, v.w); }
	// glm matrices serialization
	template<class Archive> void serialize(Archive& archive, mat2& m) { archive(m[0], m[1]); }
	template<class Archive> void serialize(Archive& archive, dmat2& m) { archive(m[0], m[1]); }
	template<class Archive> void serialize(Archive& archive, mat3& m) { archive(m[0], m[1], m[2]); }
	template<class Archive> void serialize(Archive& archive, mat4& m) { archive(m[0], m[1], m[2], m[3]); }
	template<class Archive> void serialize(Archive& archive, dmat4& m) { archive(m[0], m[1], m[2], m[3]); }

	template<class Archive> void serialize(Archive& archive, quat& q) { archive(q.x, q.y, q.z, q.w); }
	template<class Archive> void serialize(Archive& archive, dquat& q) { archive(q.x, q.y, q.z, q.w); }

};
namespace maple
{
	class Scene;
	class Material;

	class MAPLE_EXPORT Serialization
	{
	  public:
		static auto serialize(Scene *scene) -> void;
		static auto loadScene(Scene *scene, const std::string &file) -> void;
		static auto loadMaterial(Material *material, const std::string &path) -> void;
		static auto serialize(Material *material) -> void;
	};
};
