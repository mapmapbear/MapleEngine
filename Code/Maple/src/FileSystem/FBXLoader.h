//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "MeshLoader.h"
#include <string>
#include <unordered_map>

namespace maple 
{
	class Mesh;
	class FBXLoader : public ModelLoader
	{
	public:
		static constexpr char* EXTENSIONS[] = {"fbx"};
		auto load(const std::string& obj, const std::string& extension, std::unordered_map<std::string, std::shared_ptr<Mesh>>&, std::shared_ptr<Skeleton>& skeleton)-> void override;
	};
};
