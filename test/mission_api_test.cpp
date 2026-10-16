#include <cassert>
#include <map>
#include <iostream>
#include "MissionApi.h"
#include "SyncCommandValidation.h"
struct MemoryStorage : MissionApi::Storage {
  std::map<std::string, std::string> files;
  bool failWrite = false, failRename = false;
  bool read(const std::string& p, std::string& out) override {
    if (!files.count(p) || files[p].size() > MissionApi::MAX_BYTES) return false;
    out = files[p]; return true;
  }
  bool write(const std::string& p, const std::string& data) override {
    if (failWrite) return false;
    files[p] = data; return true;
  }
  bool exists(const std::string& p) override { return files.count(p) != 0; }
  bool remove(const std::string& p) override { return files.erase(p) != 0; }
  bool rename(const std::string& a, const std::string& b) override {
    if (failRename || !files.count(a)) return false;
    files[b] = files[a]; files.erase(a); return true;
  }
  std::vector<std::string> names() override {
    std::vector<std::string> result;
    for (auto& item : files)
      if (item.first.size() > 9 && item.first.substr(item.first.size()-8) == ".mission")
        result.push_back(item.first.substr(1,item.first.size()-9));
    return result;
  }
};
int main() {
  auto sync = [](const char* input) {
    JsonDocument command; assert(!deserializeJson(command, input));
    return validSyncCommand(command);
  };
  assert(sync(R"({"T":16,"idn":2,"id":[1,2],"pos":[2047,2047],"spd":[100,100],"acc":[10,10]})"));
  assert(!sync(R"({"T":17,"idn":100000,"id":[1],"pos":[2047]})"));
  assert(!sync(R"({"T":17,"idn":2,"id":[1,2],"pos":[2047]})"));
  assert(!sync(R"({"T":17,"idn":2,"id":[1,1],"pos":[2047,2047]})"));
  assert(!sync(R"({"T":37,"idn":1,"id":[1],"pos":[4095]})"));
  assert(!sync(R"({"T":26,"idn":1,"id":[1],"pos":[2047],"spd":[0],"acc":[10],"tor":[150]})"));
  MemoryStorage fs;
  auto call = [&](const char* json, bool running = false) {
    JsonDocument in, out;
    assert(!deserializeJson(in, json));
    assert(MissionApi::handle(fs, in, out, running));
    return out;
  };
  auto out = call(R"({"T":301,"name":"demo","intro":"example","rid":42})");
  assert(out["ok"] && out["rid"] == 42);
  assert(!call(R"({"T":301,"name":"demo","intro":"other"})")["ok"]);
  for (auto name : {"../boot", "boot", "a/b", "", "a.mission"}) assert(!MissionApi::validName(name));
  assert(!call(R"({"T":302,"name":"boot"})")["ok"]);
  assert(!call(R"({"T":301,"name":"x","intro":"bad\nintro"})")["ok"]);
  assert(call(R"({"T":303,"name":"demo","json":"{\"T\":732,\"id\":1}"})")["ok"]);
  assert(!call(R"({"T":303,"name":"demo","json":"{\"T\":308,\"name\":\"demo\"}"})")["ok"]);
  assert(!call(R"({"T":303,"name":"demo","json":"{\"T\":740,\"rid\":1}"})")["ok"]);
  assert(!call(R"({"T":306,"name":"demo","step":0})")["ok"]);
  auto old = fs.files["/demo.mission"];
  fs.failWrite = true;
  assert(!call(R"({"T":305,"name":"demo","step":1,"json":"{\"T\":740}"})")["ok"]);
  assert(fs.files["/demo.mission"] == old); fs.failWrite = false;
  fs.failRename = true;
  assert(!call(R"({"T":305,"name":"demo","step":1,"json":"{\"T\":740}"})")["ok"]);
  assert(fs.files["/demo.mission"] == old); fs.failRename = false;
  assert(call(R"({"T":304,"name":"demo","step":1,"json":"{\"T\":51,\"delay\":10}"})")["ok"]);
  assert(call(R"({"T":302,"name":"demo"})")["total"] == 2);
  assert(call(R"({"T":306,"name":"demo","step":1})")["ok"]);
  assert(call(R"({"T":308,"name":"demo","interval":0,"loop":-1})")["status"] == "started");
  assert(!call(R"({"T":308,"name":"demo","interval":0,"loop":0})")["ok"]);
  assert(!call(R"({"T":309,"name":"demo"})", true)["ok"]);
  assert(call(R"({"T":301,"name":"draft_new","intro":"replacement"})")["ok"]);
  assert(call(R"({"T":303,"name":"draft_new","json":"{\"T\":740}"})")["ok"]);
  assert(!call(R"({"T":310,"source":"draft_new","name":"demo"})")["ok"]);
  fs.failRename = true;
  assert(!call(R"({"T":310,"source":"draft_new","name":"demo","overwrite":true})")["ok"]);
  assert(fs.files["/demo.mission"] == old); fs.failRename = false;
  assert(call(R"({"T":310,"source":"draft_new","name":"demo","overwrite":true})")["ok"]);
  assert(!fs.exists("/draft_new.mission"));
  assert(call(R"({"T":302,"name":"demo"})")["intro"] == "replacement");
  fs.files["/boot.mission"] = "secret configuration\n";
  assert(call(R"({"T":300})")["total"] == 1);
  assert(!call(R"({"T":302,"name":"demo","offset":2147483647})")["ok"]);
  for (int i = 0; i < 63; ++i) assert(call(R"({"T":303,"name":"demo","json":"{\"T\":740}"})")["ok"]);
  assert(!call(R"({"T":303,"name":"demo","json":"{\"T\":740}"})")["ok"]);
  assert(call(R"({"T":302,"name":"demo"})")["steps"].size() == 8);
  assert(call(R"({"T":302,"name":"demo"})")["next"] == 8);
  assert(call(R"({"T":309,"name":"demo"})")["ok"]);
  std::cout << "PASS mission CRUD, bounded pagination, validation, protected boot, atomic draft commit, write/rename failures and running guard\n";
}
