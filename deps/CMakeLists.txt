# TODO: Unfortunately, adding cnfkit and gatekit via add_subdirectory is broken at the moment :(
add_library(cnfkit INTERFACE)
target_include_directories(cnfkit INTERFACE cnfkit/include)

add_library(gatekit INTERFACE)
target_include_directories(gatekit INTERFACE gatekit/include)

