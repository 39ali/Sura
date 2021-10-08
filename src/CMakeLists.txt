

get_filename_component(_parent ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)



# Add source to this project's executable.
file(GLOB_RECURSE  src "*.h" "*.cpp" )
# Create the source groups for source tree with root at CMAKE_CURRENT_SOURCE_DIR.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${src})


add_executable (Sura
${src}
"${_parent}/third_party/tracy/TracyClient.cpp"
 )


set_property(TARGET Sura PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Sura>")

target_include_directories(Sura PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(Sura  PUBLIC
glm tinyobjloader imgui stb_image tracy
Vulkan::Vulkan 
)

target_link_libraries(Sura PRIVATE 
vkbootstrap vma SDL2main SDL2-static spdlog 
glslang
)


add_dependencies(Sura Shaders)

##	set c++20
set_property(TARGET Sura PROPERTY CXX_STANDARD 17)
set_property(TARGET Sura PROPERTY CXX_STANDARD_REQUIRED ON)

##  set pch
target_precompile_headers(Sura PRIVATE surapch.h)


if(MSVC)
	message("building with multiple processors")
	target_compile_options(Sura PRIVATE "/MP")
endif()
