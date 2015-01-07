# Find the popt library
find_package(Popt REQUIRED)
include_directories(${POPT_INCLUDE_DIRS})
add_definitions(${POPT_DEFINITIONS})

add_executable(devcal devcal.cpp)
target_link_libraries(devcal asyncaudio asynccpp trx ${POPT_LIBRARIES})
set_target_properties(devcal PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(noisegen noisegen.cpp)
target_link_libraries(noisegen asyncaudio asynccpp trx)
set_target_properties(noisegen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)
