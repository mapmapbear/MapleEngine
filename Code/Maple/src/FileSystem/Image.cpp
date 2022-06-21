//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Image.h"
#include "Loaders/ImageLoader.h"

namespace maple
{
	Image::Image(const std::string &fileName) :
	    fileName(fileName)
	{
		ImageLoader::loadAsset(fileName, this);
	}

};        // namespace maple
