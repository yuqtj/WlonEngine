
#pragma once

#include "Scene/Component.h"
#include "Framework/GlmCommon.h"
#include <glm/gtx/quaternion.hpp>

class Transform : public Component
{
public:
	void SetParent(Transform* transform);

	inline void SetTranslation(const glm::vec3& translation) { this->translation = translation; }

	inline void SetRotation(const glm::quat& rotation) { }

	inline void SetScale(const glm::vec3& scale) { this->scale = scale; }

	inline const glm::vec3& GetTranslation() const { return translation; }

	inline const glm::quat& GetRotation() const { ; }

	inline const glm::vec3& GetScale() const { return scale; }

	void SetMatrix(const glm::mat4& matrix);

	inline glm::mat4 GetMatrix() const 
	{
		return glm::translate(glm::mat4(1.0), translation) *
			glm::mat4_cast(rotation) *
			glm::scale(glm::mat4(1.0), scale);
	}

	glm::mat4 GetWorldMatrix();

	void UpdateWorldTransform();

private:
	glm::vec3 translation = glm::vec3(0.0, 0.0, 0.0);

	glm::quat rotation = glm::quat(1.0, 0.0, 0.0, 0.0);

	glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);

	glm::mat4 worldMatrix = glm::mat4(1.0);

	bool updateWorldMatrix = false;

	Transform* parent;
};