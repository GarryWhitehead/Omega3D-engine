
#include "Omega_Global.h"

#include "Managers/EventManager.h"

#include <assert.h>

namespace OmegaEngine
{
namespace Global
{

struct Managers
{
	EventManager *eventManager = nullptr;
};

static Managers managers;

EventManager *eventManager()
{
	assert(managers.eventManager != nullptr);
	return managers.eventManager;
}

void initEventManager()
{
	managers.eventManager = new EventManager();
	assert(managers.eventManager != nullptr);
}

void init()
{
	initEventManager();
}
} // namespace Global
} // namespace OmegaEngine