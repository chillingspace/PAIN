#pragma once
#include "pch.h"


#include "../CoreSystems/Renderer/Mesh.h"


namespace PAIN {

	struct SceneObject {
		Mesh* mesh;
		glm::mat4 transform;  // model matrix
	};

	class Scene {
	public:
		Scene() = default;
		~Scene() = default;

		void Init();
		void OnUpdate();

		Mesh* AddObject(std::unique_ptr<Mesh> mesh, glm::mat4 transform);
		const std::vector<SceneObject>& GetObjects() const;

		//Entity CreateEntity(const std::string name);
		//void DestroyEntity(Entity entity);

	private:
		std::vector<std::unique_ptr<Mesh>> m_Meshes;
		std::vector<SceneObject> m_Objects;
		//std::unique_ptr<ECS::Controller> ecs;
	};
}

