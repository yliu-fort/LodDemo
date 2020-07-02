set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/")
configure_file("${CONFIGURE_SRC_PATH_TEMPLATE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_source_dir.h" @ONLY)

file(GLOB SOURCE
    #${UTILITY}
    "*.h"
    "*.cpp"
    "renderer/*"
    "renderer/atmosphere/*"
    )

# then create a project file per tutorial
# COUP architechture
# C - system, low-level logic
# O - object ( primitives, camera, complex object)
# U - utility ( texture loader, wrapper etc)
# P - parts ( transform, shader, material...)
###
add_executable(COUPdemo ${SOURCE})
target_link_libraries(COUPdemo ${LIBS})
