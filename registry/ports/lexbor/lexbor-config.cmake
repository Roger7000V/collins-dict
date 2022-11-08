set(LEXBOR_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../..)
find_library(LEXBOR_LIBRARY_RELEASE NAMES lexbor lexbor_static
    PATHS ${LEXBOR_INSTALL_PREFIX}/lib NO_DEFAULT_PATH)
find_library(LEXBOR_LIBRARY_DEBUG NAMES lexbor lexbor_static
    PATHS ${LEXBOR_INSTALL_PREFIX}/debug/lib NO_DEFAULT_PATH)
add_library(lexbor UNKNOWN IMPORTED)
select_library_configurations(LEXBOR)

set(LEXBOR_INCLUDE_DIR ${LEXBOR_INSTALL_PREFIX}/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lexbor REQUIRED_VARS LEXBOR_LIBRARY LEXBOR_INCLUDE_DIR)

if(LEXBOR_LIBRARY_RELEASE)
    set_target_properties(lexbor PROPERTIES
        IMPORTED_CONFIGURATIONS RELEASE
        IMPORTED_LOCATION_RELEASE ${LEXBOR_LIBRARY_RELEASE}
        IMPORTED_IMPLIB_RELEASE ${LEXBOR_LIBRARY_RELEASE}
        INTERFACE_INCLUDE_DIRECTORIES ${LEXBOR_INCLUDE_DIR})
else()
    set_target_properties(lexbor PROPERTIES
        IMPORTED_CONFIGURATIONS DEBUG
        IMPORTED_LOCATION_DEBUG ${LEXBOR_LIBRARY_DEBUG}
        IMPORTED_IMPLIB_DEBUG ${LEXBOR_LIBRARY_DEBUG}
        INTERFACE_INCLUDE_DIRECTORIES ${LEXBOR_INCLUDE_DIR})
endif()
