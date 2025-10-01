/**
 * @file GraphicsSettings.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-10-02
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once


namespace PAIN {
	class GraphicsSettings {
	private:
		GraphicsSettings() = default;
		~GraphicsSettings() = default;
	public:
		static GraphicsSettings& get() {
			static GraphicsSettings instance;
			return instance;
		}

		// softer shadows look better, but requires more VRAM
		enum class SHADOW_TYPES {
			SOFTEST,
			SOFT,
			HARD
		};

		// width = height for shadow maps. dont follow screen resolution
		const std::unordered_map<SHADOW_TYPES, int> SHADOW_MAP_WIDTHS{
			{SHADOW_TYPES::SOFTEST, 4096},
			{SHADOW_TYPES::SOFT, 2048},
			{SHADOW_TYPES::HARD, 1024}
		};

		// actual settings
		SHADOW_TYPES shadow_type = SHADOW_TYPES::SOFTEST;

	};
}