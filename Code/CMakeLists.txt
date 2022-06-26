cmake_minimum_required(VERSION 3.12)

project(Engine)

# create output folder
#file(MAKE_DIRECTORY output)

add_subdirectory(Engine)

# Add third party libraries
add_subdirectory(third_party)