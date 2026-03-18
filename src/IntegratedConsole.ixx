module;

#include <sstream>
#include <deque>
#include <string>
#include <iostream>
#include <imgui.h>

export module IntegratedConsole;

export class IntegratedConsole
{
public:
    IntegratedConsole();
    void update();

private:
    std::stringstream redirectStream;
    std::deque<std::string> lines;
    const size_t maxLines = 200;
};

// ---- Implementation ----

IntegratedConsole::IntegratedConsole() {
    std::cout.rdbuf(redirectStream.rdbuf());
    std::cerr.rdbuf(redirectStream.rdbuf());
    std::clog.rdbuf(redirectStream.rdbuf());
}

void IntegratedConsole::update()
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
