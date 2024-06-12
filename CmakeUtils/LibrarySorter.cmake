cmake_minimum_required(VERSION 3.24)

# List contain libraries which require hard order
list(APPEND Correct_Order
        Vulkan::Vulkan
        volk
        glad
        glfw
        glslang
        SPIRV
        glslang-default-resource-limits)

option(LOG_LIBRARY_SORTING "" OFF)

#
function (sort_library Library_List)
    list(APPEND Library_List ${ARGN})

    if(${LOG_LIBRARY_SORTING} OR ${LOG_ALL})
        message(STATUS "Unordered library list")
        foreach (each ${Library_List})
            message(STATUS ${each})
        endforeach (each)
    endif ()

    list(REMOVE_DUPLICATES Library_List)
    set(Ordered_List "")

    foreach (Current_Order ${Correct_Order})
        list(FIND Library_List ${Current_Order} Element_Index)

        if(${Element_Index} GREATER -1)
            list(GET Library_List ${Element_Index} Element)
            list(APPEND Ordered_List ${Element})
            list(REMOVE_ITEM Library_List ${Element})
        endif ()

    endforeach (Current_Order)

    list(APPEND Ordered_List ${Library_List})

    if(${LOG_LIBRARY_SORTING} OR ${LOG_ALL})
        message(STATUS "Ordered library list")
        foreach (each ${Ordered_List})
            message(STATUS ${each})
        endforeach (each)
    endif ()

    set(ThirdParty_Libs ${Ordered_List} PARENT_SCOPE)
endfunction(sort_library)
