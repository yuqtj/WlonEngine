
#include <vector>

#include "Scene/Component.h"
#include "Framework/GlmCommon.h"

enum class LightType
{
	Directional = 0,
	Point       = 1,
	Spot        = 2,
	Max
};

struct LightProperties
{
	glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
	glm::vec3 color{ 1.0f, 1.0f, 1.0f };
	float intensity{ 1.0f };
	float range{ 0.0f };
	float innerConeAngle{ 0.0f };
	float outerConeAngle{ 0.0f };
};

class Light : public Component
{
public:
	Light() {};
	~Light() {};

	void SetLightType(const LightType& type);
	void SetLightProperties(const LightProperties& properties);

private:
	LightType m_Type;
	LightProperties m_Properties;
};
