cmake_minimum_required(VERSION 3.8)
project(rabbithole)

set(CMAKE_CXX_STANDARD 20)

# Find packages if needed
find_package(Vulkan REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Vendor
    ${CMAKE_SOURCE_DIR}/extern
    ${CMAKE_SOURCE_DIR}/extern/glfw/include
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/extern/vma
    ${CMAKE_SOURCE_DIR}/extern/assimp/include/
    ${CMAKE_SOURCE_DIR}/extern/slang/
    ${CMAKE_SOURCE_DIR}/extern/optick/include/
    ${CMAKE_SOURCE_DIR}/extern/fsr/include/
)

# Source and header files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

file(GLOB_RECURSE HEADERS 
    "src/*.h"
    "src/*.hpp"
)

# show all files in solution explorer
foreach(_source IN ITEMS ${SOURCES} ${HEADERS})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${CMAKE_SOURCE_DIR}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

# create a pseudo target shaders just to have shader files in solution explorer
file(GLOB_RECURSE SHADER_SOURCES "res/shaders/*.slang" "res/shaders/*.hlsl")
foreach(_shader IN ITEMS ${SHADER_SOURCES})
    get_filename_component(_shader_path "${_shader}" PATH)
    file(RELATIVE_PATH _shader_path_rel "${CMAKE_SOURCE_DIR}" "${_shader_path}")
    string(REPLACE "/" "\\" _group_path "${_shader_path_rel}")
    source_group("${_group_path}" FILES "${_shader}")
endforeach()
add_custom_target(shaders SOURCES ${SHADER_SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
set_target_properties(rabbithole PROPERTIES LINKER_LANGUAGE CXX)

# Linking libraries
target_link_libraries(${PROJECT_NAME} 
    Vulkan::Vulkan
)

target_link_libraries(${PROJECT_NAME} debug ${CMAKE_SOURCE_DIR}/extern/slang/bin/windows-x64/release/slang.lib)
target_link_libraries(${PROJECT_NAME} debug ${CMAKE_SOURCE_DIR}/extern/optick/lib/debug/OptickCoreD.lib)
target_link_libraries(${PROJECT_NAME} debug ${CMAKE_SOURCE_DIR}/extern/assimp/lib/Debug/assimp-vc142-mtd.lib)
target_link_libraries(${PROJECT_NAME} debug ${CMAKE_SOURCE_DIR}/extern/fsr/lib/ffx_fsr2_api_x64d.lib)
target_link_libraries(${PROJECT_NAME} debug ${CMAKE_SOURCE_DIR}/extern/fsr/lib/ffx_fsr2_api_vk_x64d.lib)
target_link_libraries(${PROJECT_NAME} debug ${CMAKE_SOURCE_DIR}/extern/glfw/lib/glfw3.lib)

target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_SOURCE_DIR}/extern/slang/bin/windows-x64/release/slang.lib)
target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_SOURCE_DIR}/extern/optick/lib/release/OptickCore.lib)
target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_SOURCE_DIR}/extern/assimp/lib/Release/assimp-vc142-mt.lib)
target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_SOURCE_DIR}/extern/fsr/lib/ffx_fsr2_api_x64.lib)
target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_SOURCE_DIR}/extern/fsr/lib/ffx_fsr2_api_vk_x64.lib)
target_link_libraries(${PROJECT_NAME} optimized ${CMAKE_SOURCE_DIR}/extern/glfw/lib/glfw3.lib)

target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:DEBUG>:/Od>
    $<$<CONFIG:RELEASE>:/O2>
    $<$<CONFIG:RELEASE>:/Oi>
    $<$<CONFIG:RELEASE>:/Ot>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:DEBUG>:DEBUG>
    $<$<CONFIG:RELEASE>:NDEBUG>
)

# copy dlls and stuff from extern
file(GLOB DLL_FILES "${CMAKE_SOURCE_DIR}/extern/dlls/*")

foreach(DLL_FILE ${DLL_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_FILE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endforeach()

# copy imgui ini to build and build/target/ folders
file(GLOB IMGUI_INI "${CMAKE_SOURCE_DIR}/imgui.ini")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${IMGUI_INI}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${IMGUI_INI}"
        "${CMAKE_BINARY_DIR}"
)