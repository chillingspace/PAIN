#pragma once

#include "../Core/Core.h"

namespace PAIN {

	class PAIN_API Application
	{ 
		public:
			Application();
			virtual ~Application();

			void Run();
	};

	// Defined in client
	Application* CreateApplication();


}

