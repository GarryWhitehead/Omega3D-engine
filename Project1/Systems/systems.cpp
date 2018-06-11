#include "Systems/system.h"
#include <assert.h>

System::System(MessageHandler *msg) :
	p_message(msg)
{
}

System::~System() 
{
}

// message handling functions
std::function<void(Message)> System::NotifyResponse()
{
	auto msgListener = [&](Message msg) -> void
	{
		this->OnNotify(msg);
	};
	return msgListener;
}

void System::SendMessage(Message msg)
{
	p_message->AddMessage(msg);
}


