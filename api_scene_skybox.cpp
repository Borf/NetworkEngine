#include "api.h"

#include <VrLib/tien/components/DynamicSkyBox.h>
#include <VrLib/tien/components/StaticSkyBox.h>

Api scene_skybox_settime("scene/skybox/settime", [](NetworkEngine* engine, json &data, json &packet)
{
	auto skybox = engine->tien.scene.cameraNode->getComponent<vrlib::tien::components::DynamicSkyBox>();
	if (skybox) {
		skybox->timeOfDay = data["time"];
		packet["status"] = "ok";
	}
	else
	{
		packet["status"] = "error";
	}
});

Api scene_skybox_update("scene/skybox/update", [](NetworkEngine* engine, json &data, json &packet)
{
	if (data.find("type") == data.end())
	{
		packet["error"] = "No type field";
		return;
	}
	auto cameraNode = engine->tien.scene.cameraNode;
	if (data["type"] == "dynamic")
	{
		if (cameraNode->getComponent<vrlib::tien::components::DynamicSkyBox>())
		{
			packet["error"] = "Skybox is already dynamic";
			return;
		}
		cameraNode->addComponent(new vrlib::tien::components::DynamicSkyBox());
		if (cameraNode->getComponent<vrlib::tien::components::StaticSkyBox>())
		{
			auto box = cameraNode->getComponent<vrlib::tien::components::StaticSkyBox>();
			cameraNode->removeComponent<vrlib::tien::components::StaticSkyBox>();
			delete box;
		}
	}
	if (data["type"] == "static")
	{
		if (!data["files"].is_object() || data["files"].size() != 6)
		{
			packet["error"] = "No files or not enough files";
			return;
		}
		auto skybox = new vrlib::tien::components::StaticSkyBox();
		cameraNode->addComponent(skybox);
		if (cameraNode->getComponent<vrlib::tien::components::DynamicSkyBox>())
		{
			auto box = cameraNode->getComponent<vrlib::tien::components::DynamicSkyBox>();
			cameraNode->removeComponent<vrlib::tien::components::DynamicSkyBox>();
		}
		skybox->initialize();
		skybox->setTexture(0, data["files"]["xpos"]);
		skybox->setTexture(1, data["files"]["xneg"]);
		skybox->setTexture(2, data["files"]["ypos"]);
		skybox->setTexture(3, data["files"]["yneg"]);
		skybox->setTexture(4, data["files"]["zpos"]);
		skybox->setTexture(5, data["files"]["zneg"]);


	}
	packet["status"] = "ok";
});
	
	