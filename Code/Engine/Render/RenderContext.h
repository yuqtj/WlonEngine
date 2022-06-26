#pragma once

class RenderContext
{
public:
	RenderContext() {};

	virtual void Init() = 0;
	void Begin();
protected:
private:
};