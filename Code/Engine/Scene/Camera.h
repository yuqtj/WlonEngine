
#pragma once

#include "Scene/Component.h"
#include "Framework/GlmCommon.h"

class Camera : public Component
{
public:
	Camera() {};
	~Camera() {};

	inline void SetAspectRatio(float aspectRatio) { this->aspectRatio = aspectRatio; };

	inline void SetFieldOfView(float fov) { this->fov = fov; }

	inline float GetFarPlane() const { return farPlane; }

	inline void SetFarPlane(float zfar) { farPlane = zfar; }

	inline float GetNearPlane() const { return nearPlane; }

	inline void SetNearPlane(float znear) { nearPlane = znear; }

	inline float GetAspectRatio() { return aspectRatio; }

	inline float GetFieldOfView() { return fov; }

private:
	/**
	 * @brief Screen size aspect ratio
	 */
	float aspectRatio{ 1.0f };

	/**
	 * @brief Horizontal field of view in radians
	 */
	float fov{ glm::radians(60.0f) };

	float farPlane{ 100.0 };

	float nearPlane{ 0.1f };
};
