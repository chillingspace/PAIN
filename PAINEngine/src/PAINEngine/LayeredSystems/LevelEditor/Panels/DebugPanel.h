#pragma once

#ifdef _DEBUG
#ifndef DEBUG_PANEL_HPP
#define DEBUG_PANEL_HPP

#include "Panels.h"


namespace PAIN {
	namespace Editor {
		namespace Panel {

			class DebugPanel : public IPanel {
			public:
				DebugPanel();
				~DebugPanel() override = default;
				void nextWindowSettings() override; 
				void onUpdate(AppTiming timing) override;

			private:
				std::vector<float> frameTimes;

			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
