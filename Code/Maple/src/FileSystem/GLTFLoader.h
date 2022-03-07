//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "MeshLoader.h"

namespace maple 
{
	class GLTFLoader : public ModelLoader
	{
	public:
		static constexpr char* EXTENSIONS[] = {"gltf","glb"};
		auto load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>&)-> void override;
	};
};
