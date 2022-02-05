add_executable(garrett
  evaluation.cpp
  main.cpp
  progress_indicator.cpp
  validation.cpp
)

target_link_libraries(garrett PRIVATE cnfkit gatekit minisat-lib)
target_include_directories(garrett PRIVATE "${LibArchive_INCLUDE_DIR}")
target_link_libraries(garrett PRIVATE "${LibArchive_LIBRARIES}")
