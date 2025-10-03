
#include "Scene.h"

namespace PAIN {
	void Scene::onDetach() {}

	void Scene::onAttach()
	{
		glm::vec3 pos{ 0.f, 2.f, 4.f };
		glm::vec3 forward{ -glm::normalize(pos) };
		glm::vec3 up{ 0.f, 1.f, 0.f };

		float fov{ 90.f };
		float near_plane{ 0.1f };		// closest distance camera can see
		float far_plane{ 100.f };		// furthest distance camera can see

		float width_ratio{ 16.f };
		float height_ratio{ 9.f };

		camera = std::make_unique<Camera>(pos, forward, up, fov, near_plane, far_plane, width_ratio, height_ratio);
	}
	void Scene::onUpdate(AppTiming timing) {}
	void Scene::onEvent(Event::Event& e) {}

	Mesh* Scene::AddObject(std::unique_ptr<Mesh> mesh, glm::mat4 transform)
	{
		//auto ecs = services->get<ECS::Controller>();

		Mesh* raw = mesh.get();
		m_Meshes.push_back(std::move(mesh));
		m_Objects.push_back({ raw, transform });
		return raw;
	}
	void Scene::DeleteObject(int index)
	{
		m_Objects.erase(m_Objects.begin() + index);
	}

	const std::vector<SceneObject>& Scene::GetObjects() const
	{
		return m_Objects;
	}
	Camera* Scene::GetActiveCamera()
	{
		return camera.get();
	}
}

