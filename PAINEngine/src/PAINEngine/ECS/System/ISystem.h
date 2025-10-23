#pragma once

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "CoreSystems/Events/Event.h"
#include <Applications/AppSystem.h>


namespace PAIN {
	namespace ECS {
		namespace System {

			//Base ISystem
			class ISystem {
			protected:
				std::weak_ptr<Services> services;

				std::shared_ptr<Services> getServices() const {
					return services.lock();
				}
			public:
				explicit ISystem(std::shared_ptr<Services> svc) {
					services = svc;
				}	
				virtual ~ISystem() = default;

				virtual std::string getSysName() const = 0;
				virtual void onUpdate(AppTiming timing, entt::registry& reg) = 0;
				// Optional override	
				virtual void onEvent(Event::Event& e) {}  
		
				bool enabled = true;
			};
		}
	}
}

#endif