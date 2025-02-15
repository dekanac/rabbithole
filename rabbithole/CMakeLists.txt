cmake_minimum_required(VERSION 3.8)
project(rabbithole)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(Vulkan REQUIRED)
find_package(GLFW3 REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/vendor
    ${CMAKE_SOURCE_DIR}/extern
    ${CMAKE_SOURCE_DIR}/extern/glfw/include
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/extern/vma
    ${CMAKE_SOURCE_DIR}/extern/assimp/include/
    ${CMAKE_SOURCE_DIR}/extern/slang/include/
    ${CMAKE_SOURCE_DIR}/extern/optick/include/
    ${CMAKE_SOURCE_DIR}/extern/fsr/include/
)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")

foreach(_file IN ITEMS ${SOURCES} ${HEADERS})
    get_filename_component(_file_path "${_file}" PATH)
    file(RELATIVE_PATH _file_path_rel "${CMAKE_SOURCE_DIR}" "${_file_path}")
    string(REPLACE "/" "\\" _group_path "${_file_path_rel}")
    source_group("${_group_path}" FILES "${_file}")
endforeach()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set(LIBRARIES
    Vulkan::Vulkan
    glfw
    debug ${CMAKE_SOURCE_DIR}/extern/slang/lib/libslang.so
    debug ${CMAKE_SOURCE_DIR}/extern/assimp/bin/libassimp.so
    debug ${CMAKE_SOURCE_DIR}/extern/fsr/lib/libffx_fsr2_api_x86_64.a
    debug ${CMAKE_SOURCE_DIR}/extern/fsr/lib/libffx_fsr2_api_vk_x86_64.a
    optimized ${CMAKE_SOURCE_DIR}/extern/slang/lib/libslang.so
    optimized ${CMAKE_SOURCE_DIR}/extern/assimp/bin/libassimp.so
    optimized ${CMAKE_SOURCE_DIR}/extern/fsr/lib/libffx_fsr2_api_x86_64.a
    optimized ${CMAKE_SOURCE_DIR}/extern/fsr/lib/libffx_fsr2_api_vk_x86_64.a
)

target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

set(COMPILER_OPTIONS_DEBUG -O0 -g)
set(COMPILER_OPTIONS_RELEASE -O3)
set(COMPILER_OPTIONS_RELWITHDEBINFO -O2 -g)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wno-braced-scalar-init>
    $<$<CONFIG:DEBUG>:${COMPILER_OPTIONS_DEBUG}>
    $<$<CONFIG:RELEASE>:${COMPILER_OPTIONS_RELEASE}>
    $<$<CONFIG:RELWITHDEBINFO>:${COMPILER_OPTIONS_RELWITHDEBINFO}>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:DEBUG>:DEBUG>
    $<$<CONFIG:RELEASE>:NDEBUG>
    $<$<CONFIG:RELWITHDEBINFO>:RELWITHDEBINFO>
)

# copy required libs to the build folder 
file(GLOB DLL_FILES "${CMAKE_SOURCE_DIR}/extern/dlls/*")
foreach(DLL_FILE ${DLL_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLL_FILE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endforeach()

file(GLOB IMGUI_INI "${CMAKE_SOURCE_DIR}/imgui.ini")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${IMGUI_INI}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)
