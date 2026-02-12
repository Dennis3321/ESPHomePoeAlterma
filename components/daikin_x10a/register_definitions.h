#pragma once
#include <vector>
#include <cstdint>

inline constexpr uint32_t REGISTER_SCAN_INTERVAL_MS = 30000;

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
};
