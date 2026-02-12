#pragma once
#include <vector>
#include <cstdint>

inline constexpr uint32_t REGISTER_SCAN_INTERVAL_MS = 30000;

// Convids that produce text values (strings in asString, no numeric dblData)
inline bool is_text_convid(int convid) {
    switch (convid) {
        case 200: case 201: case 203: case 204: case 211: case 217:
        case 300: case 301: case 302: case 303: case 304: case 305: case 306: case 307:
        case 315: case 316:
            return true;
        default:
            return false;
    }
}

class Register {
    public:
        int Mode;
        int convid;
        int offset;
        int registryID;
        int dataSize;
        int dataType;
        const char* label;
        char asString[30];

    Register(int Mode_, int convid_, int offset_, int registryID_,
             int dataSize_, int dataType_, const char* label_)
        : Mode(Mode_), convid(convid_), offset(offset_),
          registryID(registryID_), dataSize(dataSize_),
          dataType(dataType_), label(label_) {
        asString[0] = '\0';
    }

    bool is_text_type() const { return is_text_convid(convid); }
};
