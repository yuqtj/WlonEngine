
#include "App.h"
#include "Apps/InputSystem.h"
#include "Render/RenderManager.h"
#include "Apps/window/BasicWindow.h"
#include "ModelReader/GltfReader.h"
#include "Scene/Scene.h"


#include "Scene/GameObjectUntil.h"
#include "Scene/Light.h"
#include "Scene/Transform.h"
#include "Scene/Camera.h"

ExitCode App::Initialize()
{
	InputSystem::Initialized();

	m_AppWindow = CreateWlWindow();

	Scene* scene = GltfReader::LoadFile("C:/Wlon/WlonEngine/Code/Resources/Bonza4X.gltf");
	GameObject* rootGo = scene->GetRootNode();
	Transform* rootTransform = rootGo->GetComponent<Transform>();

	{
		GameObject* go = CreateGameObject("Light");
		Light* light = go->AddComponent<Light>();
		Transform* transform = go->AddComponent<Transform>();
		glm::quat rotation = glm::quat({ glm::radians(-30.0f), glm::radians(175.0f), glm::radians(0.0f) });
		transform->SetTranslation(glm::vec3(-50, 0, 0));
		transform->SetRotation(rotation);
		light->SetLightType(LightType::Directional);
		
		transform->SetParent(rootTransform);
		scene->AddNode(go);
	}

	{
		/*GameObject* go = CreateGameObject("Camera");
		Camera* camera = go->AddComponent<Camera>();*/

	}
	// vulkan init
	RenderManager::Initialized(m_AppWindow);

	return ExitCode::Success;
}

ExitCode App::PlayerLoop()
{
	while (!m_AppWindow->ShouldClose())
	{
		RenderManager::GetInstance().Update();

		m_AppWindow->ProcessEvents();
	}

	return ExitCode::Success;
}

void App::Terminate()
{
	WL_DELETE(m_AppWindow);
}
