#pragma once
#include <unordered_map>
#include <queue>
#include <functional>

enum class ListenerID
{
	VULKAN_MSG,
	MESH_MSG,
	INPUT_MSG,
	CAMERA_MSG,
	PHYSICS_MSG,
	GRAPHICS_MSG,
	TRANSFORM_MSG,
};

struct Message
{
	Message(std::string msg, ListenerID id, uint32_t objID) :
		message(msg),
		msgID(id),
		objectID(objID)
	{}

	Message(std::string msg, ListenerID id) :
		Message(msg, id, -1)
	{}

	~Message() {}

	std::string message;
	ListenerID msgID;
	int32_t objectID;
};

class MessageHandler
{
public:

	MessageHandler() {}

	void AddListener(ListenerID ID, std::function<void(Message)> listener);
	void RemoveListener(ListenerID ID);
	void Notify();
	void AddMessage(Message msg);

	friend class MessageComponent;

private:

	std::unordered_map<ListenerID, std::function<void(Message)> > m_listeners;
	std::queue<Message> m_messages;
};

