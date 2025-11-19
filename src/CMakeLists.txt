# Define the shared library
add_library(BlockTypeSupportsBasicPythonSupport SHARED RegisterBlockFactories.cpp
            RegisterBlockFactories.cpp
            LoggerInstance.cpp)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)


include(FetchContent)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.15.0
)

FetchContent_MakeAvailable(spdlog)

add_dependencies(BlockTypeSupportsBasicPythonSupport spdlog::spdlog)
target_link_libraries(BlockTypeSupportsBasicPythonSupport PRIVATE spdlog::spdlog)



target_link_libraries(BlockTypeSupportsBasicPythonSupport PRIVATE PySysLinkBase::PySysLinkBase)

# Include directories for header files
target_include_directories(BlockTypeSupportsBasicPythonSupport PUBLIC ${PySysLinkBase_INCLUDE_DIRS})

target_include_directories(
    BlockTypeSupportsBasicPythonSupport PRIVATE
    ${Python3_INCLUDE_DIRS}
)

target_link_libraries(
    BlockTypeSupportsBasicPythonSupport PRIVATE
    ${Python3_LIBRARIES}
)

set_target_properties(BlockTypeSupportsBasicPythonSupport PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    IMPORTED_LOCATION "${CMAKE_INSTALL_PREFIX}${INSTALLED_DYNAMIC_LIB_NAME}.so"
    OUTPUT_NAME "${INSTALLED_DYNAMIC_LIB_NAME}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(BlockTypeSupportsBasicPythonSupport
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>  # Include path for the build tree
        $<INSTALL_INTERFACE:include>                   # Include path for the installed tree
)
