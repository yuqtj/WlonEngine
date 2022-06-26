
#pragma once

#include <string>
#include <vector>
#include <type_traits>

#include "Apps/BaseInclude.h"

class Component;

class GameObject
{
public:
	GameObject(const std::string& name);
	//~GameObject();
	virtual ~GameObject() = default;

	template<typename T>
	T* AddComponent()
	{
		T* t = WL_NEW(T);
		m_Components.push_back(t);
		return t;
	}

	template<typename T>
	T* GetComponent()
	{
		auto it = m_Components.begin();
		for (; it != m_Components.end(); ++it)
		{
			Component* c = *it;
			if constexpr (std::is_same<Component, T>::value)
			{
				ASSERT(false);
				return nullptr;
			}
			else
			{
				T* actualType = dynamic_cast<T*>(c);
				if (actualType)
				{
					return actualType;
				}
			}
		}

		return nullptr;
	}

private:
	std::vector<Component*> m_Components;
};
