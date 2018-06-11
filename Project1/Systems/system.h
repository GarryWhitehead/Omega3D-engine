#pragma once
#include <vector>
#include <assert.h>
#include "utility/message_handler.h"

class System
{
public:

	System(MessageHandler *msg);

	virtual ~System();

	virtual void Update() = 0;
	virtual void Destroy() = 0;
	virtual void OnNotify(Message& msg) = 0;

protected:

	MessageHandler *p_message;

	// message handling functions
	std::function<void(Message)> System::NotifyResponse();
	void System::SendMessage(Message msg);
};

