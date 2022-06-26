
#include "GameObjectUntil.h"

GameObject* CreateGameObject(const std::string& name)
{
	return WL_NEW(GameObject)(name);
}

