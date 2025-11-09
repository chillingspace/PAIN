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

				//popup
				std::function<void(std::any const&)> createNewScenePopUp(std::string const& popup_id);
				std::function<void(std::any const&)> saveAsPopUp(std::string const& popup_id);

			public:
				Tools();
				~Tools() override = default;

				void nextWindowSettings() override;


				void onAttach() override;

				void onUpdate(AppTiming timing) override;
			};
		}
	}
}

#endif
#endif