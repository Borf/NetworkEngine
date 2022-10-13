#include "api.h"
#include "PanelComponent.h"

#include <glm/gtc/matrix_transform.hpp>

#include <VrLib/tien/components/DynamicSkyBox.h>


Api scene_panel_clear("scene/panel/clear", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end())
	{
		packet["error"] = "id not found";
		return;
	}
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (!node)
	{
		packet["error"] = "node not found";
		return;
	}
	PanelComponent* panel = node->getComponent<PanelComponent>();
	if (!panel)
	{
		packet["error"] = "panel component not found";
		return;
	}
	packet["status"] = "ok";
	panel->clear();
});


Api scene_panel_swap("scene/panel/swap", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end())
	{
		packet["error"] = "id not found";
		return;
	}
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (!node)
	{
		packet["error"] = "node not found";
		return;
	}
	PanelComponent* panel = node->getComponent<PanelComponent>();
	if (!panel)
	{
		packet["error"] = "panel component not found";
		return;
	}
	panel->swap();
	packet["status"] = "ok";
});

Api scene_panel_drawlines("scene/panel/drawlines", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end())
	{
		packet["error"] = "id not found";
		return;
	}
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (!node)
	{
		packet["error"] = "node not found";
		return;
	}
	PanelComponent* panel = node->getComponent<PanelComponent>();
	if (!panel)
	{
		packet["error"] = "panel component not found";
		return;
	}

	std::vector<vrlib::gl::VertexP3C4> verts;
	for (size_t i = 0; i < data["lines"].size(); i++)
	{
		glm::vec4 color(data["lines"][i][4],
						data["lines"][i][5],
						data["lines"][i][6],
						data["lines"][i][7]);

		verts.push_back(vrlib::gl::VertexP3C4(glm::vec3(data["lines"][i][0], data["lines"][i][1], 0), color));
		verts.push_back(vrlib::gl::VertexP3C4(glm::vec3(data["lines"][i][2], data["lines"][i][3], 0), color));
	}
	
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, panel->backFbo->getWidth(), panel->backFbo->getHeight());
	panel->backFbo->bind();

	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	engine->debugShader->use();
	engine->debugShader->setUniform(NetworkEngine::DebugUniform::projectionMatrix, glm::ortho(0.0f, (float)panel->backFbo->getWidth(), (float)panel->backFbo->getHeight(), 0.0f));
	engine->debugShader->setUniform(NetworkEngine::DebugUniform::modelViewMatrix, glm::mat4());

	vrlib::gl::setAttributes<vrlib::gl::VertexP3C4>(&verts[0]);
	glLineWidth(data["width"]);
	glDrawArrays(GL_LINES, 0, verts.size());
	glEnable(GL_DEPTH_TEST);



	panel->backFbo->unbind();
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);


	packet["status"] = "ok";
});


Api scene_panel_drawtext("scene/panel/drawtext", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end())
	{
		packet["error"] = "id not found";
		return;
	}
	if (data.find("position") == data.end())
	{
		packet["error"] = "position field not set";
		return;
	}
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (!node)
	{
		packet["error"] = "node not found";
		return;
	}
	PanelComponent* panel = node->getComponent<PanelComponent>();
	if (!panel)
	{
		packet["error"] = "panel component not found";
		return;
	}
	
	std::string font = "segoeui";
	float size = 32;
	glm::vec4 color = glm::vec4(0, 0, 0, 1);

	if (data.find("font") != data.end())
		font = data["font"];
	if (data.find("size") != data.end())
		size = data["size"];
	if (data.find("color") != data.end())
		color = glm::vec4(data["color"][0], data["color"][1], data["color"][2], data["color"][3]);

	if (!panel->drawText(glm::vec2(data["position"][0], data["position"][1]), font, data["text"], color, size))
		packet["error"] = "Error loading font";
	else
		packet["status"] = "ok";
});


Api scene_panel_setclearcolor("scene/panel/setclearcolor", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end())
	{
		packet["error"] = "id not found";
		return;
	}
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (!node)
	{
		packet["error"] = "node not found";
		return;
	}
	PanelComponent* panel = node->getComponent<PanelComponent>();
	if (!panel)
	{
		packet["error"] = "panel component not found";
		return;
	}

	for (int i = 0; i < 4; i++)
		panel->clearColor[i] = data["color"][i];

	node->getComponent<vrlib::tien::components::MeshRenderer>()->useDeferred = panel->clearColor.a > 0.99;
	packet["status"] = "ok";
});



Api scene_panel_image("scene/panel/image", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("id") == data.end())
	{
		packet["error"] = "id not found";
		return;
	}
	if (data.find("image") == data.end())
	{
		packet["error"] = "image field not found";
		return;
	}
	vrlib::tien::Node* node = engine->tien.scene.findNodeWithGuid(data["id"]);
	if (!node)
	{
		packet["error"] = "node not found";
		return;
	}
	PanelComponent* panel = node->getComponent<PanelComponent>();
	if (!panel)
	{
		packet["error"] = "panel component not found";
		return;
	}

	glm::vec2 position(0, 0);
	glm::vec2 size(panel->backFbo->getWidth(), panel->backFbo->getHeight());

	if (data.find("position") != data.end())
		position = glm::vec2(data["position"][0], data["position"][1]);
	if (data.find("size") != data.end())
		position = glm::vec2(data["size"][0], data["size"][1]);

	if (!panel->drawImage(data["image"], position, size))
		packet["error"] = "Error drawing image";
	else
		packet["status"] = "ok";
});