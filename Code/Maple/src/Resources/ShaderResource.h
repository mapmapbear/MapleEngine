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
	class Shader;
	class MAPLE_EXPORT ShaderResource : public Resources<Shader> { };
};