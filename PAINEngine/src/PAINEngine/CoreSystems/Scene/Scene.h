#pragma once
#include "pch.h"


#include "CoreSystems/Renderer/Mesh.h"
#include "Camera.h"

namespace PAIN {

	struct SceneObject {
		Mesh* mesh;
		glm::mat4 transform;  // model matrix
	};

	class Scene : public AppSystem {
	public:
		Scene() = default;
		~Scene() = default;

		void onDetach() override;
		void onAttach() override;
		void onFixedUpdate(AppTiming timing) override {};
		void onUpdate(AppTiming timing) override;
		void onEvent([[maybe_unused]] Event::Event& e) override;

		std::shared_ptr<Mesh> AddObject(std::shared_ptr<Mesh> mesh, glm::vec3 pos, glm::quat quat, glm::vec3 scale);
		void DeleteObject(int index);

		const std::vector<SceneObject>& GetObjects() const;
		Camera* GetActiveCamera();
		std::vector<Mesh*> m_Meshes;
		//Entity CreateEntity(const std::string name);
		//void DestroyEntity(Entity entity);

	private:

		std::vector<SceneObject> m_Objects;
		std::unique_ptr<Camera> camera;

		//std::vector<Camera*> cameras; // multiple cams impl
		
		
		//std::unique_ptr<ECS::Controller> ecs;
	};
}

