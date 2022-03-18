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
	bool temp = *(flags & RenderFlags::EnableCoolCorrection);
	bool temp2 = *(flags & RenderFlags::EnableWarmCorrection);
	bool temp3 = *(flags & RenderFlags::EnableCustomCorrection);

	if (ImGui::Checkbox("Enable Cool Colour Correction", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableCoolCorrection) | (temp ? RenderFlags::EnableCoolCorrection: RenderFlags::None);
	}
	if (ImGui::Checkbox("Enable Warm Colour Correction", &temp2)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableWarmCorrection) | (temp2 ? RenderFlags::EnableWarmCorrection : RenderFlags::None);
	}
	if (ImGui::Checkbox("Enable Custom Colour Correction", &temp3)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableCustomCorrection) | (temp3 ? RenderFlags::EnableCustomCorrection : RenderFlags::None);
	}

	if (changed) {
		renderLayer->SetRenderFlags(flags);
	}
}
