add_executable(garrett main.cpp)

target_link_libraries(garrett PRIVATE cnfkit gatekit)
target_include_directories(garrett PRIVATE "${LibArchive_INCLUDE_DIR}")
target_link_libraries(garrett PRIVATE "${LibArchive_LIBRARIES}")
