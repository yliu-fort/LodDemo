set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/")
configure_file("${CONFIGURE_SRC_PATH_TEMPLATE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_source_dir.h" @ONLY)

file(GLOB SOURCE
    "*.h"
    "renderer/*"
    "renderer/debug/*"
    )

# then create a project file per tutorial
###
add_executable(planet "main.cpp" ${SOURCE})
target_link_libraries(planet ${LIBS})
