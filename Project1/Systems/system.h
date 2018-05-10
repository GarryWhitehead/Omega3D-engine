#pragma once
#include <vector>
#include <assert.h>

class ComponentManager;
enum class ComponentManagerId;

class System
{
public:

	System();

	virtual ~System();

	virtual void Update() = 0;
	virtual void Destroy() = 0;

protected:

};

