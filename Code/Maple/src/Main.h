//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Application.h"
#include "Others/Console.h"

extern Maple::Application *createApplication();

auto main() -> int32_t
{
	Maple::Console::init();
	Maple::Application::app = createApplication();
	auto retCode            = Maple::Application::app->start();
	delete Maple::Application::app;
	return retCode;
}
