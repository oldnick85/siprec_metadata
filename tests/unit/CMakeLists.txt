add_executable(unit
    main.cpp
)

find_package(GTest REQUIRED)

target_link_libraries(unit PRIVATE
    ${PROJECT_NAME}
    GTest::gtest_main)
