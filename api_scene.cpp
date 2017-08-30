#include "api.h"

#include <fstream>
#include <VrLib/json.hpp>

Api scene_get("scene/get", [](NetworkEngine* engine, json &data, json &packet)
{
	json meshes = json::array();
	packet["data"] = engine->tien.scene.asJson(meshes);
	packet["status"] = "ok";
});


Api scene_save("scene/save", [](NetworkEngine* engine, json &data, json &packet)
{
	json d;
	d["meshes"] = json::array();
	d["tree"] = engine->tien.scene.asJson(d["meshes"]);

	std::ofstream file("engine.json");
	file << d;
	file.close();

	packet["status"] = "ok";
});


Api scene_load("scene/load", [](NetworkEngine* engine, json &data, json &packet)
{
	std::ifstream file("engine.json");
	json fileData = json::parse(file);

	engine->tien.scene.fromJson(fileData["tree"], fileData);
	engine->tien.scene.cameraNode = nullptr;

	packet["status"] = "ok";
});


Api scene_raycast("scene/raycast", [](NetworkEngine* engine, json &data, json &packet)
{
	packet["error"] = "not implemented";
});


Api scene_reset("scene/reset", [](NetworkEngine* engine, json &data, json &packet)
{
	engine->reset();

	std::string nodes[] = { "Camera", "Sunlight", "LeftHand", "RightHand", "Head", "GroundPlane" };
	for (int i = 0; i < sizeof(nodes) / sizeof(std::string); i++)
	{
		if (data.find(nodes[i]) != data.end())
			engine->tien.scene.findNodesWithName(nodes[i])[0]->guid = data[nodes[i]].get<std::string>();
		data[nodes[i]] = engine->tien.scene.findNodesWithName(nodes[i])[0]->guid;
	}


	packet["status"] = "ok";
});
