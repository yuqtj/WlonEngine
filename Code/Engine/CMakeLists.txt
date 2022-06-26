cmake_minimum_required(VERSION 3.12)

# create sample app project
project(Engine LANGUAGES C CXX)

# Engine_Files代表包含的文件
file(GLOB_RECURSE Engine_Files
    *.h
    *.cpp
	*.inl
)

# Engine_Include_Path代表存储所有的头文件目录路径
set(Engine_Include_Path 
	/
	${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/vulkan/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glslang
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tinygltf
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glm
	)

set(Engine_Link_Libraries
    glfw
    volk
    glslang
    SPIRV
    tinygltf
)

add_executable(${PROJECT_NAME} WIN32 ${Engine_Files})

target_include_directories(Engine PRIVATE ${Engine_Include_Path})

# 链接库
target_link_libraries(Engine ${Engine_Link_Libraries})

# Create MSVC project
if(MSVC)
    #Set the working directory to the source of the project so developer dont have to
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    #Configure output paths
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG_TYPE} SUFFIX)
        string(TOLOWER ${CONFIG_TYPE} CONFIG_DIR)
        set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/${TARGET_ARCH})
        set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
        set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
    endforeach()
endif()

IF(${WIN32})
    # Windows specific code
	Add_Definitions(-DUSE_WINDOWS=1)
ELSEIF(${UNIX})
    IF(${APPLE})
        # Apple specific code
        add_subdirectory(Platform/Darwin)
    ELSEIF(${ANDROID})
        add_subdirectory(Platform/Android)
    ELSE(${APPLE})
        # Linux specific code
        add_subdirectory(Platform/Linux)
    ENDIF(${APPLE})
ELSEIF(${ORBIS})
    add_subdirectory(Platform/Orbis)
ELSE()
    # Empty
    add_subdirectory(Empty)
ENDIF(${WIN32})