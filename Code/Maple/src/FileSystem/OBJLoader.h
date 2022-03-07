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
	class OBJLoader : public ModelLoader
	{
	public:
		static constexpr char* EXTENSIONS[] = {"obj"};
		auto load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>&)-> void override;
	};
};
