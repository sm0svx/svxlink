add_executable(devcal devcal.cpp)
target_link_libraries(devcal asyncaudio asynccpp trx popt)
set_target_properties(devcal PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

add_executable(noisegen noisegen.cpp)
target_link_libraries(noisegen asyncaudio asynccpp trx)
set_target_properties(noisegen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)
