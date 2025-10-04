#pragma once

#ifdef _DEBUG
#ifndef TOOLS_PANELS_HPP
#define TOOLS_PANELS_HPP

#include "Panels.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Controller.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {

			class Tools : public IPanel {
			private:
			public:
				Tools();
				~Tools() override = default;

				void nextWindowSettings() override;

				void onUpdate(AppTiming timing) override;
			};
		}
	}
}

#endif
#endif