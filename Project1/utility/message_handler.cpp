#include "utility/message_handler.h"

void MessageHandler::RemoveListener(ListenerID ID)
{
	m_listeners.erase(ID);
}

void MessageHandler::AddMessage(Message msg)
{
	m_messages.emplace(msg);
}

void MessageHandler::AddListener(ListenerID ID, std::function<void(Message)> listener)
{

	m_listeners.insert(std::make_pair(ID, listener));
}

void MessageHandler::Notify()
{
	while (!m_messages.empty())
	{
		for (auto& listener : m_listeners)
		{
			if (m_messages.front().msgID == listener.first) {

				(listener.second)(m_messages.front());
			}
		}
		m_messages.pop();		// meesage sent so remove from the list
	}
	
}