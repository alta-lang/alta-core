cmake_minimum_required(VERSION 3.10)

project(altacore)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "disable yaml tests")

add_subdirectory("${PROJECT_SOURCE_DIR}/deps/yaml-cpp")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

add_library(altacore
  "${PROJECT_SOURCE_DIR}/src/lexer.cpp"
  "${PROJECT_SOURCE_DIR}/src/parser.cpp"
  "${PROJECT_SOURCE_DIR}/src/fs.cpp"
  "${PROJECT_SOURCE_DIR}/src/modules.cpp"

  # AST nodes
  "${PROJECT_SOURCE_DIR}/src/ast/node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/statement-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/expression-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/root-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/expression-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/type.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/parameter.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/block-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/function-definition-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/return-directive-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/integer-literal-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/variable-definition-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/accessor.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/fetch.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/assignment-expression.cpp"

  # DET nodes
  "${PROJECT_SOURCE_DIR}/src/det/node.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/module.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/scope.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/scope-item.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/function.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/type.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/variable.cpp"
)
target_include_directories(altacore PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(altacore PRIVATE yaml-cpp)
