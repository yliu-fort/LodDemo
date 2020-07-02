set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/")
configure_file("${CONFIGURE_SRC_PATH_TEMPLATE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_source_dir.h" @ONLY)

file(GLOB SOURCE
    #${UTILITY}
    "*.h"
    "*.cpp"
    "renderer/*"
    "atmosphereRender/*"
    )

# then create a project file per tutorial
###
add_executable(architecture ${SOURCE})
target_link_libraries(architecture ${LIBS})
