set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_C_EXTENSIONS off)

add_library(semver_c "${CMAKE_CURRENT_LIST_DIR}/semver.c/semver.c")
