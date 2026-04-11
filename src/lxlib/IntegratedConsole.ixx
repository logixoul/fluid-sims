module;
#include "precompiled.h"
#include <deque>

export module lxlib.IntegratedConsole;

export namespace lx {
	class IntegratedConsole
	{
	public:
		IntegratedConsole()
		{
			std::cout.rdbuf(redirectStream.rdbuf());
			std::cerr.rdbuf(redirectStream.rdbuf());
			std::clog.rdbuf(redirectStream.rdbuf());
		}

		void update()
		{
			std::string curContents = redirectStream.str();
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

			ImGui::Begin("Console");

			if (ImGui::Button("Clear")) {
				lines.clear();
			}

			ImGui::BeginChild("ConsoleRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			for (const auto& l : lines) {
				ImGui::TextUnformatted(l.c_str());
			}
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();

			ImGui::End();
		}

	private:
		std::stringstream redirectStream;
		std::deque<std::string> lines;
		const size_t maxLines = 200;
	};
}
