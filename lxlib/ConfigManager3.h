#pragma once
#include "toml.hpp"
using std::string;

struct ConfigManager3
{
private:
	toml::table tbl;
public:
	ConfigManager3();
	void init(); // to avoid static initialization order fiasco. Call this at the start of setup() in Sketch.
	bool getBool(string const& name);
	//int getInt(string const& name, int min, int max, int defaultValue, ImGuiSliderFlags flags = ImGuiSliderFlags_::ImGuiSliderFlags_None);
	float getFloat(string const& name);
	void begin();
	void end();
};
