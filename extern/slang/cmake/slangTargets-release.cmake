#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "slang::slang-llvm" for configuration "Release"
set_property(TARGET slang::slang-llvm APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(slang::slang-llvm PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELEASE ""
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libslang-llvm.so"
  IMPORTED_NO_SONAME_RELEASE "TRUE"
  )

list(APPEND _cmake_import_check_targets slang::slang-llvm )
list(APPEND _cmake_import_check_files_for_slang::slang-llvm "${_IMPORT_PREFIX}/lib/libslang-llvm.so" )

# Import target "slang::slang-glslang" for configuration "Release"
set_property(TARGET slang::slang-glslang APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(slang::slang-glslang PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELEASE ""
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libslang-glslang.so"
  IMPORTED_NO_SONAME_RELEASE "TRUE"
  )

list(APPEND _cmake_import_check_targets slang::slang-glslang )
list(APPEND _cmake_import_check_files_for_slang::slang-glslang "${_IMPORT_PREFIX}/lib/libslang-glslang.so" )

# Import target "slang::slangd" for configuration "Release"
set_property(TARGET slang::slangd APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(slang::slangd PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/slangd"
  )

list(APPEND _cmake_import_check_targets slang::slangd )
list(APPEND _cmake_import_check_files_for_slang::slangd "${_IMPORT_PREFIX}/bin/slangd" )

# Import target "slang::gfx" for configuration "Release"
set_property(TARGET slang::gfx APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(slang::gfx PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "slang::slang"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libgfx.so"
  IMPORTED_SONAME_RELEASE "libgfx.so"
  )

list(APPEND _cmake_import_check_targets slang::gfx )
list(APPEND _cmake_import_check_files_for_slang::gfx "${_IMPORT_PREFIX}/lib/libgfx.so" )

# Import target "slang::slang" for configuration "Release"
set_property(TARGET slang::slang APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(slang::slang PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libslang.so"
  IMPORTED_SONAME_RELEASE "libslang.so"
  )

list(APPEND _cmake_import_check_targets slang::slang )
list(APPEND _cmake_import_check_files_for_slang::slang "${_IMPORT_PREFIX}/lib/libslang.so" )

# Import target "slang::slangc" for configuration "Release"
set_property(TARGET slang::slangc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(slang::slangc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/slangc"
  )

list(APPEND _cmake_import_check_targets slang::slangc )
list(APPEND _cmake_import_check_files_for_slang::slangc "${_IMPORT_PREFIX}/bin/slangc" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
