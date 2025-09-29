
#include "Scene.h"
namespace PAIN {


	void Scene::Init() {
		// Example: load two meshes
		m_Objects.push_back(Mesh::LoadObj("ogre.obj"));
		m_Objects.push_back(Mesh::LoadObj());
	}

	void Scene::OnUpdate()
	{

		glm::mat4 transform1 = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0.f));
		glm::mat4 transform2 = glm::translate(glm::mat4(1.f), glm::vec3(2.f, 1.f, 0.f));

		RendererLayer::Submit(m_Objects[0].get(), transform1);
		RendererLayer::Submit(m_Objects[1].get(), transform2);
		//for (auto& mesh : m_Objects) {

		//	RendererLayer::Submit(mesh.get());
		//}
	}
}

