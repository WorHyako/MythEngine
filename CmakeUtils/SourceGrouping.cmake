cmake_minimum_required(VERSION 3.24)

function(get_all_cmake_targets OutVar CurrentDir)
    get_property(Targets DIRECTORY ${CurrentDir} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(Subdirs DIRECTORY ${CurrentDir} PROPERTY SUBDIRECTORIES)

    foreach (Subdir ${Subdirs})
        get_all_cmake_targets(SubdirTargets ${Subdir})
        list(APPEND Targets ${SubdirTargets})
    endforeach ()

    set(${OutVar} ${Targets} PARENT_SCOPE)
endfunction()

function(group_targets)
    set(prefix ARG)
    set(noValues CUSTOM_TARGET)
    set(singleValues TARGETS_PATH)

    cmake_parse_arguments(${prefix}
            "${noValues}"
            "${singleValues}"
            "${multiValues}"
            ${ARGN})

    #########################
    #   Target collecting   #
    #########################
    file(RELATIVE_PATH TargetOwner ${ARG_TARGETS_PATH}/.. ${ARG_TARGETS_PATH})
    if (NOT ${ARG_CUSTOM_TARGET})
        get_all_cmake_targets(Targets "${ARG_TARGETS_PATH}")
        set_target_properties(${Targets} PROPERTIES FOLDER "ThirdParty/${TargetOwner}")
    else ()
        set(Targets ${TargetOwner})
    endif ()
#    message(WARNING "For owner: ${TargetOwner}\nFounded targets: ${Targets}")

    ##########################
    #   Headers collecting   #
    ##########################
    foreach (Target IN LISTS Targets)
        #        get_target_property(TargetSources ${Target} SOURCES)
        get_target_property(TargetIncludeDirs ${Target} INCLUDE_DIRECTORIES)
        set(Headers "")
        foreach (dir IN LISTS TargetIncludeDirs)
            file(GLOB header CONFIGURE_DEPENDS ${dir}/*.h*)
            list(APPEND Headers ${header})
        endforeach ()

        #                source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX ${TargetOwner} FILES ${Headers})
        source_group("${TargetOwner} headers" FILES ${Headers})
        #        message(WARNING "For target '${Target}' was founded headers: ${Headers}")

        get_target_property(TargetSources ${Target} SOURCES)
        source_group("${TargetOwner} sources" FILES ${TargetSources})
    endforeach ()

endfunction()

function(CREATE_SOURCE_GROUP IN_PATH)
    file(GLOB_RECURSE SOURCES_FOR_GROUP "${IN_PATH}/*")
    foreach (source IN LISTS SOURCES_FOR_GROUP)
        get_filename_component(source_path ${source} PATH)
        string(REPLACE ${PROJECT_SOURCE_DIR} "" relative_path "${source_path}")
        if (${LOG_GROUP_SOURCES} OR ${LOG_ALL})
            message(STATUS "Group: ${relative_path}. \nSource: ${source}")
        endif ()
        source_group("${relative_path}" FILES "${source}")
    endforeach ()
endfunction()