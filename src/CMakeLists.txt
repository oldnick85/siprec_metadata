add_library(${PROJECT_NAME}
    siprec_metadata.cpp
)

target_include_directories(${PROJECT_NAME}
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "siprec_metadata.h"
)