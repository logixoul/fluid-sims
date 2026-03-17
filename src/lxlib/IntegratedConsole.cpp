#include "precompiled.h"
#include "IntegratedConsole.h"
#include <deque>
#include <sstream>
#include <imgui.h>

IntegratedConsole::IntegratedConsole() {
	// Redirect cout, cerr and clog into our stringstream
	std::cout.rdbuf(redirectStream.rdbuf());
	std::cerr.rdbuf(redirectStream.rdbuf());
	std::clog.rdbuf(redirectStream.rdbuf());
}

void IntegratedConsole::update()
{
	// Read all current contents from the redirect stream
	std::string curContents = redirectStream.str();
	// Clear the underlying stringstream so we only process new text next frame
	redirectStream.str("");
	redirectStream.clear();

	if (!curContents.empty()) {
		std::istringstream curContentsStream(curContents);
		std::string line;
		while (std::getline(curContentsStream, line)) {
			lines.push_back(line);
			if (lines.size() > maxLines) lines.pop_front();
		}
	}

	// Draw a dedicated console window
	ImGui::Begin("Console");

	// Optional controls could go here (clear button, copy, etc.)
	if (ImGui::Button("Clear")) {
		lines.clear();
	}

	// Make console scrollable and auto-scroll to bottom on new messages
	ImGui::BeginChild("ConsoleRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	for (const auto& l : lines) {
		ImGui::TextUnformatted(l.c_str());
	}
	// Auto-scroll: move cursor to bottom if at/near bottom
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();

	ImGui::End();
}
