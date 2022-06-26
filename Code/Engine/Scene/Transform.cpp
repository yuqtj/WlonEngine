
#include "Transform.h"

#include <glm/gtx/matrix_decompose.hpp>

void Transform::SetParent(Transform* transform)
{
	parent = transform;
	updateWorldMatrix = true;
}

void Transform::SetMatrix(const glm::mat4& matrix)
{
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, rotation, translation, skew, perspective);
	rotation = glm::conjugate(rotation);
}

glm::mat4 Transform::GetWorldMatrix()
{
	UpdateWorldTransform();

	return worldMatrix;
}

void Transform::UpdateWorldTransform()
{
	if (!updateWorldMatrix)
	{
		return;
	}

	worldMatrix = GetMatrix();
	if (parent)
	{
		worldMatrix = parent->GetWorldMatrix() * worldMatrix;
	}

	updateWorldMatrix = false;
}
