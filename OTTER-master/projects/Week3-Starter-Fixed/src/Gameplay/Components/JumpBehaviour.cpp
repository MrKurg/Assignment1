#include "Gameplay/Components/JumpBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"

void JumpBehaviour::Awake()
{
	_body = GetComponent<Gameplay::Physics::RigidBody>();
	if (_body == nullptr) {
		IsEnabled = false;
	}
}

void JumpBehaviour::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat, "Impulse", &_impulse, 1.0f);
}

nlohmann::json JumpBehaviour::ToJson() const {
	return {
		{ "impulse", _impulse }
	};
}

JumpBehaviour::JumpBehaviour() :
	IComponent(),
	_impulse(10.0f)
{ }

JumpBehaviour::~JumpBehaviour() = default;

JumpBehaviour::Sptr JumpBehaviour::FromJson(const nlohmann::json& blob) {
	JumpBehaviour::Sptr result = std::make_shared<JumpBehaviour>();
	result->_impulse = blob["impulse"];
	return result;
}

void JumpBehaviour::Update(float deltaTime) {
	if (InputEngine::GetKeyState(GLFW_KEY_SPACE) == ButtonState::Pressed) {
		_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, _impulse));
		Gameplay::IComponent::Sptr ptr = Panel.lock();
		if (ptr != nullptr) {
			ptr->IsEnabled = !ptr->IsEnabled;
		}
	}

	_body->SetAngularFactor(glm::vec3(0.0f, 0.0f, 0.0f));

	if (InputEngine::GetKeyState(GLFW_KEY_D) == ButtonState::Pressed)
	{
		_body->SetLinearVelocity(glm::vec3(9.0f, 0.0f, 0.0f));
	}
	else if (InputEngine::GetKeyState(GLFW_KEY_A) == ButtonState::Pressed)
	{
		_body->SetLinearVelocity(glm::vec3(-9.0f, 0.0f, 0.0f));
	}
	else if (InputEngine::GetKeyState(GLFW_KEY_W) == ButtonState::Pressed)
	{
		_body->SetLinearVelocity(glm::vec3(0.0f, 9.0f, 0.0f));
	}
	else if (InputEngine::GetKeyState(GLFW_KEY_S) == ButtonState::Pressed)
	{
		_body->SetLinearVelocity(glm::vec3(0.0f, -9.0f, 0.0f));
	}
}

