cmake_minimum_required(VERSION 3.10)

project(altacore)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests")
set(OPTIONAL_ENABLE_DOCS OFF CACHE BOOL "Disable Optional docs")
set(OPTIONAL_ENABLE_TESTS OFF CACHE BOOL "Disable Optional tests")

add_subdirectory("${PROJECT_SOURCE_DIR}/deps/yaml-cpp")
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/optional")
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/crossguid")
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/variant")

include("${PROJECT_SOURCE_DIR}/deps/CMakeLists-semver.c.cmake")

# currently uses Boost 1.70.0
add_subdirectory("${PROJECT_SOURCE_DIR}/deps/boost-cmake")

add_library(altacore
  "${PROJECT_SOURCE_DIR}/src/lexer.cpp"
  "${PROJECT_SOURCE_DIR}/src/parser.cpp"
  "${PROJECT_SOURCE_DIR}/src/fs.cpp"
  "${PROJECT_SOURCE_DIR}/src/modules.cpp"
  "${PROJECT_SOURCE_DIR}/src/util.cpp"
  "${PROJECT_SOURCE_DIR}/src/attributes.cpp"
  "${PROJECT_SOURCE_DIR}/src/validator.cpp"
  "${PROJECT_SOURCE_DIR}/src/altacore.cpp"
  "${PROJECT_SOURCE_DIR}/src/errors.cpp"
  "${PROJECT_SOURCE_DIR}/src/timing.cpp"
  "${PROJECT_SOURCE_DIR}/src/logging.cpp"

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
  "${PROJECT_SOURCE_DIR}/src/ast/conditional-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/conditional-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-definition-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-statement-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-member-definition-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-method-definition-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-special-method-definition-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-instantiation-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/pointer-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/dereference-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/while-loop-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/cast-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-read-accessor-definition-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/character-literal-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/type-alias-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/subscript-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/retrieval-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/super-class-fetch.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/instanceof-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/generic.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/for-loop-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/ranged-for-loop-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/unary-operation.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/sizeof-operation.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/floating-point-literal-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/structure-definition-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/export-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/variable-declaration-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/alias-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/delete-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/control-directive.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/try-catch-block.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/throw-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/nullptr-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/code-literal-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/bitfield-definition-node.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/lambda-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/special-fetch-expression.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/class-operator-definition-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/enumeration-definition-statement.cpp"
  "${PROJECT_SOURCE_DIR}/src/ast/yield-expression.cpp"

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
  "${PROJECT_SOURCE_DIR}/src/det/class.cpp"
)

# credit to Austin Lasher (https://medium.com/@alasher/colored-c-compiler-output-with-ninja-clang-gcc-10bfe7f2b949)
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
if(${FORCE_COLORED_OUTPUT})
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(altacore PRIVATE -fdiagnostics-color=always)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    target_compile_options(altacore PRIVATE -fcolor-diagnostics)
  endif()
endif()

target_include_directories(altacore PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_compile_definitions(altacore PUBLIC ALTACORE_LOCAL_SEMVER)
target_compile_definitions(altacore PUBLIC ALTACORE_LOCAL_ANY)

if (CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "10.14")
  target_compile_definitions(altacore PUBLIC ALTACORE_NO_SYSTEM_ANY)
  target_compile_definitions(altacore PUBLIC ALTACORE_NO_SYSTEM_VARIANT)
endif()

target_link_libraries(altacore PRIVATE yaml-cpp)
target_link_libraries(altacore PUBLIC semver_c optional crossguid mpark_variant Boost::filesystem)

set_target_properties(altacore
  PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED on
  CXX_EXTENSIONS off
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# install files
install(TARGETS altacore
  ARCHIVE DESTINATION lib
)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
  DESTINATION include
  FILES_MATCHING PATTERN "*"
)
