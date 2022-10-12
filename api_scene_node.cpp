#include "api.h"

#include "PanelComponent.h"
#include "WaterComponent.h"

#include <VrLib/tien/components/Transform.h>
#include <VrLib/tien/components/ModelRenderer.h>
#include <VrLib/tien/components/AnimatedModelRenderer.h>
#include <VrLib/tien/components/TerrainRenderer.h>

#include <VrLib/Log.h>
using vrlib::logger;

Api scene_node_add("scene/node/add", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	vrlib::tien::Node* parent = &engine->tien.scene;
	if (data.find("parent") != data.end())
		parent = parent->findNodeWithGuid(data["parent"]);
	if (!parent)
	{
		packet["error"] = "Parent not found";
		return;
	}
	if (data.find("name") == data.end())
	{
		packet["error"] = "Name not specified";
		return;
	}


	vrlib::tien::Node* n = new vrlib::tien::Node(data["name"], parent);
	if (data.find("id") != data.end())
		n->guid = data["id"];
	n->addComponent(new vrlib::tien::components::Transform());
	if (data.find("components") != data.end())
	{
		if (data["components"].find("transform") != data["components"].end())
		{
			if (!data["components"]["transform"]["scale"].is_number_float() && !data["components"]["transform"]["scale"].is_number_integer())
			{
				packet["error"] = "transform/scale should be a float";
				return;
			}

			if (data["components"]["transform"].find("position") != data["components"]["transform"].end())
				n->transform->position = glm::vec3(data["components"]["transform"]["position"][0], data["components"]["transform"]["position"][1], data["components"]["transform"]["position"][2]);
			if (data["components"]["transform"].find("rotation") != data["components"]["transform"].end())
				n->transform->rotation = glm::quat(glm::vec3(glm::radians(data["components"]["transform"]["rotation"][0].get<float>()), glm::radians(data["components"]["transform"]["rotation"][1].get<float>()), glm::radians(data["components"]["transform"]["rotation"][2].get<float>())));
			if (data["components"]["transform"].find("scale") != data["components"]["transform"].end())
				n->transform->scale = glm::vec3(data["components"]["transform"]["scale"], data["components"]["transform"]["scale"], data["components"]["transform"]["scale"]);
		}
		if (data["components"].find("model") != data["components"].end())
		{
			if (data["components"]["model"].find("animated") != data["components"]["model"].end() && data["components"]["model"]["animated"].get<bool>())
			{
				auto renderer = new vrlib::tien::components::AnimatedModelRenderer(data["components"]["model"]["file"]);

				if (data["components"]["model"].find("animation") != data["components"]["model"].end())
					renderer->playAnimation(data["components"]["model"]["animation"], true);

				n->addComponent(renderer);
			}
			else
			{
				if (data["components"]["model"].find("file") == data["components"]["model"].end())
				{
					packet["error"] = "no file field found in model";
					return;
				}
				auto renderer = new vrlib::tien::components::ModelRenderer(data["components"]["model"]["file"]);
				if (data["components"]["model"].find("cullbackfaces") != data["components"]["model"].end())
					renderer->cullBackFaces = data["components"]["model"]["cullbackfaces"];
				n->addComponent(renderer);
			}
		}
		if (data["components"].find("terrain") != data["components"].end())
		{
			if (!engine->terrain)
			{
				delete n;
				packet["error"] = "No terrain added";
				return;
			}
			auto renderer = new vrlib::tien::components::TerrainRenderer(engine->terrain);
			if (data["components"]["terrain"].find("smoothnormals") != data["components"]["terrain"].end())
				renderer->smoothNormals = data["components"]["terrain"]["smoothnormals"];
			n->addComponent(renderer);
		}
		if (data["components"].find("water") != data["components"].end())
		{
			glm::vec2 size = glm::vec2(data["components"]["water"]["size"][0], data["components"]["water"]["size"][1]);
			float resolution = 0.1f;
			if (data["components"]["water"].find("resolution") != data["components"]["water"].end())
				resolution = data["components"]["water"]["resolution"];
			n->addComponent(new WaterComponent(size, resolution));
		}
		if (data["components"].find("panel") != data["components"].end())
		{
			auto panel = new PanelComponent(glm::vec2(data["components"]["panel"]["size"][0], data["components"]["panel"]["size"][1]), 
											glm::ivec2(data["components"]["panel"]["resolution"][0], data["components"]["panel"]["resolution"][0]));

			if (data["components"]["panel"].find("background") != data["components"]["panel"].end())
			{
				panel->clearColor = glm::vec4(	data["components"]["panel"]["background"][0], 
												data["components"]["panel"]["background"][1], 
												data["components"]["panel"]["background"][2], 
												data["components"]["panel"]["background"][3]);
				panel->clear();
			}
			n->addComponent(panel);
			n->addComponent(new vrlib::tien::components::MeshRenderer(panel));
			if (data["components"]["panel"].find("castshadow") != data["components"]["panel"].end())
				n->getComponent<vrlib::tien::components::MeshRenderer>()->castShadow = data["components"]["panel"]["castshadow"];
			n->getComponent<vrlib::tien::components::MeshRenderer>()->useDeferred = panel->clearColor.a > 0.99;
		}
	}

	data["id"] = n->guid;

	packet["data"]["uuid"] = n->guid;
	packet["data"]["name"] = n->name;
	packet["status"] = "ok";
});



Api scene_node_moveto("scene/node/moveto", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end()|| !data["id"].is_string())
	{
		packet["error"] = "ID field not set";
		return;
	}

	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (node)
	{
		if (data.find("stop") != data.end())
		{
			for (int i = 0; i < (int)engine->movers.size(); i++)
				if (engine->movers[i].node == node)
				{
					engine->movers.erase(engine->movers.begin() + i);
					i--;
				}
		}
		else
		{
			for(size_t i = 0; i < engine->movers.size(); i++)
				if (engine->movers[i].node == node)
				{
					engine->movers.erase(engine->movers.begin() + i);
					break;
				}

			Mover m;
			m.node = node;
			m.position = glm::vec3(data["position"][0], data["position"][1], data["position"][2]);
			m.speed = data.find("speed") != data.end() ? data["speed"] : -1;
			m.time = data.find("time") != data.end() ? data["time"] : -1;
			m.interpolate = Mover::Interpolate::Linear;
			if (data.find("interpolate") != data.end() && data["interpolate"] == "exponential")
				m.interpolate = Mover::Interpolate::Exponential;
			m.followHeight = false;
			if(data.find("followheight") != data.end())
				m.followHeight = data["followheight"];
			m.rotate = Mover::Rotate::NONE;
			if (data.find("rotate") != data.end())
			{
				if (data["rotate"] == "XZ")
					m.rotate = Mover::Rotate::XZ;
				else if (data["rotate"] == "XYZ")
					m.rotate = Mover::Rotate::XYZ;
			}
			engine->movers.push_back(m);
		}
		packet["status"] = "ok";
	}
	else
	{
		packet["status"] = "error";
		packet["error"] = "node not found";
	}
});


Api scene_node_update("scene/node/update", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end())
	{
		packet["error"] = "id not specified";
		return;
	}

	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (node)
	{
		packet["status"] = "ok";
		if (data.find("parent") != data.end())
		{
			vrlib::tien::Node* newParent = engine->tien.scene.findNodeWithGuid(data["parent"]);
			if (newParent)
				node->setParent(newParent);
			else
				logger << "Could not find new parent " << data["parent"] << " in scene/node/update, for node " << data["id"] << " with name " << node->name << Log::newline;

		}
		if (data.find("transform") != data.end())
		{
			if(data["transform"].find("position") != data["transform"].end())
				node->transform->position = glm::vec3(data["transform"]["position"][0], data["transform"]["position"][1], data["transform"]["position"][2]);
			if (data["transform"].find("rotation") != data["transform"].end())
				node->transform->rotation = glm::quat(glm::vec3(glm::radians(data["transform"]["rotation"][0].get<float>()), glm::radians(data["transform"]["rotation"][1].get<float>()), glm::radians(data["transform"]["rotation"][2].get<float>())));
			if (data["transform"].find("scale") != data["transform"].end())
				node->transform->scale = glm::vec3(data["transform"]["scale"], data["transform"]["scale"], data["transform"]["scale"]);
		}
		if (data.find("animation") != data.end())
		{
			if (data["animation"].find("name") != data["animation"].end())
				node->getComponent<vrlib::tien::components::AnimatedModelRenderer>()->playAnimation(data["animation"]["name"]);
			if (data["animation"].find("speed") != data["animation"].end())
				node->getComponent<vrlib::tien::components::AnimatedModelRenderer>()->animationSpeed = data["animation"]["speed"];
		}
	}
	else
	{
		packet["error"] = "node not found";
	}
});



Api scene_terrain_addlayer("scene/node/addlayer", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end() || !data["id"].is_string())
	{
		packet["error"] = "No ID field";
		return;
	}

	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (node)
	{
		auto renderer = node->getComponent<vrlib::tien::components::TerrainRenderer>();
		if (!renderer)
		{
			packet["error"] = "node has no terrain renderer";
		}
		else
		{
			packet["status"] = "ok";
			renderer->addMaterialLayer(data["diffuse"], data["normal"], data["minHeight"], data["maxHeight"], data["fadeDist"]);
		}
	}
	else
	{
		packet["error"] = "node not found";
	}
});




Api scene_terrain_dellayer("scene/node/dellayer", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	packet["error"] = "not implemented";
});



Api scene_node_delete("scene/node/delete", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end() || !data["id"].is_string())
	{
		packet["error"] = "id not specified";
		return;
	}
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (node)
	{
		delete node;
		packet["status"] = "ok";
	}
	else
	{
		packet["error"] = "node not found";
	}
});


Api scene_node_find("scene/node/find", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("name") != data.end())
	{
		nlohmann::json meshes = nlohmann::json::array();
		std::vector<vrlib::tien::Node*> nodes = engine->tien.scene.findNodesWithName(data["name"]);
		for (auto n : nodes)
			packet["data"].push_back(n->asJson(meshes));
	}
	packet["status"] = "ok";
});


