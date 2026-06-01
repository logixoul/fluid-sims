module;
#include "precompiled.h"
#include <filesystem>
#include "toml.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

export module lxlib.ConfigManager3;

static std::filesystem::path getExecutableDirectory()
{
#if defined(_WIN32)
	std::vector<char> buffer(MAX_PATH);
	auto length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	while (length == buffer.size()) {
		buffer.resize(buffer.size() * 2);
		length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	}
	if (length == 0) {
		throw std::runtime_error("Failed to get executable path");
	}
	return std::filesystem::path(std::string(buffer.data(), length)).parent_path();
#elif defined(__linux__)
	char result[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
	if (count != -1) {
		return std::filesystem::path(std::string(result, count)).parent_path();
	}
#else
	return std::filesystem::current_path();
#endif
}

template<class T> T& getOpt_Base(std::string const& name, T defaultValue) {
	static std::map<std::string, T> m;
	if (!m.count(name)) {
		m[name] = defaultValue;
	}
	return m[name];
}

export namespace lx {
	struct ConfigManager3
	{
private:
	toml::table tbl;
public:
	ConfigManager3(std::string const filePath)
	{
       auto path = std::filesystem::path(filePath);
		if (path.is_relative()) {
			path = getExecutableDirectory() / path;
		}
		tbl = toml::parse_file(path.string());
	}
	void init() // to avoid static initialization order fiasco. Call this at the start of setup() in Sketch.
	{
		ImGuiIO& io = ImGui::GetIO();
		//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	}
	bool getBool(std::string const& name)
	{
		auto val = tbl.at_path("param." + name);
		auto& ref = getOpt_Base<bool>(name, val["default"].value_or(false));
		ImGui::Checkbox(name.c_str(), &ref);
		return ref;
	}
	//int getInt(string const& name, int min, int max, int defaultValue, ImGuiSliderFlags flags = ImGuiSliderFlags_::ImGuiSliderFlags_None);
	// Note: using value_or throughout to handle the "entire table doesn't exist" possibility
	float getFloat(std::string const& name)
	{
		auto subTable = tbl.at_path("param." + name);
		float& ref = getOpt_Base<float>(name, subTable["default"].value_or(0.5));

		ImGui::DragFloat(
			name.c_str(),
			&ref,
			subTable["speed"].value_or(.1),
			subTable["min"].value_or(-100.0),
			subTable["max"].value_or(100.0),
			"%.3f",
			subTable["logarithmic"].value_or(false) ? ImGuiSliderFlags_Logarithmic : ImGuiSliderFlags_None);

		return ref;
	}
	void begin()
	{
		ImGui::Begin("Parameters");
	}
	void end()
	{
		ImGui::End();
	}
  };
}
