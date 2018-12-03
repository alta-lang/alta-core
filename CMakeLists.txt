cmake_minimum_required(VERSION 3.10)

project(altacore)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests")
set(OPTIONAL_ENABLE_DOCS OFF CACHE BOOL "Disable Optional docs")
set(OPTIONAL_ENABLE_TESTS OFF CACHE BOOL "Disable Optional tests")

add_subdirectory("${PROJECT_SOURCE_DIR}/deps/yaml-cpp")
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/optional")

include("${PROJECT_SOURCE_DIR}/deps/CMakeLists-semver.c.cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

add_library(altacore
  "${PROJECT_SOURCE_DIR}/src/lexer.cpp"
  "${PROJECT_SOURCE_DIR}/src/parser.cpp"
  "${PROJECT_SOURCE_DIR}/src/fs.cpp"
  "${PROJECT_SOURCE_DIR}/src/modules.cpp"
  "${PROJECT_SOURCE_DIR}/src/util.cpp"
  "${PROJECT_SOURCE_DIR}/src/preprocessor.cpp"
  "${PROJECT_SOURCE_DIR}/src/attributes.cpp"

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
  "${PROJECT_SOURCE_DIR}/src/ast/boolean-literal-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/binary-operation.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/import-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/function-call-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/string-literal-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/function-declaration-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/attribute-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/literal-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/attribute-statement.cpp"

  # DET nodes
  "${PROJECT_SOURCE_DIR}/src/det/node.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/module.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/scope.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/scope-item.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/function.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/type.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/variable.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/alias.cpp"
  "${PROJECT_SOURCE_DIR}/src/det/namespace.cpp"
)
target_include_directories(altacore PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_compile_definitions(altacore PUBLIC ALTACORE_LOCAL_SEMVER)

target_link_libraries(altacore PRIVATE yaml-cpp)
target_link_libraries(altacore PUBLIC semver_c optional)
