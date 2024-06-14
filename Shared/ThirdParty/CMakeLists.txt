set(ThirdParty_Libs)
set(ThirdParty_IncludeDirs)

set(MessageQuiet OFF)
function(message)
    if (NOT MessageQuiet)
        _message(${ARGN})
    endif ()
endfunction()

macro(quiet Status)
    set(MessageQuiet ${Status})
endmacro()

macro(quiet_add_subdirectory Subdir)
    quiet(ON)
    add_subdirectory(${Subdir})
    quiet(OFF)
endmacro()

include(../../CmakeUtils/SourceGrouping.cmake)

##############
#   assimp   #
##############
if (USE_ASSIMP)
    set(Assimp_Root ${CMAKE_CURRENT_SOURCE_DIR}/assimp)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_FBX_IMPORTER ON)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON)
    set(BUILD_SHARED_LIBS OFF)
    set(ASSIMP_NO_EXPORT ON)
    set(ASSIMP_BUILD_TESTS OFF)
    set(ASSIMP_INSTALL OFF)

    block(SCOPE_FOR POLICIES)
        cmake_policy(SET CMP0077 NEW)

        quiet_add_subdirectory(${Assimp_Root})
    endblock()

    group_targets(TARGETS_PATH ${Assimp_Root})

    list(APPEND ThirdParty_Libs assimp)
endif ()

############
#   glfw   #
############
if (USE_GLFW)
    set(Glfw_Root ${CMAKE_CURRENT_SOURCE_DIR}/glfw)
    set(GLFW_BUILD_EXAMPLES OFF)
    set(GLFW_BUILD_TESTS OFF)
    set(GLFW_BUILD_DOCS OFF)
    set(GLFW_INSTALL OFF)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL ON)

    quiet_add_subdirectory(${Glfw_Root})

    group_targets(TARGETS_PATH ${Glfw_Root})

    list(APPEND ThirdParty_Libs glfw)
endif ()

###############
#   glslang   #
###############
if (USE_GLSLANG)
    set(Glslang_Root ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
    set(ENABLE_CTEST OFF)
    set(ENABLE_GLSLANG_JS OFF)

    quiet_add_subdirectory(${Glslang_Root})

    group_targets(TARGETS_PATH ${Glslang_Root})

    list(APPEND ThirdParty_Libs glslang)
endif ()

############
#   volk   #
############
if (USE_VOLK)
    set(Volk_Root ${CMAKE_CURRENT_SOURCE_DIR}/volk)
    if (WIN32)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
    elseif (APPLE)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
    endif ()

    quiet_add_subdirectory(${Volk_Root})

    group_targets(TARGETS_PATH ${Volk_Root})

    list(APPEND ThirdParty_Libs volk)
endif ()

#################
#  Dear ImGui   #
#################
if (USE_IMGUI)
    if (NOT USE_GLFW)
        message(FATAL_ERROR "ImGui required glfw")
    endif ()

    set(ImGui_Root ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
    file(GLOB ImGui_Sources ${ImGui_Root}/*.cpp)
    if (APPLE)
        list(APPEND ImGui_Sources
                ${ImGui_Root}/backends/imgui_impl_osx.mm
                ${ImGui_Root}/backends/imgui_impl_metal.mm)
    elseif (WIN32)
        list(APPEND ImGui_Sources
                ${ImGui_Root}/backends/imgui_impl_glfw.cpp
                ${ImGui_Root}/backends/imgui_impl_opengl3.cpp)
    endif ()

    add_library(imgui)
    target_sources(imgui
            PRIVATE ${ImGui_Sources})
    target_include_directories(imgui
            PUBLIC
            ${ImGui_Root}
            ${ImGui_Root}/backends)
    target_link_libraries(imgui
            PUBLIC glfw)

    group_targets(TARGETS_PATH ${ImGui_Root} CUSTOM_TARGET)

    list(APPEND ThirdParty_Libs imgui)
endif ()

################
#   ImGuizmo   #
################
if (USE_IMGUIZMO)
    if (NOT USE_IMGUI)
        message(FATAL_ERROR "ImGuizmo required imgui")
    endif ()

    set(ImGuizmo_Root ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)
    file(GLOB Imguizmo_Sources ${ImGuizmo_Root}/*.cpp)

    add_library(ImGuizmo)

    target_sources(ImGuizmo
            PRIVATE ${Imguizmo_Sources})

    target_include_directories(ImGuizmo
            PUBLIC ${ImGuizmo_Root})

    target_link_libraries(ImGuizmo
            PUBLIC imgui)

    group_targets(TARGETS_PATH ${ImGuizmo_Root} CUSTOM_TARGET)

    list(APPEND ThirdParty_Libs imguizmo)
endif ()

###########
#   stb   #
###########
if (USE_STB)
    set(Stb_Root ${CMAKE_CURRENT_SOURCE_DIR}/stb)

    add_library(stb INTERFACE)

    target_include_directories(stb
            INTERFACE ${Stb_Root})

    group_targets(TARGETS_PATH ${Stb_Root} CUSTOM_TARGET stb)

    list(APPEND ThirdParty_Libs stb)
endif ()

############
#   mINI   #
############
if (${USE_MINI})
    set(Mini_Root ${CMAKE_CURRENT_SOURCE_DIR}/mINI)

    add_library(mINI INTERFACE)

    target_include_directories(mINI
            INTERFACE ${Mini_Root}/src)

    group_targets(TARGETS_PATH ${Mini_Root} CUSTOM_TARGET)

    list(APPEND ThirdParty_Libs mINI)
endif ()

############
#   glad   #
############
if (USE_GLAD)
    set(Glad_Root ${CMAKE_CURRENT_SOURCE_DIR}/glad)

    add_library(glad)

    target_sources(glad
            PRIVATE
            ${Glad_Root}/src/gl.c
            ${Glad_Root}/src/vulkan.c)

    target_include_directories(glad
            PUBLIC ${Glad_Root}/include)

    group_targets(TARGETS_PATH ${Glad_Root} CUSTOM_TARGET)

    list(APPEND ThirdParty_Libs glad)
endif ()

###########
#   glm   #
###########
if (USE_GLM)
    set(Glm_Root ${CMAKE_CURRENT_SOURCE_DIR}/glm)

    quiet_add_subdirectory(${Glm_Root})

    group_targets(TARGETS_PATH ${Glm_Root})

    list(APPEND ThirdParty_Libs glm)
endif ()

###########
#   gli   #
###########
if (USE_GLI)
    set(Gli_Root ${CMAKE_CURRENT_SOURCE_DIR}/gli)
    set(GLI_TEST_ENABLE OFF)

    quiet_add_subdirectory(${Gli_Root})

    group_targets(TARGETS_PATH ${Gli_Root})

    list(APPEND ThirdParty_Libs gli)
endif ()

#############
#   tracy   #
#############
if (USE_TRACY)
    set(Tracy_Root ${CMAKE_CURRENT_SOURCE_DIR}/tracy)
    set(TRACY_ON_DEMAND ON)

    quiet_add_subdirectory(${Tracy_Root})

    group_targets(TARGETS_PATH ${Tracy_Root})

    list(APPEND ThirdParty_Libs Tracy::TracyClient)
endif ()

################
#   TaskFlow   #
################
if (USE_TASK_FLOW)
    set(TaskFlow_Root ${CMAKE_CURRENT_SOURCE_DIR}/taskflow)
    set(TF_BUILD_EXAMPLES OFF)
    set(TF_BUILD_TESTS OFF)

    quiet_add_subdirectory(${TaskFlow_Root})

    group_targets(TARGETS_PATH ${TaskFlow_Root})

    list(APPEND ThirdParty_Libs Taskflow)
endif ()

##############
#   Optick   #
##############
if (USE_OPTICK)
    if (USE_EASY_PROFILER)
        message(FATAL_ERROR "Cannot enable both profilers (Optick and EasyProfiler) at once. Just pick one please.")
    endif ()

    set(Optick_Root ${CMAKE_CURRENT_SOURCE_DIR}/optick})

    quiet_add_subdirectory(${Optick_Root})

    target_compile_definitions(Optick
            PUBLIC BUILD_WITH_OPTICK)

    group_targets(TARGETS_PATH ${Optick_Root})

    list(APPEND ThirdParty_IncludeDirs ${Optick_Root}/src)
    list(APPEND ThirdParty_Libs Optick)
endif ()

####################
#   EasyProfiler   #
####################
if (USE_EASY_PROFILER)
    if (USE_OPTICK)
        message(FATAL_ERROR "Cannot enable both profilers (Optick and EasyProfiler) at once. Just pick one please.")
    endif ()
    set(EasyProfiler_Root ${CMAKE_CURRENT_SOURCE_DIR}/easy_profiler)
    set(EASY_PROFILER_NO_GUI ON)

    quiet_add_subdirectory(${EasyProfiler_Root})

    group_targets(TARGETS_PATH ${EasyProfiler_Root})

    target_compile_definitions(easy_profiler
            PUBLIC BUILD_WITH_EASY_PROFILER)

    list(APPEND ThirdParty_IncludeDirs ${EasyProfiler_Root}/easy_profiler_core/include)
    list(APPEND ThirdParty_Libs easy_profiler)
endif ()

###############
#   bullet3   #
###############
if (USE_BULLET3)
    set(Bullet3_Root ${CMAKE_CURRENT_SOURCE_DIR}/bullet3)

    set(BUILD_EXTRAS OFF)
    set(BUILD_UNIT_TESTS OFF)
    set(BUILD_ENET OFF)
    set(BUILD_CLSOCKET OFF)
    set(BUILD_PYBULLET OFF)
    set(USE_GRAPHICAL_BENCHMARK OFF)
    set(ENABLE_VHACD OFF)
    set(BUILD_CPU_DEMOS OFF)
    set(INSTALL_LIBS OFF)
    set(INSTALL_CMAKE_FILES OFF)
    set(BUILD_BULLET2_DEMOS OFF)

    block(SCOPE_FOR POLICIES)
        cmake_policy(SET CMP0077 NEW)

        quiet_add_subdirectory(${Bullet3_Root})
    endblock()

    group_targets(TARGETS_PATH ${Bullet3_Root})

    add_library(bullet3::LinearMath ALIAS LinearMath)
    add_library(bullet3::Common ALIAS Bullet3Common)
    add_library(bullet3::InverseDynamics ALIAS BulletInverseDynamics)
    add_library(bullet3::Collision ALIAS BulletCollision)
    add_library(bullet3::Dynamics ALIAS BulletDynamics)
    add_library(bullet3::SoftBody ALIAS BulletSoftBody)

    list(APPEND ThirdParty_IncludeDirs
            ${Bullet3_Root}/src)
    list(APPEND ThirdParty_Libs
            bullet3::LinearMath
            bullet3::Common
            bullet3::InverseDynamics
            bullet3::Collision
            bullet3::Dynamics
            bullet3::SoftBody)
endif ()

set(ThirdParty_Libs ${ThirdParty_Libs} PARENT_SCOPE)
set(ThirdParty_IncludeDirs ${ThirdParty_IncludeDirs} PARENT_SCOPE)
