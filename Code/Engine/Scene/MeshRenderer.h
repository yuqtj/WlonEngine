
#pragma once


#include "Scene/Component.h"

class Mesh;

class MeshRenderer : public Component
{
public:
	MeshRenderer() { mesh = nullptr; }
	~MeshRenderer () {};

	inline void SetMesh(Mesh* mesh) { this->mesh = mesh; }

private:
	Mesh* mesh;
};
