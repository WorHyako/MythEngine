cmake_minimum_required(VERSION 3.24)

# List contain libraries which require hard order
list(APPEND CorrectOrder
        Vulkan::Vulkan
        volk
        glad
        glfw
        glslang
        SPIRV
        glslang-default-resource-limits)

option(LOG_LIBRARY_SORTING "" OFF)

function (sort_library LibraryList)
    list(APPEND LibraryList ${ARGN})

    if(LOG_LIBRARY_SORTING OR LOG_ALL)
        message(STATUS "Unordered library list")
        foreach (each ${LibraryList})
            message(STATUS ${each})
        endforeach ()
    endif ()

    list(REMOVE_DUPLICATES LibraryList)
    set(OrderedList "")

    foreach (CurrentOrder ${CorrectOrder})
        list(FIND LibraryList ${CurrentOrder} ElementIndex)

        if(${ElementIndex} GREATER -1)
            list(GET LibraryList ${ElementIndex} Element)
            list(APPEND OrderedList ${Element})
            list(REMOVE_ITEM LibraryList ${Element})
        endif ()

    endforeach ()

    list(APPEND OrderedList ${LibraryList})

    if(${LOG_LIBRARY_SORTING} OR ${LOG_ALL})
        message(STATUS "Ordered library list")
        foreach (each ${OrderedList})
            message(STATUS ${each})
        endforeach ()
    endif ()

    set(ThirdParty_Libs ${OrderedList} PARENT_SCOPE)
endfunction()
