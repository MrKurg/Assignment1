#include "DebugWindow.h"
#include "Application/Application.h"
#include "Application/ApplicationLayer.h"
#include "Application/Layers/RenderLayer.h"

DebugWindow::DebugWindow() :
	IEditorWindow()
{
	Name = "Debug";
	SplitDirection = ImGuiDir_::ImGuiDir_None;
	SplitDepth = 0.5f;
	Requirements = EditorWindowRequirements::Menubar;
}

DebugWindow::~DebugWindow() = default;

void DebugWindow::RenderMenuBar() 
{
	Application& app = Application::Get();
	RenderLayer::Sptr renderLayer = app.GetLayer<RenderLayer>();

	BulletDebugMode physicsDrawMode = app.CurrentScene()->GetPhysicsDebugDrawMode();
	if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDrawMode)) {
		app.CurrentScene()->SetPhysicsDebugDrawMode(physicsDrawMode);
	}

	ImGui::Separator();

	RenderFlags flags = renderLayer->GetRenderFlags();
	bool changed = false;
	bool tempCool = *(flags & RenderFlags::EnableCoolCorrection);
	bool tempWarm = *(flags & RenderFlags::EnableWarmCorrection);
	bool tempCustom = *(flags & RenderFlags::EnableCustomCorrection);

	if (ImGui::Checkbox("Enable Cool Colour Correction", &tempCool)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableCoolCorrection) | (tempCool ? RenderFlags::EnableCoolCorrection: RenderFlags::None);
	}
	if (ImGui::Checkbox("Enable Warm Colour Correction", &tempWarm)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableWarmCorrection) | (tempWarm ? RenderFlags::EnableWarmCorrection : RenderFlags::None);
	}
	if (ImGui::Checkbox("Enable Custom Colour Correction", &tempCustom)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableCustomCorrection) | (tempCustom ? RenderFlags::EnableCustomCorrection : RenderFlags::None);
	}

	if (changed) {
		renderLayer->SetRenderFlags(flags);
	}
}
