
#include "RenderManager.h"

#include "Apps/window/WindowInclude.h"
#include "Render/Vulkan/VkContext.h"

RenderManager RenderManager::s_Instance;

RenderManager& RenderManager::GetInstance()
{
	return s_Instance;
}

void RenderManager::Initialized(BasicWindow* window)
{
	GetInstance().Init(window);
}

void RenderManager::Update()
{
	//m_Device->Update();
}

RenderManager::RenderManager()
{
	m_RenderContext = WL_NEW(VkContext);
}

void RenderManager::Init(BasicWindow* window)
{
	m_RenderContext->Init();
	//m_Device = WL_NEW(GfxDeviceVulkan)(window);
}
