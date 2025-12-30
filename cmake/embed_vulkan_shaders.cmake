# Determine which compiler and flags to use
if(GLSLC_PATH MATCHES "glslc")
    set(COMPILE_COMMAND ${GLSLC_PATH} ${INPUT_VERT} -o ${INPUT_VERT}.spv)
    set(COMPILE_COMMAND_FRAG ${GLSLC_PATH} ${INPUT_FRAG} -o ${INPUT_FRAG}.spv)
else()
    # Fallback for glslangValidator (requires -V to output SPIR-V)
    set(COMPILE_COMMAND ${GLSLC_PATH} -V ${INPUT_VERT} -o ${INPUT_VERT}.spv)
    set(COMPILE_COMMAND_FRAG ${GLSLC_PATH} -V ${INPUT_FRAG} -o ${INPUT_FRAG}.spv)
endif()

# Execute compilation
execute_process(COMMAND ${COMPILE_COMMAND} RESULT_VARIABLE RES1)
execute_process(COMMAND ${COMPILE_COMMAND_FRAG} RESULT_VARIABLE RES2)

if(NOT RES1 EQUAL 0 OR NOT RES2 EQUAL 0)
    message(FATAL_ERROR "Shader compilation failed")
endif()

# Read as HEX
file(READ "${INPUT_VERT}.spv" VERT_HEX HEX)
file(READ "${INPUT_FRAG}.spv" FRAG_HEX HEX)

function(format_to_uint32_array HEX_DATA OUT_VAR)
    string(REGEX MATCHALL "........" CHUNKS "${HEX_DATA}")
    set(FORMATTED "")
    foreach(CHUNK ${CHUNKS})
        # Handle Little Endian swap
        string(SUBSTRING ${CHUNK} 6 2 B1)
        string(SUBSTRING ${CHUNK} 4 2 B2)
        string(SUBSTRING ${CHUNK} 2 2 B3)
        string(SUBSTRING ${CHUNK} 0 2 B4)
        string(APPEND FORMATTED "0x${B1}${B2}${B3}${B4},")
    endforeach()
    set(${OUT_VAR} ${FORMATTED} PARENT_SCOPE)
endfunction()

format_to_uint32_array("${VERT_HEX}" VERT_ARRAY)
format_to_uint32_array("${FRAG_HEX}" FRAG_ARRAY)

file(WRITE ${OUTPUT_HEADER} "
#pragma once
#include <vector>
#include <cstdint>

static const std::vector<uint32_t> CUBE_VERT_SPV = { ${VERT_ARRAY} };
static const std::vector<uint32_t> CUBE_FRAG_SPV = { ${FRAG_ARRAY} };
")