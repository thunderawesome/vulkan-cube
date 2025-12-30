# Compile to temporary SPV files
execute_process(COMMAND ${GLSLC_PATH} ${INPUT_VERT} -o ${INPUT_VERT}.spv)
execute_process(COMMAND ${GLSLC_PATH} ${INPUT_FRAG} -o ${INPUT_FRAG}.spv)

# Read as HEX but specifically as 4-byte words to preserve endianness
file(READ "${INPUT_VERT}.spv" VERT_HEX HEX)
file(READ "${INPUT_FRAG}.spv" FRAG_HEX HEX)

function(format_to_uint32_array HEX_DATA OUT_VAR)
    # Split the long hex string into 8-character chunks (4 bytes = uint32)
    string(REGEX MATCHALL "........" CHUNKS "${HEX_DATA}")
    set(FORMATTED "")
    foreach(CHUNK ${CHUNKS})
        # Vulkan/SPIR-V is stored in a specific byte order. 
        # We need to flip the bytes in the chunk to make them valid C++ uint32 literals
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