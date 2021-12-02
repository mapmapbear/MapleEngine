//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Engine/Mesh.h"


namespace maple 
{
	namespace MeshLoader
	{
		auto load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>&)-> void;
	};
};
