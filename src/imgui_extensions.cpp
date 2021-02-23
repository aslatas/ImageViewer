#include "imgui_extensions.h"
#include "imgui/imgui_internal.h"

ImGuiID ImGui::SetUpInitialDockSpace(const char* tool_panel_name, float size_ratio)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	
	ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	
	char label[32];
	ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(label, NULL, host_window_flags);
	ImGui::PopStyleVar(3);
	
	static bool is_set_up = false;
	ImGuiID dockspace_id = ImGui::GetID("DockSpace");
	if (!is_set_up)
	{
		is_set_up = true;
		DockBuilderRemoveNode(dockspace_id);
		DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);
		ImGuiID main_id = dockspace_id;
		ImGuiID right_id = DockBuilderSplitNode(main_id, ImGuiDir_Right, size_ratio, 0, &main_id);
		
		DockBuilderDockWindow(tool_panel_name, right_id);
		ImGuiDockNode* main_node = DockBuilderGetNode(dockspace_id);
		main_node->LocalFlags |= ImGuiDockNodeFlags_NoSplit;
		ImGuiDockNode* right_node = DockBuilderGetNode(right_id);
		right_node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDocking | ImGuiDockNodeFlags_NoSplit;
		DockBuilderFinish(dockspace_id);
	}
	DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None, 0);
	ImGui::End();
	
	return dockspace_id;
}