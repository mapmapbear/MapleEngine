//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderDocExt.h"

#ifdef _WIN32
#	include <windows.h>
#endif

#include "Engine/Core.h"
#include "Others/Console.h"

namespace maple
{
	auto RenderDocExt::openLib() -> void
	{
#ifdef _WIN32
		if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
		{
			pRENDERDOC_GetAPI RENDERDOC_GetAPI =
			    (pRENDERDOC_GetAPI) GetProcAddress(mod, "RENDERDOC_GetAPI");
			int32_t ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **) &doc);
			if (ret != 1)
			{
				LOGW("Unable to load render doc libs");
			}
		}
#else

		// At init, on linux/android.
		// For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
		if (void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
		{
			pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI) dlsym(mod, "RENDERDOC_GetAPI");
			int               ret              = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **) &doc);
			assert(ret == 1);
		}
#endif        // _WIN32
	}

	auto RenderDocExt::startCapture() -> void
	{
		if (doc && enable)
			doc->StartFrameCapture(NULL, NULL);
	}

	auto RenderDocExt::endCapture() -> void
	{
		if (doc && enable)
			doc->EndFrameCapture(NULL, NULL);
	}
};        // namespace maple
