#include "api.h"
#include <map>

#include <VrLib/Log.h>
using vrlib::logger; using vrlib::Log;


Api::Api(const std::string &route, const std::function<void(NetworkEngine*, nlohmann::json &, nlohmann::json &)> &callback)
{
	callbacks()[route] = callback;
}


Api version("version", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	packet["data"]["status"] = "ok";
});



Api play("play", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	engine->tien.start();
	packet["data"]["status"] = "ok";
});


Api pause("pause", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	engine->tien.pause();
	packet["data"]["status"] = "ok";
});

Api get("get", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	bool ok = false;


	glm::mat4 matrix;
	if (data["type"] == "head")
	{
		matrix = engine->vive.hmd.getData();
		ok = true;
	}
	else if (data["type"] == "handleft")
	{
		matrix = engine->vive.controllers[0].transform.getData();
		ok = true;
	}
	else if (data["type"] == "handright")
	{
		matrix = engine->vive.controllers[1].transform.getData();
		ok = true;
	}
	if (ok)
	{
		glm::vec4 position = matrix * glm::vec4(0, 0, 0, 1);
		glm::vec4 forward = glm::normalize((matrix * glm::vec4(0, 0, -1, 1)) - position);
		packet["data"]["status"] = "ok";
		for (int i = 0; i < 3; i++)
			packet["data"]["position"].push_back(position[i]);
		for (int i = 0; i < 3; i++)
			packet["data"]["forward"].push_back(forward[i]);
		return;
	}

	if (data["type"] == "button")
	{
		auto & controller = engine->vive.controllers[0];
		if (data["hand"] == "left")
			controller = engine->vive.controllers[1];

		vrlib::DigitalDevice* button = nullptr;
		if (data["button"] == "trigger")
			button = &controller.triggerButton;
		else if (data["button"] == "thumbpad")
			button = &controller.touchButton;
		else if (data["button"] == "application")
			button = &controller.applicationMenuButton;
		else if (data["button"] == "grip")
			button = &controller.gripButton;

		packet["id"] = "get";
		packet["data"]["status"] = "ok";
		packet["data"]["value"] = button->getData();
		return;
	}
});

Api setcallback("setcallback", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data["type"] == "button")
	{
		auto & controller = engine->vive.controllers[0];
		int hand = 0;
		if (data["hand"] == "left")
			hand = 4;


		if (data["button"] == "trigger")
			engine->callbackMask &= 1 << (hand + 0);
		else if (data["button"] == "thumbpad")
			engine->callbackMask &= 1 << (hand + 1);
		else if (data["button"] == "application")
			engine->callbackMask &= 1 << (hand + 2);
		else if (data["button"] == "grip")
			engine->callbackMask &= 1 << (hand + 3);

		packet["data"]["status"] = "ok";
		return;
	}
});


/*
void sendError(vrlib::Tunnel* tunnel, const std::string &id, const std::string &error)
{
	if (!tunnel)
		return;
	json packet;
	packet["id"] = id;
	packet["data"]["status"] = "error";
	packet["data"]["error"] = error;
	tunnel->send(packet);
}



void sendOk(vrlib::Tunnel* tunnel, const std::string &id)
{
	if (!tunnel)
		return;
	json packet;
	packet["id"] = id;
	packet["data"]["status"] = "ok";
	tunnel->send(packet);
}*/