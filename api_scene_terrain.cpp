#include "api.h"

#include <VrLib/tien/components/TerrainRenderer.h>
#include <VrLib/tien/Terrain.h>

Api scene_terrain_add("scene/terrain/add", [](NetworkEngine* engine, json &data, json &packet)
{
	if (data.find("size") == data.end() || !data["size"].is_array())
	{
		packet["error"] = "Size parameter is needed";
		return;
	}

	if (data["size"][0].get<int>() > 1024 || data["size"][1].get<int>() > 1024)
	{
		packet["error"] = "Size of terrain too big";
		return;
	}
	if (data.find("heights") != data.end() && data["heights"].size() < data["size"][0].get<int>() * data["size"][1].get<int>())
	{
		packet["error"] = "Not enough terrain height data";
		return;
	}



	if (!engine->terrain) //todo: multiple terrains with a seperate guid?
		engine->terrain = new vrlib::tien::Terrain();
	engine->terrain->setSize(data["size"][0], data["size"][1]);
	if (data.find("heights") != data.end())
		for (int y = 0; y < engine->terrain->getHeight(); y++)
			for (int x = 0; x < engine->terrain->getWidth(); x++)
				(*engine->terrain)[x][y] = data["heights"][x + engine->terrain->getWidth() * y].get<float>();
	auto renderer = engine->tien.scene.findNodeWithComponent<vrlib::tien::components::TerrainRenderer>();
	if (renderer)
		renderer->getComponent<vrlib::tien::components::TerrainRenderer>()->rebuildBuffers();
	packet["status"] = "ok";
});


Api scene_terrain_update("scene/terrain/update", [](NetworkEngine* engine, json &data, json &packet)
{
	packet["error"] = "not implemented";
});


Api scene_terrain_delete("scene/terrain/delete", [](NetworkEngine* engine, json &data, json &packet)
{
	auto renderer = engine->tien.scene.findNodeWithComponent<vrlib::tien::components::TerrainRenderer>();
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (renderer)
		packet["error"] = "terrain is still in use";
	else
	{
		delete engine->terrain;
		engine->terrain = nullptr;
		packet["status"] = "ok";
	}
});





Api scene_terrain_getheight("scene/terrain/getheight", [](NetworkEngine* engine, json &data, json &packet)
{
	if (engine->terrain)
	{
		packet["status"] = "ok";
		if(data.find("position") != data.end())
			packet["data"]["height"] = engine->terrain->getPosition(glm::vec2(128 + data["position"][0].get<float>(), 128 + data["position"][1].get<float>())).y;
		else if(data.find("positions") != data.end())
		{
			for(int i = 0; i < data["positions"].size(); i++)
				packet["data"]["heights"].push_back(engine->terrain->getPosition(glm::vec2(128 + data["positions"][i][0].get<float>(), 128 + data["positions"][i][1].get<float>())).y);
		}
	}
	else
		packet["status"] = "error";
});
