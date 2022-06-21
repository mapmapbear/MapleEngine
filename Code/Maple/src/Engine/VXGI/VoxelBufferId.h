//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace maple
{
	namespace VoxelBufferId
	{
		enum Id
		{
			Albedo,
			Normal,
			Radiance,
			Emissive,
			Length
		};

		static const char *Names[] =
		    {
		        "Albedo",
		        "Normal",
		        "Radiance",
		        "Emissive",
		        nullptr};
	}        // namespace VoxelBufferId
}        // namespace maple