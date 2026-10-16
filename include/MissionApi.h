#pragma once
#include <ArduinoJson.h>
#include <algorithm>
#include <string>
#include <vector>

// Transport-independent user mission operations. The system boot file is private.
namespace MissionApi {
constexpr size_t MAX_BYTES = 16384, MAX_STEPS = 64;
struct Storage {
  virtual ~Storage() = default;
  virtual bool read(const std::string&, std::string&) = 0;
  virtual bool write(const std::string&, const std::string&) = 0;
  virtual bool exists(const std::string&) = 0;
  virtual bool remove(const std::string&) = 0;
  virtual bool rename(const std::string&, const std::string&) = 0;
  virtual std::vector<std::string> names() = 0;
};
inline bool validName(const std::string& name) {
  if (name.empty() || name.size() > 32 || name == "boot") return false;
  for (unsigned char c : name)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
  return true;
}
inline std::string path(const std::string& name) { return "/" + name + ".mission"; }
inline bool fail(JsonDocument& out, const char* error) {
  out["ok"] = false; out["error"] = error; return true;
}
inline bool validStep(const std::string& line) {
  if (line.empty() || line.size() > 255 || line.find_first_of("\r\n") != std::string::npos) return false;
  JsonDocument cmd;
  if (deserializeJson(cmd, line) || !cmd.is<JsonObject>() || !cmd["T"].is<int>()) return false;
  // A mission must not recurse into missions or edit its own storage while running.
  int t = cmd["T"];
  return !(t >= 300 && t <= 399) && !cmd.containsKey("rid");
}
inline std::vector<std::string> lines(const std::string& data) {
  std::vector<std::string> result;
  size_t pos = 0;
  while (pos < data.size()) {
    size_t end = data.find('\n', pos);
    if (end == std::string::npos) end = data.size();
    auto line = data.substr(pos, end - pos);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    result.push_back(line); pos = end + 1;
  }
  return result;
}
inline bool save(Storage& fs, const std::string& target, const std::vector<std::string>& content) {
  std::string data;
  for (const auto& line : content) data += line + '\n';
  if (data.size() > MAX_BYTES || content.size() > MAX_STEPS + 1) return false;
  const auto temp = target + ".edit";
  if (!fs.write(temp, data)) { fs.remove(temp); return false; }
  std::string check;
  if (!fs.read(temp, check) || check != data || !fs.rename(temp, target)) {
    fs.remove(temp); return false;
  }
  return true;
}
inline bool handle(Storage& fs, const JsonDocument& in, JsonDocument& out, bool running) {
  int t = in["T"] | -1;
  if (t < 300 || t > 310) return false;
  out["T"] = t; out["ok"] = true;
  if (in["rid"].is<uint32_t>()) out["rid"] = in["rid"];
  if (running) return fail(out, "mission_running_stop_first");
  int offset = 0;
  if (!in["offset"].isNull()) {
    if (!in["offset"].is<int>() || (in["offset"].as<int>() < 0 || in["offset"].as<int>() > 65535)) return fail(out, "invalid_offset");
    offset = in["offset"];
  }
  if (t == 300) {
    auto names = fs.names();
    names.erase(std::remove_if(names.begin(), names.end(), [](const std::string& n){return !validName(n);}), names.end());
    std::sort(names.begin(), names.end());
    auto list = out["missions"].to<JsonArray>();
    size_t end = std::min(names.size(), size_t(offset) + 16);
    for (size_t i = size_t(offset); i < end; ++i) list.add(names[i]);
    out["total"] = names.size(); out["next"] = end < names.size() ? int(end) : -1;
    return true;
  }
  if (!in["name"].is<const char*>()) return fail(out, "invalid_mission_name");
  const std::string name = in["name"].as<const char*>();
  if (!validName(name)) return fail(out, "invalid_mission_name");
  const auto target = path(name);
  out["name"] = name;
  if (t == 310) {
    std::string source = in["source"] | "";
    if (!validName(source) || source.find("draft_") != 0 || source == name)
      return fail(out, "invalid_draft_name");
    if (fs.exists(target) && !(in["overwrite"].is<bool>() && in["overwrite"].as<bool>()))
      return fail(out, "mission_exists");
    std::string data;
    if (!fs.read(path(source), data)) return fail(out, "draft_not_found");
    auto content = lines(data);
    if (content.size() < 2 || content.size() > MAX_STEPS + 1) return fail(out, "invalid_step_count");
    for (size_t i = 1; i < content.size(); ++i)
      if (!validStep(content[i])) return fail(out, "invalid_step_json");
    if (!fs.rename(path(source), target)) return fail(out, "storage_write_failed");
    out["steps"] = content.size() - 1;
    return true;
  }
  if (t == 301) {
    if (fs.exists(target)) return fail(out, "mission_exists");
    if (!in["intro"].is<const char*>()) return fail(out, "invalid_intro");
    std::string intro = in["intro"].as<const char*>();
    if (intro.size() > 96 || intro.find_first_of("\r\n") != std::string::npos) return fail(out, "invalid_intro");
    if (!save(fs, target, {intro})) return fail(out, "storage_write_failed");
    out["steps"] = 0; return true;
  }
  std::string data;
  if (!fs.read(target, data)) return fail(out, "mission_not_found_or_too_large");
  auto content = lines(data);
  if (content.empty()) return fail(out, "invalid_mission_file");
  const int count = int(content.size()) - 1;
  if (t == 302) {
    out["intro"] = content[0]; out["total"] = count;
    auto steps = out["steps"].to<JsonArray>();
    int end = std::min(count, offset + 8);
    for (int i = offset; i < end; ++i) steps.add(content[size_t(i) + 1]);
    out["next"] = end < count ? end : -1;
    return true;
  }
  if (t == 309) {
    if (!fs.remove(target)) return fail(out, "storage_write_failed");
    return true;
  }
  int step = 0;
  if (t == 304 || t == 305 || t == 306 || t == 307) {
    if (!in["step"].is<int>()) return fail(out, "invalid_step_index");
    step = in["step"];
    if (step < 1 || step > count + (t == 304 ? 1 : 0)) return fail(out, "invalid_step_index");
  }
  if (t == 307 || t == 308) {
    if (count < 1) return fail(out, "empty_mission");
    for (size_t i = 1; i < content.size(); ++i)
      if (!validStep(content[i])) return fail(out, "invalid_step_json");
    if (t == 308 && (!in["interval"].is<int>() || in["interval"].as<int>() < 0 ||
        in["interval"].as<int>() > 60000 || !in["loop"].is<int>() ||
        !(in["loop"].as<int>() == -1 || (in["loop"].as<int>() >= 1 && in["loop"].as<int>() <= 10000))))
      return fail(out, "invalid_run_parameters");
    out["status"] = "started"; return true;
  }
  if (t == 303 || t == 304 || t == 305) {
    if (!in["json"].is<const char*>()) return fail(out, "invalid_step_json");
    std::string line = in["json"].as<const char*>();
    if (!validStep(line)) return fail(out, "invalid_step_json");
    if (t != 305 && count >= int(MAX_STEPS)) return fail(out, "too_many_steps");
    if (t == 303) content.push_back(line);
    if (t == 304) content.insert(content.begin() + step, line);
    if (t == 305) content[size_t(step)] = line;
  } else if (t == 306) content.erase(content.begin() + step);
  if (!save(fs, target, content)) return fail(out, "storage_write_failed");
  out["steps"] = content.size() - 1;
  return true;
}
}  // namespace MissionApi
