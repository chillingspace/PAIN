#pragma once

#include "pch.h"

namespace PAIN {

	struct Light {
		glm::vec3 position{};
		glm::vec3 L_intensity = glm::vec3(0.1f);
	};

	class LightSources {
	private:
		LightSources() = default;
		~LightSources() = default;

		std::unordered_map<std::string, Light> sources;

	public:
		bool lightsOn = true;		// global switch to toggle lights

		static constexpr int MAX_LIGHT_SOURCES = 16;		// remember to set in fragment shader if this is changed
		glm::vec3 AMBIENT_LIGHT = glm::vec3(0.1f);

		/**
		 * get singleton instance.
		 * 
		 * \return 
		 */
		static LightSources& get() {
			static LightSources instance;
			return instance;
		}

		/**
		 * returns if creation was successful. not throwing exception so that imgui can show error msg to user instead.
		 * 
		 * \param ref
		 * \return 
		 */
		bool create(const std::string& ref) {
			if (sources.find(ref) != sources.end()) {
				PN_CORE_ERROR("Light reference {0} already exists", ref);
				return false;
			}
			if (sources.size() >= MAX_LIGHT_SOURCES) {
				PN_CORE_ERROR("Too many light sources!");
				return false;
			}
			sources[ref] = Light();
			return true;
		}

		/**
		 * get light object using ref.
		 * 
		 * \param ref
		 * \return 
		 */
		std::optional<std::reference_wrapper<Light>> get(const std::string& ref) {
			auto it = sources.find(ref);
			if (it == sources.end()) {
				return std::nullopt;
			}
			return std::ref(it->second);
		}

		std::vector<std::reference_wrapper<Light>> getAll() {
			std::vector<std::reference_wrapper<Light>> out;
			out.reserve(sources.size());

			for (auto& [k, v] : sources) {
				out.push_back(std::ref(v));
			}

			return out;
		}

		std::vector<Light> getAllCopy() {
			std::vector<Light> out;
			out.reserve(sources.size());

			for (const auto [k, v] : sources) {
				out.push_back(std::ref(v));
			}

			return out;
		}

		int getCount() const {
			return sources.size();
		}
	};

}

