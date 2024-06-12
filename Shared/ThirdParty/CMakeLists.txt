set(ThirdParty_Libs)
set(ThirdParty_Include_Dirs)

##############
### Assimp ###
##############
if (${USE_ASSIMP})
    set(Assimp_Root ${CMAKE_CURRENT_SOURCE_DIR}/assimp)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_FBX_IMPORTER ON)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON)
    set(BUILD_SHARED_LIBS OFF)
    set(ASSIMP_NO_EXPORT ON)
    set(ASSIMP_BUILD_TESTS OFF)
    set(ASSIMP_INSTALL OFF)
    add_subdirectory(${Assimp_Root})
    list(APPEND ThirdParty_Libs assimp)
endif ()

############
### GLFW ###
############
if (${USE_GLFW})
    set(Glfw_Root ${CMAKE_CURRENT_SOURCE_DIR}/glfw)
    set(GLFW_BUILD_EXAMPLES OFF)
    set(GLFW_BUILD_TESTS OFF)
    set(GLFW_BUILD_DOCS OFF)
    set(GLFW_INSTALL OFF)
    set(GLFW_DOCUMENT_INTERNALS OFF)
    if (WIN32)
        set(USE_MSVC_RUNTIME_LIBRARY_DLL ON)
    endif ()
    add_subdirectory(${Glfw_Root})

    list(APPEND ThirdParty_Libs glfw)
endif ()

###############
### Glslang ###
###############
if (${USE_GLSLANG})
    set(Glslang_Root ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
    add_subdirectory(${Glslang_Root})

    list(APPEND ThirdParty_Libs glslang)
endif ()

############
### Volk ###
############
if (${USE_VOLK})
    set(Volk_Root ${CMAKE_CURRENT_SOURCE_DIR}/volk)
    #    if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
    list(APPEND ThirdParty_Libs volk)
    #    endif ()
    add_subdirectory(${Volk_Root})
endif ()

#############
### ImGui ###
#############
if (${USE_IMGUI})
    if (NOT USE_GLFW)
        message(FATAL_ERROR "ImGui required glfw")
    endif ()

    set(ImGui_Root ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
    file(GLOB Imgui_Sources ${ImGui_Root}/*.*pp)
    if (APPLE)
        list(APPEND Imgui_Sources
                ${ImGui_Root}/backends/imgui_impl_osx.mm
                ${ImGui_Root}/backends/imgui_impl_metal.mm)
    elseif (WIN32)
        list(APPEND Imgui_Sources
                ${ImGui_Root}/backends/imgui_impl_glfw.cpp
                ${ImGui_Root}/backends/imgui_impl_opengl3.cpp)
    endif ()

    add_library(imgui STATIC)
    target_sources(imgui
            PRIVATE ${Imgui_Sources})
    target_include_directories(imgui
            PUBLIC ${ImGui_Root}
            PUBLIC ${ImGui_Root}/include
            PUBLIC ${ImGui_Root}/backends)
    target_link_libraries(imgui
            PUBLIC glfw)

    list(APPEND ThirdParty_Libs imgui)
endif ()

################
### ImGuizmo ###
################
if (${USE_IMGUIZMO})
    if (NOT USE_IMGUI)
        message(FATAL_ERROR "ImGuizmo required imgui")
    endif ()

    set(ImGuizmo_Root ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)
    file(GLOB Imguizmo_Sources ${ImGuizmo_Root}/*.*pp)

    add_library(imguizmo STATIC)
    target_sources(imguizmo
            PRIVATE ${Imguizmo_Sources})
    target_include_directories(imgui
            PUBLIC ${ImGuizmo_Root})
    target_link_libraries(imguizmo
            PUBLIC imgui)

    list(APPEND ThirdParty_Libs imguizmo)
endif ()

###########
### Stb ###
###########
if (${USE_STB})
    set(Stb_Root ${CMAKE_CURRENT_SOURCE_DIR}/stb)
    add_library(stb
            INTERFACE ${Stb_Root}/stb_image.h)
    target_include_directories(stb
            INTERFACE ${Stb_Root})

    list(APPEND ThirdParty_Libs stb)
endif ()

############
### mINI ###
############
if (${USE_MINI})
    set(Mini_Root ${CMAKE_CURRENT_SOURCE_DIR}/mINI/src)
    add_library(mINI
            INTERFACE ${Mini_Root}/mini/ini.h)
    target_include_directories(mINI
            INTERFACE ${Mini_Root})

    list(APPEND ThirdParty_Libs mINI)
endif ()

############
### Glad ###
############
if (${USE_GLAD})
    set(Glad_Root ${CMAKE_CURRENT_SOURCE_DIR}/glad)
    add_library(glad STATIC)
    target_sources(glad
            PRIVATE ${Glad_Root}/src/gl.c
            PRIVATE ${Glad_Root}/src/vulkan.c)
    target_include_directories(glad
            PUBLIC ${Glad_Root}/include)

    list(APPEND ThirdParty_Libs glad)
endif ()

###########
### Glm ###
###########
if (${USE_GLM})
    set(Glm_Root ${CMAKE_CURRENT_SOURCE_DIR}/glm)
    add_subdirectory(${Glm_Root})

    list(APPEND ThirdParty_Libs glm)
endif ()

###########
### Gli ###
###########
if (${USE_GLI})
    set(Gli_Root ${CMAKE_CURRENT_SOURCE_DIR}/gli)
    set(GLI_TEST_ENABLE OFF)
    add_subdirectory(${Gli_Root})

    list(APPEND ThirdParty_Libs gli)
endif ()

#############
### Tracy ###
#############
if (${USE_TRACY})
    set(Tracy_Root ${CMAKE_CURRENT_SOURCE_DIR}/tracy)
    # set options before add_subdirectory
    # available options: TRACY_ENABLE , TRACY_ON_DEMAND , TRACY_NO_BROADCAST , TRACY_NO_CODE_TRANSFER , ...
    set(TRACY_ON_DEMAND ON)
    add_subdirectory(${Tracy_Root}) # target: TracyClient or alias Tracy :: TracyClient

    list(APPEND ThirdParty_Libs Tracy::TracyClient)
endif ()

################
### TaskFlow ###
################
if (${USE_TASK_FLOW})
    set(TaskFlow_Root ${CMAKE_CURRENT_SOURCE_DIR}/taskflow)
    set(TF_BUILD_EXAMPLES OFF)
    set(TF_BUILD_TESTS OFF)
    add_subdirectory(${TaskFlow_Root})

    list(APPEND ThirdParty_Libs Taskflow)
endif ()

##############
### Optick ###
##############
if (${USE_OPTICK})
    set(Optick_Root ${CMAKE_CURRENT_SOURCE_DIR}/optick})
    option(BUILD_WITH_OPTICK "Enable Optick usage" ON)
    add_definitions(-DBUILD_WITH_OPTICK=1)
    add_subdirectory(${Optick_Root})
    set_property(TARGET OptickCore PROPERTY FOLDER "ThirdPartyLibraries")

    list(APPEND ThirdParty_Include_Dirs ${Optick_Root}/src)
    list(APPEND ThirdParty_Libs Optick)
endif ()

#####################
#### EasyProfiler ###
#####################
if (${USE_EASY_PROFILER})
    set(EasyProfiler_Root ${CMAKE_CURRENT_SOURCE_DIR}/easy_profiler)
    option(BUILD_WITH_EASY_PROFILER "Enable EasyProfiler usage" ON)
    set(EASY_PROFILER_NO_GUI ON)
    add_definitions(-DBUILD_WITH_EASY_PROFILER=1)
    add_subdirectory(${EasyProfiler_Root})

    list(APPEND ThirdParty_Include_Dirs ${EasyProfiler_Root}/easy_profiler_core/include)
    list(APPEND ThirdParty_Libs easy_profiler)
endif ()

################
#### Bullet ####
################
add_subdirectory(bullet3)
#if (${USE_BULLET})
#    set(Bullet_Root ${CMAKE_CURRENT_SOURCE_DIR}/bullet)
#    add_subdirectory(${Bullet_Root})
#
list(APPEND ThirdParty_Include_Dirs ${CMAKE_CURRENT_SOURCE_DIR}/bullet3/src)
list(APPEND ThirdParty_Libs "LinearMath;Bullet3Common;BulletInverseDynamics;BulletCollision;BulletDynamics;BulletSoftBody")
#endif ()

set(ThirdParty_Libs ${ThirdParty_Libs} PARENT_SCOPE)
set(ThirdParty_Include_Dirs ${ThirdParty_Include_Dirs} PARENT_SCOPE)
