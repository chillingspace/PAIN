/*****************************************************************//**
 * \file   text.h
 * \brief
 *
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/

#pragma once

#include "pch.h"
#include "CoreSystems/Path/Path.h"
#include "Applications/AppSystem.h"
#include <ft2build.h>
#include FT_FREETYPE_H


namespace PAIN {
	class TextRenderer {
	private:
		TextRenderer();
		~TextRenderer();

		static bool initialized;

		static std::shared_ptr<Services> services;
	public:
		// dependency injection
		static void init(std::shared_ptr<Services> s) {
			services = s;
			initialized = true;
		}

		static TextRenderer& get() {
			if (!initialized) {
				throw std::exception("TextRenderer not initialized!");
			}
			static TextRenderer instance;
			return instance;
		}


	};
}