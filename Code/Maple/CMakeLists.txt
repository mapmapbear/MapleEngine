cmake_minimum_required(VERSION 3.4.1)

project(MapleEngine)

get_filename_component(ENGINE_LIB_SRC_DIR
                       ${CMAKE_CURRENT_LIST_DIR}/lib
                       ABSOLUTE)

set(BULLET_PHYSICS_SOURCE_DIR "${ENGINE_LIB_SRC_DIR}/bullet3/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

if(CMAKE_SIZEOF_VOID_P EQUAL 8) 
    message("Using 64-bit glslangValidator")
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message("Using 32-bit glslangValidator")
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()



FILE (STRINGS "lib/bullet3/VERSION" BULLET_VERSION)

add_subdirectory(lib/spdlog)
add_subdirectory(lib/stb_image)
add_subdirectory(lib/tinyobjloader)
add_subdirectory(lib/SPIRV-Cross)
add_subdirectory(lib/ktx)
add_subdirectory(lib/charset-detect)
add_subdirectory(lib/zlib)
add_subdirectory(lib/lua)
add_subdirectory(lib/tracy)
add_subdirectory(lib/glad)
add_subdirectory(lib/nativefiledialog)
add_subdirectory(lib/imgui-node-editor)
add_subdirectory(lib/libiconv)
add_subdirectory(lib/OpenFBX)
add_subdirectory(lib/mio)
add_subdirectory(lib/bullet3/src)


file(GLOB IMGUI_SRC 
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_tables.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_draw.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_widgets.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_widgets_user.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_opengl3.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_opengl3.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/TextEditor.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_vulkan.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_impl_glfw.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/ImGuizmo.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/cimgui.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/imgui_stacklayout.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/implot_items.cpp
	${ENGINE_LIB_SRC_DIR}/imgui/src/implot.cpp
	${ENGINE_LIB_SRC_DIR}/imGuIZMOquat/imGuIZMOquat.cpp
)

file(GLOB_RECURSE SHADERS_GLSL 
	src/Shaders/*.vert 
	src/Shaders/*.frag 
	src/Shaders/*.comp 
	src/Shaders/*.geom 
	src/Shaders/*.tesc 
	src/Shaders/*.tese 
	src/Shaders/*.mesh 
	src/Shaders/*.task 
	src/Shaders/*.rgen
	src/Shaders/*.rchit 
	src/Shaders/*.rmiss
	src/Shaders/*.shader
)

file(GLOB VK_APP_SRC
	src/*.cpp
	src/*.h	
	src/Engine/*.h
	src/Engine/*.cpp
	src/Engine/Renderer/*.cpp
	src/Engine/Renderer/*.h
	src/Engine/LPVGI/*.cpp
	src/Engine/LPVGI/*.h
	src/Engine/VXGI/*.cpp
	src/Engine/VXGI/*.h
	src/Engine/DDGI/*.cpp
	src/Engine/DDGI/*.h
	src/Engine/Raytrace/*.cpp
	src/Engine/Raytrace/*.h
	src/Engine/PathTracer/*.cpp
	src/Engine/PathTracer/*.h
	src/RHI/*.h
	src/RHI/*.cpp
	src/RHI/Vulkan/*.h
	src/RHI/Vulkan/*.cpp
	src/RHI/Vulkan/Raytracing/*.h
	src/RHI/Vulkan/Raytracing/*.cpp
	src/RHI/OpenGL/*.h
	src/RHI/OpenGL/*.inl
	src/RHI/OpenGL/*.cpp
	src/RHI/ImGui/*.h
	src/RHI/ImGui/*.cpp
	src/Event/*.h
	src/Event/*.cpp
	src/Others/*.h
	src/Others/*.cpp
	src/Window/*.h
	src/Window/*.cpp
	src/Devices/*.h
	src/Devices/*.cpp
	src/ImGui/*.cpp
	src/ImGui/*.h
	src/FileSystem/*.cpp
	src/FileSystem/*.h
	src/Terrain/*.cpp
	src/Terrain/*.h
	src/Scene/*.cpp
	src/Scene/*.h
	src/Scene/*.inl
	src/Scripts/*.cpp
	src/Scripts/*.h
	src/Scripts/Mono/*.cpp
	src/Scripts/Mono/*.h
	src/Scripts/Lua/*.cpp
	src/Scripts/Lua/*.h
	src/Scene/Component/*.cpp
	src/Scene/Component/*.h
	src/Scene/Entity/*.cpp
	src/Scene/Entity/*.h
	src/Scene/System/*.h
	src/Scene/System/*.cpp
	src/Resources/*.cpp
	src/Resources/*.h
	src/Math/*.cpp
	src/Math/*.h
	src/Thread/*.cpp
	src/Thread/*.h
	src/Animation/*.cpp
	src/Animation/*.h
	src/Loaders/*.cpp
	src/Loaders/*.h
	src/2d/*.cpp
	src/2d/*.h
	src/Physics/*.cpp
	src/Physics/*.h
)


if(ENGINE_AS_LIBRARY)
	add_definitions(-DMAPLE_DYNAMIC -DMAPLE_ENGINE)
endif()

if(MAPLE_OPENGL)
	add_definitions(-DMAPLE_OPENGL)
endif()

if(MAPLE_VULKAN)
	#add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
	add_definitions(-DMAPLE_VULKAN -DUSE_VMA_ALLOCATOR)
endif()


if (${Target} MATCHES "Windows")

	add_compile_options("/std:c++17")
	add_compile_options("/MP")
	add_definitions(
		-D_CRT_SECURE_NO_WARNINGS 
		-D_CRT_RAND_S 
		-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 
		-DPLATFORM_WINDOWS
		-DPLATFORM_DESKTOP
		-DMAPLE_DEBUG
		-DNOMINMAX #conflict between windows and limits
	)

	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${VK_APP_SRC})

	source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SHADERS_GLSL})

if(ENGINE_AS_LIBRARY)
	add_library(MapleEngine SHARED ${VK_APP_SRC} ${KTX_SOURCES} ${SHADERS_GLSL} ${IMGUI_SRC})
else()
	add_library(MapleEngine STATIC ${VK_APP_SRC} ${KTX_SOURCES} ${SHADERS_GLSL} ${IMGUI_SRC})
endif()

	set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

	
if(MAPLE_VULKAN)
	find_package(Vulkan REQUIRED)
	IF (NOT Vulkan_FOUND)
		message(FATAL_ERROR "Could not find Vulkan library!")
	ELSE()
	    message(STATUS ${Vulkan_LIBRARY})
	ENDIF()

 	target_link_libraries(MapleEngine ${Vulkan_LIBRARY})
endif()

	target_include_directories(MapleEngine PUBLIC
		src
		${ENGINE_LIB_SRC_DIR}/opengl/include
		${ENGINE_LIB_SRC_DIR}/imgui/src
		${ENGINE_LIB_SRC_DIR}/spdlog/include
		${ENGINE_LIB_SRC_DIR}/stb_image
		${ENGINE_LIB_SRC_DIR}/vulkan/include
		${ENGINE_LIB_SRC_DIR}/tinyobjloader
		${ENGINE_LIB_SRC_DIR}/glm
		${ENGINE_LIB_SRC_DIR}/entt
		${ENGINE_LIB_SRC_DIR}/SPIRV-Cross
		${ENGINE_LIB_SRC_DIR}/ktx/include
		${ENGINE_LIB_SRC_DIR}/ktx/other_include
		${ENGINE_LIB_SRC_DIR}/cereal/include
		${ENGINE_LIB_SRC_DIR}/utf8/include
		${ENGINE_LIB_SRC_DIR}/zlib/src
		${ENGINE_LIB_SRC_DIR}/charset-detect
		${ENGINE_LIB_SRC_DIR}/LuaBridge
		${ENGINE_LIB_SRC_DIR}/lua/include
		${ENGINE_LIB_SRC_DIR}/libmono/6.13.0/include
		${ENGINE_LIB_SRC_DIR}/tracy
		${ENGINE_LIB_SRC_DIR}/glad/include
		${ENGINE_LIB_SRC_DIR}/nativefiledialog/include
		${ENGINE_LIB_SRC_DIR}/ecs
		${ENGINE_LIB_SRC_DIR}/imgui-notify
		${ENGINE_LIB_SRC_DIR}/imgui-node-editor
		${ENGINE_LIB_SRC_DIR}/tinygltf
		${ENGINE_LIB_SRC_DIR}/mio
		${ENGINE_LIB_SRC_DIR}/IconFontCppHeaders
		${ENGINE_LIB_SRC_DIR}/bullet3/src
		${ENGINE_LIB_SRC_DIR}/renderDoc
		${ENGINE_LIB_SRC_DIR}/vulkan-mem
		${Vulkan_INCLUDE_DIRS}
	)

	target_link_libraries(MapleEngine 
		${ENGINE_LIB_SRC_DIR}/opengl/lib/${Arch}/glfw3.lib
		${ENGINE_LIB_SRC_DIR}/opengl/lib/${Arch}/opengl32.lib
		${ENGINE_LIB_SRC_DIR}/libmono/6.13.0/windows/${Arch}/libmono-static-sgen.lib
		tinyobjloader
		#imgui
		spirvCross
		ktx
		tellenc
		zlib
		lua
		ws2_32
		Winmm
		Version
		Bcrypt
		TracyClient
		glad
		filedlg
		imgui-node-editor
		iconv
		OpenFBX
		mio
		Bullet3Common
		BulletDynamics
		BulletCollision
		LinearMath
	)


	find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(MapleEngine OpenMP::OpenMP_CXX)
endif()

	set_target_properties(
	OpenFBX lua zlib tinyobjloader 
	tellenc spirvCross ktx 
	spdlog_headers_for_ide 
	stb_image glad TracyClient filedlg imgui-node-editor 
	iconv Bullet3Common BulletDynamics BulletCollision LinearMath BulletInverseDynamics BulletSoftBody PROPERTIES FOLDER Library)
	

foreach(GLSL ${SHADERS_GLSL})

	set(DIR_NAME "")

	string(REPLACE ${CMAKE_CURRENT_LIST_DIR}/src/Shaders "" DIR_NAME ${GLSL})

    set(SPIRV "${LIBRARY_OUTPUT_PATH}/../../Assets/shaders/spv${DIR_NAME}.spv")

	get_filename_component(FILE_NAME ${GLSL} NAME)

	string(REPLACE ${FILE_NAME} "" DIR_NAME ${DIR_NAME})

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIBRARY_OUTPUT_PATH}/../../Assets/shaders/spv${DIR_NAME}"
        COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.2 -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(MapleShader DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(MapleEngine MapleShader)
	
endif()


