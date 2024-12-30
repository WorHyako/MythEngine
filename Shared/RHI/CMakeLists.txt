if(APPLE)
	set(RHI_Vulkan ON)
	set(RHI_OpenGL OFF)
	set(Render_Utils ON)
endif ()

if(WIN32)
	set(RHI_OpenGL ON)
	set(RHI_Vulkan ON)
	set(Render_Utils ON)
endif ()

list(APPEND Rhi_Source_File_Ext
        /*.cpp
        /*.hpp
        /*.ini)

if(${RHI_OpenGL})
	set(Rhi_Sources_Ext ${Rhi_Source_File_Ext})
	list(TRANSFORM Rhi_Sources_Ext PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/OpenGL)
	file(GLOB_RECURSE Rhi_OpenGL_Sources ${Rhi_Sources_Ext})
    list(APPEND Rhi_Sources ${Rhi_OpenGL_Sources})
    list(APPEND Rhi_Include_Dir ${CMAKE_CURRENT_SOURCE_DIR}/OpenGL)
endif()

if(${RHI_Vulkan})
	set(Rhi_Sources_Ext ${Rhi_Source_File_Ext})
	list(TRANSFORM Rhi_Sources_Ext PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan)
	file(GLOB_RECURSE Rhi_Vulkan_Sources ${Rhi_Sources_Ext})
    list(APPEND Rhi_Sources ${Rhi_Vulkan_Sources})
    list(APPEND Rhi_Include_Dir ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan)
endif()

if(${Render_Utils})
	set(Rhi_Sources_Ext ${Rhi_Source_File_Ext})
	list(TRANSFORM Rhi_Sources_Ext PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/RenderUtils)
	file(GLOB_RECURSE Render_Utils_Sources ${Rhi_Sources_Ext})
    list(APPEND Rhi_Sources ${Render_Utils_Sources})
    list(APPEND Rhi_Include_Dir ${CMAKE_CURRENT_SOURCE_DIR}/RenderUtils)
endif ()

set(Rhi_Include_Dir ${Rhi_Include_Dir} PARENT_SCOPE)
set(Rhi_Sources ${Rhi_Sources} PARENT_SCOPE)

set(rhiProto_Root ${CMAKE_CURRENT_SOURCE_DIR}/RHIProto)
quiet_add_subdirectory(${rhiProto_Root})

list(APPEND ThirdParty_IncludeDirs ${rhiProto_Root}/)
list(APPEND ThirdParty_Libs rhiProto)

set(ThirdParty_Libs ${ThirdParty_Libs} PARENT_SCOPE)
set(ThirdParty_IncludeDirs ${ThirdParty_IncludeDirs} PARENT_SCOPE)