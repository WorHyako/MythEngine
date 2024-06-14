set(ThirdParty_Libs)
set(ThirdParty_IncludeDirs)

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

        add_subdirectory(${Assimp_Root})
    endblock()

    set_target_properties(assimp PROPERTIES FOLDER "ThirdParty/assimp")

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

    add_subdirectory(${Glfw_Root})

    set_target_properties(glfw PROPERTIES FOLDER "ThirdParty/glfw")

    list(APPEND ThirdParty_Libs glfw)
endif ()

###############
#   glslang   #
###############
if (USE_GLSLANG)
    set(Glslang_Root ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
    set(ENABLE_CTEST OFF)
    set(ENABLE_GLSLANG_JS OFF)

    add_subdirectory(${Glslang_Root})

    set_target_properties(glslang PROPERTIES FOLDER "ThirdParty/glslang")

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

    add_subdirectory(${Volk_Root})

    set_target_properties(volk PROPERTIES FOLDER "ThirdParty/volk")

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
    file(GLOB ImGui_Sources ${ImGui_Root}/*.*pp)
    if (APPLE)
        list(APPEND ImGui_Sources
                ${ImGui_Root}/backends/imgui_impl_osx.mm
                ${ImGui_Root}/backends/imgui_impl_metal.mm)
    elseif (WIN32)
        list(APPEND ImGui_Sources
                ${ImGui_Root}/backends/imgui_impl_glfw.cpp
                ${ImGui_Root}/backends/imgui_impl_opengl3.cpp)
    endif ()

    add_library(imgui STATIC)
    target_sources(imgui
            PRIVATE ${ImGui_Sources})
    target_include_directories(imgui
            PUBLIC
            ${ImGui_Root}
            ${ImGui_Root}/backends)
    target_link_libraries(imgui
            PUBLIC glfw)

    set_target_properties(imgui PROPERTIES FOLDER "ThirdParty/ImGui")

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

    add_library(imguizmo)

    target_sources(imguizmo
            PRIVATE ${Imguizmo_Sources})

    target_include_directories(imgui
            PUBLIC ${ImGuizmo_Root})

    target_link_libraries(imguizmo
            PUBLIC imgui)

    set_target_properties(imguizmo PROPERTIES FOLDER "ThirdParty/ImGui")

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

    set_target_properties(stb PROPERTIES FOLDER "ThirdParty/stb")

    list(APPEND ThirdParty_Libs stb)
endif ()

############
#   mINI   #
############
if (${USE_MINI})
    set(Mini_Root ${CMAKE_CURRENT_SOURCE_DIR}/mINI/src)

    add_library(mINI INTERFACE)

    target_include_directories(mINI
            INTERFACE ${Mini_Root})

    set_target_properties(mINI PROPERTIES FOLDER "ThirdParty/mINI")

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

    set_target_properties(glad PROPERTIES FOLDER "ThirdParty/glad")

    list(APPEND ThirdParty_Libs glad)
endif ()

###########
#   glm   #
###########
if (USE_GLM)
    set(Glm_Root ${CMAKE_CURRENT_SOURCE_DIR}/glm)

    add_subdirectory(${Glm_Root})

    set_target_properties(glm PROPERTIES FOLDER "ThirdParty/glm")

    list(APPEND ThirdParty_Libs glm)
endif ()

###########
#   gli   #
###########
if (USE_GLI)
    set(Gli_Root ${CMAKE_CURRENT_SOURCE_DIR}/gli)
    set(GLI_TEST_ENABLE OFF)

    add_subdirectory(${Gli_Root})

    set_target_properties(gli PROPERTIES FOLDER "ThirdParty/gli")

    list(APPEND ThirdParty_Libs gli)
endif ()

#############
#   tracy   #
#############
if (USE_TRACY)
    set(Tracy_Root ${CMAKE_CURRENT_SOURCE_DIR}/tracy)
    set(TRACY_ON_DEMAND ON)

    add_subdirectory(${Tracy_Root})

    set_target_properties(TracyClient PROPERTIES FOLDER "ThirdParty/Tracy")

    list(APPEND ThirdParty_Libs Tracy::TracyClient)
endif ()

################
#   TaskFlow   #
################
if (USE_TASK_FLOW)
    set(TaskFlow_Root ${CMAKE_CURRENT_SOURCE_DIR}/taskflow)
    set(TF_BUILD_EXAMPLES OFF)
    set(TF_BUILD_TESTS OFF)

    add_subdirectory(${TaskFlow_Root})

    set_target_properties(Taskflow PROPERTIES FOLDER "ThirdParty/Taskflow")

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

    add_subdirectory(${Optick_Root})

    target_compile_definitions(Optick
            PUBLIC BUILD_WITH_OPTICK)

    set_target_properties(Optick PROPERTIES FOLDER "ThirdParty/optick")

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

    add_subdirectory(${EasyProfiler_Root})

    set_target_properties(easy_profiler PROPERTIES FOLDER "ThirdParty/easy_profiler")

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

        add_subdirectory(${Bullet3_Root})
    endblock()

    add_library(bullet3::LinearMath ALIAS LinearMath)
    add_library(bullet3::Common ALIAS Bullet3Common)
    add_library(bullet3::InverseDynamics ALIAS BulletInverseDynamics)
    add_library(bullet3::Collision ALIAS BulletCollision)
    add_library(bullet3::Dynamics ALIAS BulletDynamics)
    add_library(bullet3::SoftBody ALIAS BulletSoftBody)

    set_target_properties(LinearMath
            Bullet3Common
            BulletInverseDynamics
            BulletCollision
            BulletDynamics
            BulletSoftBody
            PROPERTIES FOLDER "ThirdParty/Bullet3")

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
