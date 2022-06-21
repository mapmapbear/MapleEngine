//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Loader.h"

namespace maple
{
	class GLTFLoader : public AssetsLoader
	{
	  public:
		static constexpr char *EXTENSIONS[] = {"gltf", "glb"};
		auto                   load(const std::string &fileName, const std::string &extension, std::vector<std::shared_ptr<IResource>> &out) const -> void override;
	};
};        // namespace maple
