#pragma once

#include "pch.h"
#include "./CoreSystems/Renderer/GraphicsSettings.h"

namespace PAIN {

	class Light {
	public:
		enum class SHADOW_TYPES {
			NONE,
			MAPPED,			// expensive
			SCREEN_SPACE,
			NUM_SHADOW_TYPES,
		};

		enum class TYPES {
			POINT,
			DIRECTIONAL,
			SPOTLIGHT,
			NUM_TYPES,
		};

	private:
		static constexpr int MAX_SHADOWMAPPED_LIGHTS = 4;
		static int num_shadowmapped_lights;
		SHADOW_TYPES shadow_type = SHADOW_TYPES::NONE;
		unsigned int shadow_fbo = 0;
		unsigned int shadow_texture = 0;

		void _createShadowMapBuffers() {
			glGenFramebuffers(1, &shadow_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);

			const int tex_width = GraphicsSettings::get().SHADOW_MAP_WIDTHS.at(GraphicsSettings::get().shadow_type);

			// shadow buffer cannot be created like other textures. is not used to store data like pos,color etc. but depth
			glGenTextures(1, &shadow_texture);
			glBindTexture(GL_TEXTURE_2D, shadow_texture);

#ifdef PN_PLATFORM_ANDROID
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, tex_width, tex_width, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
#else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, tex_width, tex_width, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
#endif
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_texture, 0);

			GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL error after glFramebufferTexture2D: 0x{:x}", err);
			}

			//glDrawBuffers(0, nullptr);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("Shadow FBO is incomplete! Status: 0x{:x}", status);
				return;
			}
			PN_CORE_INFO("Shadow FBO is complete");
		}

		void _cleanup() {
			if (shadow_fbo) {
				glDeleteFramebuffers(1, &shadow_fbo);
			}

			if (shadow_texture) {
				glDeleteTextures(1, &shadow_texture);
			}
		}
	public:
		Light() {
			num_shadowmapped_lights = 0;
		}
		~Light() {
			_cleanup();
		}

		glm::vec3 position{};
		glm::vec3 L_intensity = glm::vec3(0.1f);
		TYPES type = TYPES::POINT;

		// dont touch these values unless you know what youre doing
		float aspect_ratio = 1.f / 1.f;
#ifdef PN_PLATFORM_WINDOWS
		float near_plane{ 0.1f };		// closest distance light can see
#else
		float near_plane{ 1.f };
#endif
		float far_plane{ 30.f };		// furthest distance light can see(for shadows)

		// not required for point lights
		glm::vec3 forward{0 , -1, 0};	// looking down by default
		float fov{ 120.f };				// dont set larger values
		float shadow_source_follow_distance{ far_plane * 0.75f };	// how far away should shadow map frustum origin be placed from camera

		glm::mat4 view() const {
			glm::vec3 up_vec = glm::vec3(0.f, 1.f, 0.f);

			// forward is parallel to up, use a different up vector
			if (glm::abs(glm::dot(glm::normalize(forward), up_vec)) > 0.99f) {
				up_vec = glm::vec3(0.f, 0.f, 1.f);  // Use Z-axis as up instead
			}

			return glm::lookAt(
				position,
				position + glm::normalize(forward),
				up_vec
			);
		}

		glm::mat4 projection() const {
			if (type == TYPES::DIRECTIONAL) {
				float ortho_size = shadow_source_follow_distance;  // based on scene size. lower values = sharper shadows
				return glm::ortho(
					-ortho_size, ortho_size,   // left, right
					-ortho_size, ortho_size,   // bottom, top
					near_plane, far_plane      // near, far
				);
			}
			return glm::perspective(
				glm::radians(fov),
				aspect_ratio,
				near_plane,
				far_plane
			);
		}

		SHADOW_TYPES getShadowType() {
			return shadow_type;
		}

		SHADOW_TYPES getShadowType() const {
			return shadow_type;
		}

		bool setShadowType(SHADOW_TYPES type) {
			if (type == shadow_type) {
				return false;
			}

			if (type == SHADOW_TYPES::MAPPED && num_shadowmapped_lights >= MAX_SHADOWMAPPED_LIGHTS) {
				return false;
			}

			if (type == SHADOW_TYPES::MAPPED && num_shadowmapped_lights < MAX_SHADOWMAPPED_LIGHTS) {
				++num_shadowmapped_lights;
			}

			if (type != SHADOW_TYPES::MAPPED && shadow_type == SHADOW_TYPES::MAPPED) {
				--num_shadowmapped_lights;
			}

			if (type == SHADOW_TYPES::MAPPED && shadow_fbo == 0) {
				_createShadowMapBuffers();
			}

			if (type != SHADOW_TYPES::MAPPED && (shadow_fbo || shadow_texture)) {
				_cleanup();
			}

			shadow_type = type;

			return true;
		}

		unsigned int getShadowFbo() const {
			return shadow_fbo;
		}

		unsigned int getShadowTexture() const {
			return shadow_texture;
		}
	};

	class LightSources {
	private:
		LightSources() = default;
		~LightSources() = default;

		std::unordered_map<std::string, Light> sources;

	public:
		bool lightsOn = true;		// global switch to toggle lights

		static constexpr int MAX_LIGHT_SOURCES = 16;		// remember to set in fragment shader if this is changed
		glm::vec3 AMBIENT_LIGHT = GraphicsSettings::get().AMBIENT_LIGHT;

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
				PN_CORE_ERROR("Light reference {0} already mesh_id", ref);
				return false;
			}
			if (sources.size() >= MAX_LIGHT_SOURCES) {
				PN_CORE_ERROR("Too many light sources!");
				return false;
			}
			sources[ref] = Light();
			return true;
		}

		void destroy(const std::string& ref) {
			if (sources.find(ref) != sources.end()) {
				sources.erase(ref);
			}
		}

		void destroyAll() {
			for (auto it = sources.begin(); it != sources.end(); ) {
				if (it->first != "cam" && it->first != "world") {
					it = sources.erase(it); // erase returns the next iterator
				}
				else {
					++it;
				}
			}
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

			//PN_CORE_INFO("Z");

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

		std::vector<std::pair<const std::string&, std::reference_wrapper<Light>>> getAllWithKeys() {
			std::vector<std::pair<const std::string&, std::reference_wrapper<Light>>> out;
			out.reserve(sources.size());

			for (auto& [k, v] : sources) {
				out.emplace_back(k, std::ref(v));
			}

			return out;
		}

		int getCount() const {
			return sources.size();
		}
	};

	inline int Light::num_shadowmapped_lights = 0;

}

