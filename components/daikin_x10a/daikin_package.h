// daikin_package.h
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cmath>

#include "register_definitions.h"  // Register + registers[]

class daikin_package {
 public:
  enum class Mode { SEND_REQUEST, RECEIVE };

  daikin_package() = default;
  explicit daikin_package(Mode m) : mode_(m) {}

  // --- Factory helpers ---
  static daikin_package MakeRequest(uint8_t reg_id) {
    daikin_package MyNewPackage(Mode::SEND_REQUEST);
    MyNewPackage.packet_buffer = {0x03, 0x40, reg_id, 0x00};
    MyNewPackage.packet_buffer[3] = crc_(MyNewPackage.packet_buffer.data(), 3);
    return MyNewPackage;
  }

  static daikin_package FromBytes(const std::vector<uint8_t>& bytes) {
    daikin_package MyNewPackage(Mode::RECEIVE);
    MyNewPackage.packet_buffer = bytes;
    return MyNewPackage;
  }

  // --- Buffer access ---
  const std::vector<uint8_t>& buffer() const { return packet_buffer; }
  std::vector<uint8_t>& buffer_mut() { return packet_buffer; }
  void clear() { packet_buffer.clear(); }

  bool empty() const { return packet_buffer.empty(); }
  size_t size() const { return packet_buffer.size(); }

  bool HasMinimalHeader() const { return packet_buffer.size() >= 3; }
  size_t expected_size() const {
    if (!HasMinimalHeader()) return 0;
    return static_cast<size_t>(packet_buffer[2]) + 2;
  }

  bool is_error_frame() const {
    return packet_buffer.size() >= 2 && packet_buffer[0] == 0x15 && packet_buffer[1] == 0xEA;
  }

  bool is_valid_protocol() const { return !packet_buffer.empty() && packet_buffer[0] == 0x40; }

  bool Valid_CRC() const {
    if (packet_buffer.size() < 2) return false;
    const int n = static_cast<int>(packet_buffer.size());
    return crc_(packet_buffer.data(), n - 1) == packet_buffer[n - 1];
  }

  std::string ToHexString() const {
    char b[4];
    std::string out;
    for (uint8_t c : packet_buffer) {
      snprintf(b, sizeof(b), "%02X", c);
      out += b;
      out += ' ';
    }
    return out;
  }

  Mode mode() const { return mode_; }

  // Protocol I: registry ID at byte position 1
  uint8_t registry_id() const { 
    return (packet_buffer.size() > 1) ? packet_buffer[1] : 0; 
  }

  // Protocol I: data starts at byte position 3 (after: 0x40, registry_id, length)
  unsigned data_offset() const { 
    return 3; 
  }

  // =======================================================
  // Converter (op basis van jouw registers[])
  // =======================================================
  void convert_registry_values(uint8_t reg_id) {
    unsigned offset = data_offset();
    
    ESP_LOGI("ESPoeDaikin", "convert_registry_values: registry_id = %d (hex: 0x%02X), data_offset = %u", (int)reg_id, reg_id, offset);
    for (auto &reg : registers) {
      if (static_cast<uint8_t>(reg.registryID) != reg_id) continue;

      const unsigned int idx  = static_cast<unsigned int>(reg.offset) + offset;
      const unsigned int need = idx + static_cast<unsigned int>(reg.dataSize);
      if (need > packet_buffer.size()) continue;

      const uint8_t *input = &packet_buffer[idx];
      ESP_LOGI("ESPoeDaikin", "  Processing %s: offset=%d, idx=%u, byte_at_idx=0x%02X", 
               reg.label, reg.offset, idx, packet_buffer[idx]);
      convert_one_(reg, input);
    }
  }

 private:
  std::vector<uint8_t> packet_buffer;
  Mode mode_{Mode::RECEIVE};

  // --- CRC ---
  static uint8_t crc_(const uint8_t *src, int len) {
    uint8_t b = 0;
    for (int i = 0; i < len; i++) b += src[i];
    return static_cast<uint8_t>(~b);
  }

  // --- numeric helpers ---
  static unsigned short getUnsignedValue_(const uint8_t *data, int dataSize, int cnvflg) {
    if (dataSize == 1) return (unsigned short)data[0];
    if (cnvflg == 0) return (unsigned short)((data[1] << 8) | data[0]);
    return (unsigned short)((data[0] << 8) | data[1]);
  }

  static short getSignedValue_(const uint8_t *data, int datasize, int cnvflg) {
    unsigned short num = getUnsignedValue_(data, datasize, cnvflg);
    short result = (short)num;
    if ((num & 0x8000) != 0) {
      num = (unsigned short)~num;
      num += 1;
      result = (short)((int)num * -1);
    }
    return result;
  }

  static double convertPress2Temp_(double data) { // R32
    double num  = -2.6989493795556E-07 * data * data * data * data * data * data;
    double num2 =  4.26383417104661E-05 * data * data * data * data * data;
    double num3 = -0.00262978346547749 * data * data * data * data;
    double num4 =  0.0805858127503585  * data * data * data;
    double num5 = -1.31924457284073    * data * data;
    double num6 =  13.4157368435437    * data;
    double num7 = -51.1813342993155;
    return num + num2 + num3 + num4 + num5 + num6 + num7;
  }

  static void convertTable200_(const uint8_t *data, char *ret) {
    strcpy(ret, (data[0] == 0) ? "OFF" : "ON");
  }

  static void convertTable203_(const uint8_t *data, char *ret) {
    switch (data[0]) {
      case 0: strcpy(ret, "Normal"); break;
      case 1: strcpy(ret, "Error"); break;
      case 2: strcpy(ret, "Warning"); break;
      case 3: strcpy(ret, "Caution"); break;
      default: strcpy(ret, "-"); break;
    }
  }

  static void convertTable204_(const uint8_t *data, char *ret) {
    const char array[]  = " ACEHFJLPU987654";
    const char array2[] = "0123456789AHCJEF";
    int n1 = (data[0] >> 4) & 15;
    int n2 = (int)(data[0] & 15);
    ret[0] = array[n1];
    ret[1] = array2[n2];
    ret[2] = 0;
  }

  static double convertTable312_(const uint8_t *data) {
    double dbl = (((uint8_t)(7 & (data[0] >> 4))) + (uint8_t)(15U & data[0])) / 16.0;
    if ((data[0] & 0x80) > 0) dbl *= -1.0;
    return dbl;
  }

  static void convertTable315_(const uint8_t *data, char *ret) {
    uint8_t b = (data[0] & 0xF0) >> 4;
    switch (b) {
      case 0: strcpy(ret, "Stop"); break;
      case 1: strcpy(ret, "Heating"); break;
      case 2: strcpy(ret, "Cooling"); break;
      case 4: strcpy(ret, "DHW"); break;
      case 5: strcpy(ret, "Heating + DHW"); break;
      case 6: strcpy(ret, "Cooling + DHW"); break;
      default: strcpy(ret, "-"); break;
    }
  }

  static void convertTable316_(const uint8_t *data, char *ret) {
    uint8_t b = (data[0] & 0xF0) >> 4;
    switch (b) {
      case 0: strcpy(ret, "H/P only"); break;
      case 1: strcpy(ret, "Hybrid"); break;
      case 2: strcpy(ret, "Boiler only"); break;
      default: strcpy(ret, "Unknown"); break;
    }
  }

  static void convertTable217_(const uint8_t *data, char *ret) {
    const char* r217[] = {
      "Fan Only","Heating","Cooling","Auto","Ventilation","Auto Cool","Auto Heat","Dry","Aux.",
      "Cooling Storage","Heating Storage",
      "UseStrdThrm(cl)1","UseStrdThrm(cl)2","UseStrdThrm(cl)3","UseStrdThrm(cl)4",
      "UseStrdThrm(ht)1","UseStrdThrm(ht)2","UseStrdThrm(ht)3","UseStrdThrm(ht)4"
    };
    int idx = (int)data[0];
    ESP_LOGI("ESPoeDaikin", "  convertTable217_: raw_byte=0x%02X (decimal %d), array_size=%d", data[0], idx, (int)(sizeof(r217) / sizeof(r217[0])));
    if (idx >= 0 && idx < (int)(sizeof(r217) / sizeof(r217[0]))) strcpy(ret, r217[idx]);
    else strcpy(ret, "-");
  }

  static void convertTable300_(const uint8_t *data, int tableID, char *ret) {
    uint8_t mask = (uint8_t)(1U << (tableID % 10));
    strcpy(ret, ((data[0] & mask) != 0) ? "ON" : "OFF");
  }

  static void convert_one_(Register &def, const uint8_t *data) {
    def.asString[0] = '\0';

    const int convId = def.convid;
    const int num = def.dataSize;
    double dblData = NAN;

    switch (convId) {
      case 0x00:
        return;

      case 100:
        strncat(def.asString, (const char*)data, (size_t)num);
        return;

      // signed
      case 101: dblData = (double)getSignedValue_(data, num, 0); break;
      case 102: dblData = (double)getSignedValue_(data, num, 1); break;
      case 103: dblData = (double)getSignedValue_(data, num, 0) / 256.0; break;
      case 104: dblData = (double)getSignedValue_(data, num, 1) / 256.0; break;
      case 105: dblData = (double)getSignedValue_(data, num, 0) * 0.1; break;
      case 106: dblData = (double)getSignedValue_(data, num, 1) * 0.1; break;

      case 107:
        dblData = (double)getSignedValue_(data, num, 0) * 0.1;
        if (dblData == -3276.8) { strcpy(def.asString, "---"); return; }
        break;

      case 108:
        dblData = (double)getSignedValue_(data, num, 1) * 0.1;
        if (dblData == -3276.8) { strcpy(def.asString, "---"); return; }
        break;

      // unsigned
      case 151: dblData = (double)getUnsignedValue_(data, num, 0); break;
      case 152: dblData = (double)getUnsignedValue_(data, num, 1); break;
      case 153: dblData = (double)getUnsignedValue_(data, num, 0) / 256.0; break;
      case 154: dblData = (double)getUnsignedValue_(data, num, 1) / 256.0; break;
      case 155: dblData = (double)getUnsignedValue_(data, num, 0) * 0.1; break;
      case 156: dblData = (double)getUnsignedValue_(data, num, 1) * 0.1; break;

      // tables
      case 200: convertTable200_(data, def.asString); return;
      case 203: convertTable203_(data, def.asString); return;
      case 204: convertTable204_(data, def.asString); return;
      case 201:
      case 217: convertTable217_(data, def.asString); return;

      case 300: case 301: case 302: case 303: case 304: case 305: case 306: case 307:
        convertTable300_(data, def.convid, def.asString);
        return;

      case 312:
        dblData = convertTable312_(data);
        break;

      case 315: convertTable315_(data, def.asString); return;
      case 316: convertTable316_(data, def.asString); return;

      case 211:
        if (data[0] == 0) { strcpy(def.asString, "OFF"); return; }
        dblData = (double)data[0];
        break;

      // pressure -> temp
      case 401: dblData = convertPress2Temp_((double)getSignedValue_(data, num, 0)); break;
      case 402: dblData = convertPress2Temp_((double)getSignedValue_(data, num, 1)); break;
      case 403: dblData = convertPress2Temp_((double)getSignedValue_(data, num, 0) / 256.0); break;
      case 404: dblData = convertPress2Temp_((double)getSignedValue_(data, num, 1) / 256.0); break;
      case 405: dblData = convertPress2Temp_((double)getSignedValue_(data, num, 0) * 0.1); break;
      case 406: dblData = convertPress2Temp_((double)getSignedValue_(data, num, 1) * 0.1); break;

      default:
        snprintf(def.asString, sizeof(def.asString), "Conv %d NA", convId);
        return;
    }

    if (!std::isnan(dblData)) {
      snprintf(def.asString, sizeof(def.asString), "%g", dblData);
    }
  }
};
