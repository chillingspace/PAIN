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

				bool isFocused = false;        // window focus (incl. children)
				bool contentHovered = false;   // mouse over the image area
				bool wantsInput() const { return isFocused || contentHovered; }

			private:
				ImTextureID renderTexture;
				int texWidth;
				int texHeight;
				bool isPaused;  // Added: tracks play/pause state
			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
