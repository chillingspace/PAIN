#pragma once

#ifndef PDEBUG

#ifndef IMGUI_LAYER_HPP
#define IMGUI_LAYER_HPP

#include "../Applications/AppLayer.h"

#include <../vendor/ImGui/headers/imgui.h>
#include <../vendor/ImGui/headers/imgui_impl_glfw.h>
#include <../vendor/ImGui/headers/imgui_impl_opengl3.h>

namespace PAIN {

	class ImGuiLayer : public AppLayer 
	{

	public:
		ImGuiLayer();
		~ImGuiLayer();

		void onAttach();
		void onDetach();
		void onUpdate();
		void onEvent(Event::Event& event);

	private:
		float m_Time;

	};

}

#endif //IMGUI_LAYER_HPP

#endif