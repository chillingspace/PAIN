#pragma once

#ifdef _DEBUG
#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "Panels.h"
#include "CoreSystems/Renderer/RendererLayer.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {

			class ViewportPanel : public IPanel {
			public:
				ViewportPanel();

				void nextWindowSettings() override;


				void onAttach() override;

				void onUpdate(AppTiming timing) override;

				// Provide texture from renderer
				void setRenderTexture(ImTextureID texID, int width, int height);

				//ImVec2 getViewportSize();

				bool isFocused = false;        // window focus (incl. children)
				bool contentHovered = false;   // mouse over the image area
				bool wantsInput() const { return isFocused || contentHovered; }

			private:
				ImTextureID renderTexture;
				int texWidth = 0;
				int texHeight = 0;
			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
