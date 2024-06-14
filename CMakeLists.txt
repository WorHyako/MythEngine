cmake_minimum_required(VERSION 3.24)

project(MythNgine)

set(Exe_Root_Name MythNgine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#############################################
#   Option to control third party modules   #
#############################################
option(USE_ASSIMP "Assimp" ON)
option(USE_EASY_PROFILER "Easy profiler" ON)
option(USE_GLAD "Glad" ON)
option(USE_GLFW "Glfw" ON)
option(USE_GLM "Glm" ON)
option(USE_GLI "Gli" ON)
option(USE_GLSLANG "Glslang" ON)
option(USE_IMGUI "ImGui" ON)
option(USE_IMGUIZMO "ImGuizmo" ON)
option(USE_MINI "mINI" ON)
option(USE_OPTICK "Optick" OFF)
option(USE_STB "Stb" ON)
option(USE_TASK_FLOW "Task flow" ON)
option(USE_TRACY "Tracy" ON)
option(USE_VOLK "Volk" ON)
option(USE_BULLET3 "Bullet" ON)
option(LOG_ALL "Log all results" OFF)

if (USE_EASY_PROFILER AND USE_OPTICK)
    message(FATAL_ERROR "Cannot enable both profilers (Optick and EasyProfiler) at once. Just pick one please.")
endif ()

include(CmakeUtils/SourceGrouping.cmake)
include(CmakeUtils/LibrarySorter.cmake)

###################################
#   Platform-specific variables   #
###################################
if (APPLE)
    set(Os_Flags "")
elseif (WIN32)
    set(Os_Flags WIN32)
endif ()
set(Platform_Path ${PROJECT_SOURCE_DIR}/Platform/Win64)

###################
#   Third party   #
###################
add_subdirectory(${PROJECT_SOURCE_DIR}/Shared/ThirdParty)

###########
#   RHI   #
###########
add_subdirectory(${PROJECT_SOURCE_DIR}/Shared/RHI)

####################
#   Source files   #
####################
if (WIN32)
    list(APPEND Source_File_Ext
            ${Platform_Path}/*.cpp
            ${Platform_Path}/*.hpp
            ${Platform_Path}/*.metal
            ${Platform_Path}/*.hlsl
            ${Platform_Path}/*.vs
            ${Platform_Path}/*.vert
            ${Platform_Path}/*.frag
            ${Platform_Path}/*.geom
            ${Platform_Path}/*.tesc
            ${Platform_Path}/*.tese
            ${Platform_Path}/*.comp
            ${Platform_Path}/*.spv
            ${Platform_Path}/*.ini)
elseif (APPLE)
    list(APPEND Source_File_Ext
            Platform/Win64/RHI/Vulkan/*.cpp
            Platform/Win64/UserInput/*.cpp)
endif ()
file(GLOB_RECURSE Platform_Sources ${Source_File_Ext})
file(GLOB_RECURSE Shared_Sources ${PROJECT_SOURCE_DIR}/Shared/Engine/*.*pp)

list(APPEND Platform_Include_Dir
        ${Platform_Path})
list(APPEND Shared_Include_Dir
        ${PROJECT_SOURCE_DIR}/Shared
        ${PROJECT_SOURCE_DIR}/Shared/Engine)

#######################
#   Source grouping   #
#######################
create_source_group(${PROJECT_SOURCE_DIR}/Shared)
create_source_group(${Platform_Path})

find_package(Vulkan REQUIRED)
list(APPEND ThirdParty_Libs
        Vulkan::Vulkan
        SPIRV
        glslang-default-resource-limits)

sort_library(${ThirdParty_Libs})

################
#   Executer   #
################
add_executable(MythEngine ${Os_Flags})

set(VS_STARTUP_PROJECT MythEngine)

list(REMOVE_ITEM Platform_Sources
        /Users/worhyako/Coding/MythEngine/Platform/Win64/RHI/DX12/D3D12Window.hpp
        /Users/worhyako/Coding/MythEngine/Platform/Win64/RHI/DX12/D3D12Window.cpp)
target_sources(MythEngine
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Shared/main.cpp
        ${Shared_Sources}
        ${Platform_Sources}
        ${Editor_Sources}
        ${Rhi_Sources})

target_compile_definitions(MythEngine
        PRIVATE
        PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}"
        PLATFORM_DIR="${Platform_Path}")

target_include_directories(MythEngine
        PRIVATE
        ${Platform_Include_Dir}
        ${Shared_Include_Dir}
        ${Rhi_Include_Dir}
        ${ThirdParty_IncludeDirs})

target_link_libraries(MythEngine
        PRIVATE ${ThirdParty_Libs})

target_compile_features(MythEngine
        PRIVATE cxx_std_20)
