//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "EventHandler.h"
#include "EventDispatcher.h"


namespace Maple 
{
	EventHandler::~EventHandler()
	{
		if (eventDispatcher)
			eventDispatcher->removeEventHandler(this);
	}

	auto EventHandler::remove() -> void
	{
		if (eventDispatcher)
		{
			eventDispatcher->removeEventHandler(this);
			eventDispatcher = nullptr;
		}
	}

};
