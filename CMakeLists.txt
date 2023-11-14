cmake_minimum_required(VERSION 3.7)
project(ucoap 
    VERSION 1.0.0
    LANGUAGES C
)

configure_file(manifest.h.in manifest.h)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")


include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)


# Static Library
add_library(ucoap STATIC 
    ucoap.c
    ucoap_tcp.c
    ucoap_udp.c
)


# Install
install(TARGETS ucoap DESTINATION "lib")
install(FILES ucoap.h DESTINATION "include")


# CPack
set(CPACK_PACKAGE_FILE_NAME "libucoap-${PROJECT_VERSION}")
set(CPACK_SET_DESTDIR true)
set(CPACK_PACKAGE_NAME libucoap)
set(CPACK_PACKAGE_CONTACT "Vahid Mardani <vahid.mardani@gmail.com>")
set(CPACK_GENERATOR DEB)
include(CPack)

# Examples
add_subdirectory(examples)
