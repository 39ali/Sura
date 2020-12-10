

get_filename_component(_parent ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

# Add source to this project's executable.

file(GLOB src "*.h" "*.cpp" "*.c")

add_executable (Sura
${src}
"${_parent}/third_party/tracy/TracyClient.cpp"
 )


set_property(TARGET Sura PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Sura>")

target_include_directories(Sura PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(Sura  vkbootstrap vma glm tinyobjloader imgui stb_image tracy)

target_link_libraries(Sura Vulkan::Vulkan sdl2)

add_dependencies(Sura Shaders)