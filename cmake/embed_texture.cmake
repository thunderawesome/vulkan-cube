file(READ ${INPUT_TEXTURE} RAW_HEX HEX)
string(REGEX MATCHALL ".." PAIRS "${RAW_HEX}")
foreach(PAIR ${PAIRS})
    string(APPEND TEXTURE_ARRAY "0x${PAIR},")
endforeach()

file(WRITE ${OUTPUT_HEADER} "
#pragma once
#include <vector>
#include <cstdint>

static const std::vector<uint8_t> CONTAINER_JPG_DATA = { ${TEXTURE_ARRAY} };
")