project(MINIPALTESTS)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(minipaltests
  env_tests.c
)

add_dependencies(minipaltests minipal)

target_link_libraries(minipaltests
  PUBLIC
  minipal
)
