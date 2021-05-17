find_package(Vulkan REQUIRED)

add_library(vkbootstrap STATIC)
add_library(glm INTERFACE)
add_library(vma INTERFACE)

add_library(stb_image INTERFACE)

add_library(tinyobjloader STATIC)

target_sources(vkbootstrap PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap/VkBootstrap.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap/VkBootstrap.cpp"
)

target_include_directories(vkbootstrap PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap" )
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan)

##tracy 
add_library(tracy INTERFACE)
target_include_directories(tracy INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tracy")

#target_sources(tracy PRIVATE 
 #   "${CMAKE_CURRENT_SOURCE_DIR}/tracy/TracyClient.cpp"
#)


##

#both vma and glm and header only libs so we only need the include path
target_include_directories(vma INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vma" )

target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm" )

target_sources(tinyobjloader PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader/tiny_obj_loader.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader/tiny_obj_loader.cc"
)

target_include_directories(tinyobjloader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader" )

##sdl2
message("building SDL2")
set(SDL_STATIC ON CACHE BOOL "Build a static version of SDL2")
set( SDL_SHARED OFF CACHE BOOL "" FORCE )
add_subdirectory(SDL2)



## imgui
add_library(imgui STATIC)

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

target_sources(imgui PRIVATE 
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_vulkan.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_sdl.cpp"
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL2main SDL2-static)


target_include_directories(stb_image INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb_image" )





target_compile_features(vkbootstrap PRIVATE cxx_std_17)
if(MSVC)
	target_compile_options(imgui PRIVATE "/MP")
    target_compile_options(tinyobjloader PRIVATE "/MP")
    target_compile_options(SDL2main PRIVATE "/MP")
    target_compile_options(SDL2-static PRIVATE "/MP")
    target_compile_options(vkbootstrap PRIVATE "/MP")
endif()

