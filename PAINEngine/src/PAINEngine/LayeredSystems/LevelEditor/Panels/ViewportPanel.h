#pragma once

#ifdef _DEBUG
#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "Panels.h"
#include "CoreSystems/Renderer/sRenderer.h"

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

				// Get time scale for simulation (0.0 when paused, 1.0 when playing)
				float getTimeScale() const;

				// Set simulation state programmatically (true = paused, false = playing)
				void setSimulationState(bool paused) { isSimulationPaused = paused; }

				bool isFocused = false;        // window focus (incl. children)
				bool contentHovered = false;   // mouse over the image area
				bool wantsInput() const { return isFocused || contentHovered; }

			private:
				ImTextureID renderTexture;
				int texWidth;
				int texHeight;
				bool isInputPaused;           // Controls input forwarding
				bool isSimulationPaused; // Controls scene simulation
			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
