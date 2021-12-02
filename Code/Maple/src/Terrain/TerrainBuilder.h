//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <vector>
#include "FileSystem/Image.h"
#include "Terrain/QuadCollapseMesh.h"


namespace maple 
{
	class TerrainBuilder 
	{
	public:
		TerrainBuilder(const std::string& filePath);
		auto build()->std::shared_ptr<QuadCollapseMesh>;
	private:
		std::unique_ptr<Image> heightMap;
		std::string name;
	};
};