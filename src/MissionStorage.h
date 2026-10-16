#pragma once
#include <LittleFS.h>
#include "MissionApi.h"

class MissionStorage : public MissionApi::Storage {
 public:
  bool read(const std::string& p, std::string& out) override {
    File f = LittleFS.open(p.c_str(), "r");
    if (!f || f.isDirectory() || f.size() > MissionApi::MAX_BYTES) return false;
    out.resize(f.size());
    return out.empty() || f.readBytes(&out[0], out.size()) == out.size();
  }
  bool write(const std::string& p, const std::string& data) override {
    File f = LittleFS.open(p.c_str(), "w");
    if (!f) return false;
    bool ok = f.write(reinterpret_cast<const uint8_t*>(data.data()), data.size()) == data.size();
    f.close(); return ok;
  }
  bool exists(const std::string& p) override { return LittleFS.exists(p.c_str()); }
  bool remove(const std::string& p) override { return LittleFS.remove(p.c_str()); }
  bool rename(const std::string& a, const std::string& b) override { return LittleFS.rename(a.c_str(), b.c_str()); }
  std::vector<std::string> names() override {
    std::vector<std::string> result;
    File root = LittleFS.open("/");
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      std::string name = f.name();
      if (!name.empty() && name[0] == '/') name.erase(0, 1);
      if (!f.isDirectory() && name.size() > 8 && name.substr(name.size() - 8) == ".mission")
        result.push_back(name.substr(0, name.size() - 8));
    }
    return result;
  }
};
