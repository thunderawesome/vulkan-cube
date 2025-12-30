# Compile to temporary SPV files
execute_process(COMMAND ${GLSLC_PATH} ${INPUT_VERT} -o ${INPUT_VERT}.spv)
execute_process(COMMAND ${GLSLC_PATH} ${INPUT_FRAG} -o ${INPUT_FRAG}.spv)

# Convert SPV binary to Hex
file(READ "${INPUT_VERT}.spv" VERT_HEX HEX)
file(READ "${INPUT_FRAG}.spv" FRAG_HEX HEX)

# Helper function to format hex as C array
function(format_hex HEX_DATA OUT_VAR)
    string(REGEX MATCHALL ".." PAIRS "${HEX_DATA}")
    set(FORMATTED "")
    foreach(PAIR ${PAIRS})
        string(APPEND FORMATTED "0x${PAIR},")
    endforeach()
    set(${OUT_VAR} ${FORMATTED} PARENT_SCOPE)
endfunction()

format_hex("${VERT_HEX}" VERT_ARRAY)
format_hex("${FRAG_HEX}" FRAG_ARRAY)

file(WRITE ${OUTPUT_HEADER} "
#pragma once
#include <vector>
#include <cstdint>

static const std::vector<uint32_t> CUBE_VERT_SPV = { ${VERT_ARRAY} };
static const std::vector<uint32_t> CUBE_FRAG_SPV = { ${FRAG_ARRAY} };
")