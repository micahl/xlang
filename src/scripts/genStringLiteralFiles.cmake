# Script variables:
#   out_h   - path to output header file
#   out_cpp - path to output implementation file
#   namespace - namespace to put declarations in 
#   glob_expression - glob expression for files to include in the output files

foreach(outfile ${out_h} ${out_cpp})
    file(WRITE ${outfile} "// This file was generated by CMake\n\n")
endforeach()

file(APPEND ${out_h}   "#pragma once\n\n")
file(APPEND ${out_cpp} "#include \"pch.h\"\n\n")

foreach(outfile ${out_h} ${out_cpp})
    file(APPEND ${outfile} "namespace ${namespace} {\n\n")
endforeach()

file(GLOB GLOB_FILES "${glob_expression}")
set(max_string_len 16000) # should this be configurable?
foreach(globfile ${GLOB_FILES})
    get_filename_component(globfile_name ${globfile} NAME_WE)
    file(READ ${globfile} globfile_content)
    string(LENGTH "${globfile_content}" globfile_size)

    MATH(EXPR array_len "${globfile_size} + 1") # account for null terminator
    file(APPEND ${out_h} "extern char const ${globfile_name}[${array_len}];\n\n")

    set(globfile_content_pos 0)
    file(APPEND ${out_cpp} "extern char const ${globfile_name}[] = ")

    while(globfile_content_pos LESS_EQUAL ${globfile_size})
        string(SUBSTRING "${globfile_content}" ${globfile_content_pos} ${max_string_len} globfile_content_substring)
        file(APPEND ${out_cpp} "R\"xyz(${globfile_content_substring})xyz\" ")    
        math(EXPR globfile_content_pos "${globfile_content_pos} + ${max_string_len}")
    endwhile()

    file(APPEND ${out_cpp} ";\n\n")
endforeach()

foreach(outfile ${out_h} ${out_cpp})
    file(APPEND ${outfile} "}\n\n")
endforeach()
