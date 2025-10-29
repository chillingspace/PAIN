#include "sRenderer.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Renderer/Windows/WindowsRenderer.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Scene/Scene.h"

#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/Material.h"

#include "ECS/Controller.h"

//For imgui viewport
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "LayeredSystems/LevelEditor/Editor.h"

#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Audio/Audio.h"

#include "CoreSystems/Renderer/skybox.h"

namespace PAIN {
	void sRenderer::onDetach()
	{
		w_renderer = nullptr;
	}
	void sRenderer::onAttach() {

		//Create window render
		w_renderer = std::make_unique<WindowsRenderer>();

		w_renderer->Init(services);

		//Init scene
		m_Scene = services->get<Scene>();
		
	}

	void sRenderer::shadowPass()
	{
		// populate shadow map first
		auto ecs = services->get<ECS::Controller>();
		auto scene = services->get<Scene>();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<MetaData::EntityName>();

		glViewport(0, 0, GraphicsSettings::get().getShadowMapWidth(), GraphicsSettings::get().getShadowMapWidth());

		// Im sure there is a better way to render shadows
		for (const Light& l : LightSources::get().getAll()) {

			if (l.getShadowType() != Light::SHADOW_TYPES::MAPPED) continue;

			w_renderer->BeginShadowPass(l);

			for (auto e : view) {

				auto transform = ecs->getEntityComponent<Transform>(e);

				auto mesh = ecs->getEntityComponent<MeshRenderer>(e);

				glm::mat4 model;
				if (transform.has_value())
				{
					model = transform.value().get().getMatrix();
				}

				if (mesh.has_value())
				{
					auto mesh_ptr = scene->getMesh(mesh->get().mesh_id);
					w_renderer->DrawShadows(mesh_ptr.get(), model, l); // uses shadow_shader

				}


			}
			w_renderer->EndShadowPass();
		}
	}

	void sRenderer::geometryPass()
	{
		auto ecs = services->get<ECS::Controller>();
		auto scene = services->get<Scene>();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<MetaData::EntityName>();

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before geometry pass: {}", err);
		}

		w_renderer->BeginGeometryPass(scene);
		for (auto e : view) {

  			auto transform = ecs->getEntityComponent<Transform>(e);
			auto mesh = ecs->getEntityComponent<MeshRenderer>(e);
			glm::mat4 model;
			if (transform.has_value())
			{
				model = transform.value().get().getMatrix();
			}
			if (mesh.has_value())
			{
				auto mesh_ptr = scene->getMesh(mesh->get().mesh_id);
				w_renderer->DrawGeometry(m_Scene, mesh_ptr.get(), model);
			}

		}
		w_renderer->EndGeometryPass();

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after geometry pass: {}", err);
		}
	}

	void sRenderer::reflectionPass()
	{
		auto ecs = services->get<ECS::Controller>();
		auto scene = services->get<Scene>();

		// Use EnTT view to iterate all entities with EntityName component
		auto& registry = ecs->getRegistry();
		auto view = registry.view<MetaData::EntityName>();

		for (auto e : view) {

			auto transform = ecs->getEntityComponent<Transform>(e);
			auto mesh = ecs->getEntityComponent<MeshRenderer>(e);
			glm::mat4 model;
			if (transform.has_value())
			{
				model = transform.value().get().getMatrix();
			}
			if (mesh.has_value())
			{
				auto mesh_ptr = scene->getMesh(mesh->get().mesh_id);
				w_renderer->ReflectionPass(mesh_ptr);
			}

		}
	}

	void sRenderer::lightingPass()
	{
		auto scene = services->get<Scene>();
		w_renderer->LightingPass(scene, LightSources::get());

		//Skybox::get().render(scene->GetActiveCamera()->view(), scene->GetActiveCamera()->projection());
	}

	void sRenderer::debugPass(bool show_debug)
	{
		if (!show_debug) { return; }

		auto ecs = services->get<ECS::Controller>();
		auto scene = services->get<Scene>();
		auto& reg = ecs->getRegistry();
		auto view = reg.view<MetaData::EntityName>();

		for (auto e : view) {
			auto trans = ecs->getEntityComponent<Transform>(e);
			auto mesh = ecs->getEntityComponent<MeshRenderer>(e);
			if (!trans.has_value() || !mesh.has_value()) continue;

			glm::mat4 mat = trans->get().getMatrix();
			auto mesh_ptr = scene->getMesh(mesh->get().mesh_id);

			glm::vec3 min_aabb = mesh_ptr->getAABBMin(), max_aabb = mesh_ptr->getAABBMax();

			// Transform 8 corners to world space
			glm::vec3 corners[8] = {
			  {min_aabb.x,min_aabb.y,min_aabb.z},{max_aabb.x,min_aabb.y,min_aabb.z},
			  {max_aabb.x,max_aabb.y,min_aabb.z},{min_aabb.x,max_aabb.y,min_aabb.z},
			  {min_aabb.x,min_aabb.y,max_aabb.z},{max_aabb.x,min_aabb.y,max_aabb.z},
			  {max_aabb.x,max_aabb.y,max_aabb.z},{min_aabb.x,max_aabb.y,max_aabb.z}
			};

			glm::vec3 w_min(FLT_MAX), w_max(-FLT_MAX);

#ifdef PN_PLATFORM_WINDOWS
            for (auto& c : corners) {
				glm::vec4 w = mat * glm::vec4(c, 1.0f);
				w_min.x = min(w_min.x, w.x); w_min.y = min(w_min.y, w.y); w_min.z = min(w_min.z, w.z);
				w_max.x = max(w_max.x, w.x); w_max.y = max(w_max.y, w.y); w_max.z = max(w_max.z, w.z);
			}
#else
            for (auto& c : corners) {
                glm::vec4 w = mat * glm::vec4(c, 1.0f);
                w_min.x = fmin(w_min.x, w.x); w_min.y = fmin(w_min.y, w.y); w_min.z = fmin(w_min.z, w.z);
                w_max.x = fmax(w_max.x, w.x); w_max.y = fmax(w_max.y, w.y); w_max.z = fmax(w_max.z, w.z);
            }
#endif

			w_renderer->DebugPass(w_min, w_max, { 1,1,0,1 }, scene);
		}
	}


	void sRenderer::postProcessPass()
	{
		w_renderer->PostProcessPass();
	}

	void sRenderer::onUpdate(AppTiming timing) {

		{
#ifdef _DEBUG
			auto editor = services->get<Editor::Editor>();
			bool editor_visible = editor && editor->isVisible();
			bool editor_debug = editor && editor->isDebugMode();

#else
			bool editor_visible = false;
			bool editor_debug = false;
#endif

			GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err on update loop begin: {}", err);
			}

			if (editor_visible) {
				glBindFramebuffer(GL_FRAMEBUFFER, w_renderer->getFinalFbo());
				//glViewport(0, 0, fbWidth, fbHeight);
			}
			else {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// Match viewport to window size
				auto window = services->get<Window::Window>();
#ifdef PN_PLATFORM_WINDOWS
				glfwGetFramebufferSize((GLFWwindow*)window->getNativeWindow(), &winWidth, &winHeight);
				glViewport(0, 0, winWidth, winHeight);
#else
				ANativeWindow* nativeWindow = (ANativeWindow*)window->getNativeWindow();
				winWidth = ANativeWindow_getWidth(nativeWindow);
				winHeight = ANativeWindow_getHeight(nativeWindow);
				glViewport(0, 0, winWidth, winHeight);
#endif

			}

			// Clear
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err before render passes: {}", err);
			}

			// Render all passes
			shadowPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after shadow pass: {}", err);
			}
			geometryPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after geometry pass: {}", err);
			}
			reflectionPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after reflection pass: {}", err);
			}
			lightingPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after lighting pass: {}", err);
			}
		
			debugPass(editor_debug);
			postProcessPass();
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after post process pass: {}", err);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset
		}

		// set cam light to cam
		auto olcam = LightSources::get().get("cam");
		Light& lcam = olcam.value();
		lcam.position = m_Scene->GetActiveCamera()->pos;
		lcam.position.y += 0.1f;	// light on camera = grainy
		lcam.fov = m_Scene->GetActiveCamera()->fov;
		lcam.forward = m_Scene->GetActiveCamera()->forward;
		lcam.aspect_ratio = m_Scene->GetActiveCamera()->aspect_ratio;
	}


	void sRenderer::onEvent(Event::Event& e) {}
}