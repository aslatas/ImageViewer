#ifndef IMGUI_EXTENSIONS_H
#define IMGUI_EXTENSIONS_H

#include "imgui/imgui.h"
namespace ImGui
{
	ImGuiID SetUpInitialDockSpace(const char* tool_panel_name, float size_ratio);
	ImVec2 GetDockNodeSize(ImGuiID id);
};
#endif //IMGUI_EXTENSIONS_H
