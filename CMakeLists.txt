cmake_minimum_required(VERSION 3.8)
project(rabbithole)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(Vulkan REQUIRED)
# Build submodules
# GLFW
option(GLFW_BUILD_EXAMPLES "Build GLFW examples" OFF)
option(GLFW_BUILD_TESTS "Build GLFW tests" OFF)
option(GLFW_BUILD_DOCS "Build GLFW docs" OFF)
option(GLFW_INSTALL "Generate GLFW installation target" OFF)
add_subdirectory(extern/glfw)

# --- Slang Automation ---
set(SLANG_VERSION "2026.7.1")
if(WIN32)
    set(SLANG_PLATFORM "windows-x86_64")
    set(SLANG_SUFFIX "zip")
else()
    set(SLANG_PLATFORM "linux-x86_64")
    set(SLANG_SUFFIX "tar.gz")
endif()

set(SLANG_DIR "${CMAKE_SOURCE_DIR}/extern/slang")
set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-${SLANG_PLATFORM}.${SLANG_SUFFIX}")

if(NOT EXISTS "${SLANG_DIR}/include/slang.h")
    message(STATUS "Slang binaries not found. Downloading from ${SLANG_URL}...")
    set(SLANG_TEMP_FILE "${CMAKE_BINARY_DIR}/slang_temp.${SLANG_SUFFIX}")
    file(DOWNLOAD "${SLANG_URL}" "${SLANG_TEMP_FILE}" SHOW_PROGRESS STATUS download_status)
    
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download Slang binaries: ${download_status}")
    endif()

    message(STATUS "Extracting Slang binaries to ${SLANG_DIR}...")
    file(ARCHIVE_EXTRACT INPUT "${SLANG_TEMP_FILE}" DESTINATION "${SLANG_DIR}")
    file(REMOVE "${SLANG_TEMP_FILE}")
endif()

# --- Assimp Automation (FetchContent) ---
include(FetchContent)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE) # Ensure zlib is built if needed

FetchContent_Declare(
    assimp
    URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.3.zip
)

message(STATUS "Setting up Assimp via FetchContent...")
FetchContent_MakeAvailable(assimp)
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/vendor
    ${CMAKE_SOURCE_DIR}/extern
    ${CMAKE_SOURCE_DIR}/extern/glfw/include
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/extern/vma
    ${SLANG_DIR}/include/
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

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glfw
    assimp
)



# Slang
set(SLANG_LIB_DIR "${SLANG_DIR}/lib")
set(SLANG_BIN_DIR "${SLANG_DIR}/bin")
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE "${SLANG_LIB_DIR}/slang.lib")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE "${SLANG_LIB_DIR}/libslang.so")
endif()


# Handle FSR libraries
set(FSR_LIB_DIR "${CMAKE_SOURCE_DIR}/extern/fsr/lib")
target_link_libraries(${PROJECT_NAME} PRIVATE
    debug "${FSR_LIB_DIR}/ffx_fsr2_api_x64d.lib"
    debug "${FSR_LIB_DIR}/ffx_fsr2_api_vk_x64d.lib"
    optimized "${FSR_LIB_DIR}/ffx_fsr2_api_x64.lib"
    optimized "${FSR_LIB_DIR}/ffx_fsr2_api_vk_x64.lib"
)

# Compile options...
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

# Copy imgui.ini
if(EXISTS "${CMAKE_SOURCE_DIR}/imgui.ini")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/imgui.ini" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()

# Copy Slang DLLs
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SLANG_BIN_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)
