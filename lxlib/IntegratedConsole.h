#pragma once
#include <sstream>
#include <deque>
#include <string>

class IntegratedConsole
{
public:
	IntegratedConsole();
	void update();

private:
	std::stringstream redirectStream;
	std::deque<std::string> lines;
	const size_t maxLines = 200;
};

