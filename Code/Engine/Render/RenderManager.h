#pragma once

class BasicWindow;
class RenderContext;

class RenderManager
{
public:
	static RenderManager& GetInstance();
	static void Initialized(BasicWindow* property);
	void Update();

protected:
private:
	RenderManager();;
	void Init(BasicWindow* property);

	RenderContext* m_RenderContext;
	static RenderManager s_Instance;
};