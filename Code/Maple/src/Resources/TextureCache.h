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
	class Texture;

	class MAPLE_EXPORT TextureCache : public Resources<Texture> {};
};