add_subdirectory(cnfkit)
add_subdirectory(gatekit)
add_subdirectory(minisat EXCLUDE_FROM_ALL)

add_library(minisat-lib INTERFACE)
target_link_libraries(minisat-lib INTERFACE minisat-lib-static)
target_include_directories(minisat-lib INTERFACE minisat)

