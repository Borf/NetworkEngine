#include "api.h"

#include <fstream>
#include <VrLib/json.hpp>

Api scene_get("scene/get", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{
	json meshes = json::array();

	json ret;
	ret["id"] = "scene/get";
	ret["data"] = engine->tien.scene.asJson(meshes);
	tunnel->send(ret);
});


Api scene_save("scene/save", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{
	json d;
	d["meshes"] = json::array();
	d["tree"] = engine->tien.scene.asJson(d["meshes"]);

	std::ofstream file("engine.json");
	file << d;
	file.close();


	json packet;
	packet["id"] = "scene/save";
	packet["status"] = "error";
	packet["error"] = "not implemented";
	if(tunnel)
		tunnel->send(packet);
});


Api scene_load("scene/load", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{
	std::ifstream file("engine.json");
	json fileData = json::parse(file);

	engine->tien.scene.fromJson(fileData["tree"], fileData);
	engine->tien.scene.cameraNode = nullptr;

	json packet;
	packet["id"] = "scene/load";
	packet["status"] = "error";
	packet["error"] = "not implemented";
	if(tunnel)
		tunnel->send(packet);
});


Api scene_raycast("scene/raycast", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{
	json packet;
	packet["id"] = "scene/raycast";
	packet["status"] = "error";
	packet["error"] = "not implemented";
	tunnel->send(packet);
});


Api scene_reset("scene/reset", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{
	engine->reset();

	std::string nodes[] = { "Camera", "Sunlight", "LeftHand", "RightHand", "Head", "GroundPlane" };
	for (int i = 0; i < sizeof(nodes) / sizeof(std::string); i++)
	{
		if (data.find(nodes[i]) != data.end())
			engine->tien.scene.findNodesWithName(nodes[i])[0]->guid = data[nodes[i]].get<std::string>();
		data[nodes[i]] = engine->tien.scene.findNodesWithName(nodes[i])[0]->guid;

	}


	if (tunnel)
	{
		json packet;
		packet["id"] = "scene/reset";
		packet["status"] = "ok";
		tunnel->send(packet);
	}
});
