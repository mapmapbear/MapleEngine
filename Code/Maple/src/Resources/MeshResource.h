//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <unordered_map>
#include "Resources.h"
#include "Engine/Core.h"

namespace Maple
{
	class Mesh;

	class MAPLE_EXPORT MeshResource : public Resources<MeshResource>
	{
	public:
		MeshResource(const std::string& name);
		auto addMesh(const std::string& name, Mesh* mesh) -> void;
		auto addMesh(const std::string& name, std::shared_ptr<Mesh> mesh) -> void;
		auto find(const std::string& name)->std::shared_ptr<Mesh>;
		inline auto& getMeshes() const { return meshes; }
	private:
		std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
		std::string name;
	};
};