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

				void nextWindowSettings() override; 
				void onUpdate() override;
			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
