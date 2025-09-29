#include "Scene.h"

namespace PAIN {


	void Scene::Init() {
		// Example: load two meshes
		m_Objects.push_back(Mesh::LoadObj("ogre.obj"));
		m_Objects.push_back(Mesh::LoadObj());
	}

	void Scene::OnUpdate()
	{

		for (auto& mesh : m_Objects) {
			RendererLayer::Submit(mesh.get());
		}
	}
}