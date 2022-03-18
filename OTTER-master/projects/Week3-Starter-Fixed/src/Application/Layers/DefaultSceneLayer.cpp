#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});
		specShader->SetDebugName("Textured-Specular");

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		toonShader->SetDebugName("Toon Shader");

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our tangent space normal mapping
		ShaderProgram::Sptr tangentSpaceMapping = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		tangentSpaceMapping->SetDebugName("Tangent Space Mapping");

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing");

		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr intactPillarMesh = ResourceManager::CreateAsset<MeshResource>("IntactPillar.obj");
		MeshResource::Sptr damagedPillarMesh = ResourceManager::CreateAsset<MeshResource>("DamagedPillar.obj");
		MeshResource::Sptr destroyedPillarMesh = ResourceManager::CreateAsset<MeshResource>("DestroyedPillar.obj");
		MeshResource::Sptr goblinSwordMesh = ResourceManager::CreateAsset<MeshResource>("GoblinSword.obj");
		MeshResource::Sptr swordMesh = ResourceManager::CreateAsset<MeshResource>("Sword.obj");
		MeshResource::Sptr mugMesh = ResourceManager::CreateAsset<MeshResource>("Mug_With_Handle.obj");
		MeshResource::Sptr treeMesh = ResourceManager::CreateAsset<MeshResource>("LightPineTree.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		Texture2D::Sptr    intactPillarTex      = ResourceManager::CreateAsset<Texture2D>("textures/PillarUV.png");
		Texture2D::Sptr    damagedPillarTex      = ResourceManager::CreateAsset<Texture2D>("textures/DamagedPillarUV.png");
		Texture2D::Sptr    destroyedPillarTex      = ResourceManager::CreateAsset<Texture2D>("textures/DestroyedPillarUV.png");
		Texture2D::Sptr    goblinSwordTex      = ResourceManager::CreateAsset<Texture2D>("textures/GoblinSwordUV.png");
		Texture2D::Sptr    grassTex      = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");
		Texture2D::Sptr    treeTex      = ResourceManager::CreateAsset<Texture2D>("textures/terrain/TreeTextureUVS.png");

		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);


		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>(); 

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/AssignmentCool.CUBE");
		Texture3D::Sptr lut2 = ResourceManager::CreateAsset<Texture3D>("luts/AssignmentWarm.CUBE");
		Texture3D::Sptr lut3 = ResourceManager::CreateAsset<Texture3D>("luts/AssignmentCustom.CUBE");

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			grassMaterial->Name = "Grass";
			grassMaterial->Set("u_Material.Diffuse", grassTex);
			grassMaterial->Set("u_Material.Shininess", 0.1f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr swordMaterial = ResourceManager::CreateAsset<Material>(reflectiveShader);
		{
			swordMaterial->Name = "Sword";
			swordMaterial->Set("u_Material.Diffuse", damagedPillarTex);
			swordMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr damagedPillarMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			damagedPillarMaterial->Name = "Damaged Pillar";
			damagedPillarMaterial->Set("u_Material.Diffuse", damagedPillarTex);
			damagedPillarMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr destroyedPillarMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			destroyedPillarMaterial->Name = "Destroyed Pillar";
			destroyedPillarMaterial->Set("u_Material.Diffuse", destroyedPillarTex);
			destroyedPillarMaterial->Set("u_Material.Specular", boxSpec);
		}

		Material::Sptr intactPillarMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			intactPillarMaterial->Name = "Intact Pillar";
			intactPillarMaterial->Set("u_Material.Diffuse", intactPillarTex);
			intactPillarMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr treeMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			treeMaterial->Name = "Tree";
			treeMaterial->Set("u_Material.Diffuse", treeTex);
			treeMaterial->Set("u_Material.Shininess", 0.1f);
		}

		// Our foliage vertex shader material
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.Diffuse", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.Threshold", 0.1f);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(toonShader);
		{
			toonMaterial->Name = "Toon";
			toonMaterial->Set("u_Material.Diffuse", boxTexture);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f);
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.Diffuse", diffuseMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("s_NormalMap", normalMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(tangentSpaceMapping);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.Diffuse", diffuseMap);
			normalmapMat->Set("s_NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand  = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f);
		}

		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 50.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 1000.0f;

		/*scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);*/

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ -9, -6, 15 });
			camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		GameObject::Sptr sword = scene->CreateGameObject("Sword");
		{
			// Set position in the scene
			sword->SetPostion(glm::vec3(-0.03f, 12.0f, -1.5f));
			sword->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			sword->SetScale(glm::vec3(0.1f, 0.1f, 0.1f));

			// Add some behaviour that relies on the physics body
			//sword->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = sword->Add<RenderComponent>();
			renderer->SetMesh(swordMesh);
			renderer->SetMaterial(swordMaterial);

			// Example of a trigger that interacts with static and kinematic bodies as well as dynamic bodies
			TriggerVolume::Sptr trigger = sword->Add<TriggerVolume>();
			trigger->SetFlags(TriggerTypeFlags::Statics | TriggerTypeFlags::Kinematics);
			trigger->AddCollider(BoxCollider::Create(glm::vec3(1.0f)));

			sword->Add<TriggerVolumeEnterBehaviour>();
		}

		//GameObject::Sptr demoBase = scene->CreateGameObject("Demo Parent");

		// Box to showcase the specular material
		GameObject::Sptr destroyedPillar = scene->CreateGameObject("Pedestal");
		{
			/*MeshResource::Sptr boxMesh = ResourceManager::CreateAsset<MeshResource>();
			boxMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			boxMesh->GenerateMesh();*/

			// Set and rotation position in the scene
			destroyedPillar->SetPostion(glm::vec3(0, 12.07f, -0.01f));
			destroyedPillar->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			destroyedPillar->SetScale(glm::vec3(0.6f, 0.6f, 0.6f));

			// Add a render component
			RenderComponent::Sptr renderer = destroyedPillar->Add<RenderComponent>();
			renderer->SetMesh(destroyedPillarMesh);
			renderer->SetMaterial(destroyedPillarMaterial);
		}

		GameObject::Sptr intactPillar = scene->CreateGameObject("Left Intact Pillar");
		{
			// Set and rotation position in the scene
			intactPillar->SetPostion(glm::vec3(-5.f, 0.f, -0.01f));
			intactPillar->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			intactPillar->SetScale(glm::vec3(1.f, 1.f, 1.f));

			// Add a render component
			RenderComponent::Sptr renderer = intactPillar->Add<RenderComponent>();
			renderer->SetMesh(intactPillarMesh);
			renderer->SetMaterial(intactPillarMaterial);
		}

		GameObject::Sptr intactPillar2 = scene->CreateGameObject("Right Intact Pillar");
		{
			// Set and rotation position in the scene
			intactPillar2->SetPostion(glm::vec3(5.f, 7.f, -0.01f));
			intactPillar2->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			intactPillar2->SetScale(glm::vec3(1.f, 1.f, 1.f));

			// Add a render component
			RenderComponent::Sptr renderer = intactPillar2->Add<RenderComponent>();
			renderer->SetMesh(intactPillarMesh);
			renderer->SetMaterial(intactPillarMaterial);
		}

		GameObject::Sptr intactPillar3 = scene->CreateGameObject("Tilted Pillar");
		{
			// Set and rotation position in the scene
			intactPillar3->SetPostion(glm::vec3(34.29f, 34.45f, -1.11f));
			intactPillar3->SetRotation(glm::vec3(158.f, 0.f, -30.f));
			intactPillar3->SetScale(glm::vec3(3.f, 3.f, 3.f));

			// Add a render component
			RenderComponent::Sptr renderer = intactPillar3->Add<RenderComponent>();
			renderer->SetMesh(intactPillarMesh);
			renderer->SetMaterial(intactPillarMaterial);
		}

		GameObject::Sptr damagedPillar = scene->CreateGameObject("Left Damaged Pillar");
		{
			// Set and rotation position in the scene
			damagedPillar->SetPostion(glm::vec3(-5.f, 7.f, -0.01f));
			damagedPillar->SetRotation(glm::vec3(90.f, 0.f, -90.f));
			damagedPillar->SetScale(glm::vec3(1.f, 1.f, 1.f));

			// Add a render component
			RenderComponent::Sptr renderer = damagedPillar->Add<RenderComponent>();
			renderer->SetMesh(damagedPillarMesh);
			renderer->SetMaterial(damagedPillarMaterial);
		}

		GameObject::Sptr destroyedPillar2 = scene->CreateGameObject("Right Destroyed Pillar");
		{
			// Set and rotation position in the scene
			destroyedPillar2->SetPostion(glm::vec3(5.f, 0.f, -0.01f));
			destroyedPillar2->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			destroyedPillar2->SetScale(glm::vec3(1.f, 1.f, 1.f));

			// Add a render component
			RenderComponent::Sptr renderer = destroyedPillar2->Add<RenderComponent>();
			renderer->SetMesh(destroyedPillarMesh);
			renderer->SetMaterial(destroyedPillarMaterial);
		}

		GameObject::Sptr tree1 = scene->CreateGameObject("Tree 1");
		{
			// Set and rotation position in the scene
			tree1->SetPostion(glm::vec3(8.49f, 19.63f, 0.f));
			tree1->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree1->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));

			// Add a render component
			RenderComponent::Sptr renderer = tree1->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		GameObject::Sptr tree2 = scene->CreateGameObject("Tree 2");
		{
			// Set and rotation position in the scene
			tree2->SetPostion(glm::vec3(-11.09f, -8.6f, 0.f));
			tree2->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree2->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));

			// Add a render component
			RenderComponent::Sptr renderer = tree2->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		GameObject::Sptr tree3 = scene->CreateGameObject("Tree 3");
		{
			// Set and rotation position in the scene
			tree3->SetPostion(glm::vec3(15.72f, -19.22f, 0.f));
			tree3->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree3->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));

			// Add a render component
			RenderComponent::Sptr renderer = tree3->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		GameObject::Sptr tree4 = scene->CreateGameObject("Tree 4");
		{
			// Set and rotation position in the scene
			tree4->SetPostion(glm::vec3(-31.74f, 11.39f, 0.f));
			tree4->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree4->SetScale(glm::vec3(0.2f, 0.2f, 0.2f));

			// Add a render component
			RenderComponent::Sptr renderer = tree4->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		GameObject::Sptr tree5 = scene->CreateGameObject("Tree 5");
		{
			// Set and rotation position in the scene
			tree5->SetPostion(glm::vec3(25.97f, -7.22f, 0.f));
			tree5->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree5->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

			// Add a render component
			RenderComponent::Sptr renderer = tree5->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		GameObject::Sptr tree6 = scene->CreateGameObject("Tree 6");
		{
			// Set and rotation position in the scene
			tree6->SetPostion(glm::vec3(-21.13f, 27.91f, 0.f));
			tree6->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree6->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

			// Add a render component
			RenderComponent::Sptr renderer = tree6->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		GameObject::Sptr tree7 = scene->CreateGameObject("Tree 7");
		{
			// Set and rotation position in the scene
			tree7->SetPostion(glm::vec3(-31.27f, -29.66f, 0.f));
			tree7->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree7->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

			// Add a render component
			RenderComponent::Sptr renderer = tree7->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		GameObject::Sptr tree8 = scene->CreateGameObject("Tree 8");
		{
			// Set and rotation position in the scene
			tree8->SetPostion(glm::vec3(-5.56f, -29.56f, 0.f));
			tree8->SetRotation(glm::vec3(90.f, 0.f, 0.f));
			tree8->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

			// Add a render component
			RenderComponent::Sptr renderer = tree8->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);
		}

		// sphere to showcase the foliage material
		//GameObject::Sptr foliageBall = scene->CreateGameObject("Foliage Sphere");
		//{
		//	// Set and rotation position in the scene
		//	foliageBall->SetPostion(glm::vec3(-4.0f, -4.0f, 1.0f));

		//	// Add a render component
		//	RenderComponent::Sptr renderer = foliageBall->Add<RenderComponent>();
		//	renderer->SetMesh(sphere);
		//	renderer->SetMaterial(foliageMaterial);

		//	demoBase->AddChild(foliageBall);
		//}

		//// Box to showcase the foliage material
		//GameObject::Sptr foliageBox = scene->CreateGameObject("Foliage Box");
		//{
		//	MeshResource::Sptr box = ResourceManager::CreateAsset<MeshResource>();
		//	box->AddParam(MeshBuilderParam::CreateCube(glm::vec3(0, 0, 0.5f), ONE));
		//	box->GenerateMesh();

		//	// Set and rotation position in the scene
		//	foliageBox->SetPostion(glm::vec3(-6.0f, -4.0f, 1.0f));

		//	// Add a render component
		//	RenderComponent::Sptr renderer = foliageBox->Add<RenderComponent>();
		//	renderer->SetMesh(box);
		//	renderer->SetMaterial(foliageMaterial);

		//	demoBase->AddChild(foliageBox);
		//}

		//// Box to showcase the specular material
		//GameObject::Sptr toonBall = scene->CreateGameObject("Toon Object");
		//{
		//	// Set and rotation position in the scene
		//	toonBall->SetPostion(glm::vec3(-2.0f, -4.0f, 1.0f));

		//	// Add a render component
		//	RenderComponent::Sptr renderer = toonBall->Add<RenderComponent>();
		//	renderer->SetMesh(sphere);
		//	renderer->SetMaterial(toonMaterial);

		//	demoBase->AddChild(toonBall);
		//}

		//GameObject::Sptr displacementBall = scene->CreateGameObject("Displacement Object");
		//{
		//	// Set and rotation position in the scene
		//	displacementBall->SetPostion(glm::vec3(2.0f, -4.0f, 1.0f));

		//	// Add a render component
		//	RenderComponent::Sptr renderer = displacementBall->Add<RenderComponent>();
		//	renderer->SetMesh(sphere);
		//	renderer->SetMaterial(displacementTest);

		//	demoBase->AddChild(displacementBall);
		//}

		//GameObject::Sptr multiTextureBall = scene->CreateGameObject("Multitextured Object");
		//{
		//	// Set and rotation position in the scene 
		//	multiTextureBall->SetPostion(glm::vec3(4.0f, -4.0f, 1.0f));

		//	// Add a render component 
		//	RenderComponent::Sptr renderer = multiTextureBall->Add<RenderComponent>();
		//	renderer->SetMesh(sphere);
		//	renderer->SetMaterial(multiTextureMat);

		//	demoBase->AddChild(multiTextureBall);
		//}

		//GameObject::Sptr normalMapBall = scene->CreateGameObject("Normal Mapped Object");
		//{
		//	// Set and rotation position in the scene 
		//	normalMapBall->SetPostion(glm::vec3(6.0f, -4.0f, 1.0f));

		//	// Add a render component 
		//	RenderComponent::Sptr renderer = normalMapBall->Add<RenderComponent>();
		//	renderer->SetMesh(sphere);
		//	renderer->SetMaterial(normalmapMat);

		//	demoBase->AddChild(normalMapBall);
		//}

		//// Create a trigger volume for testing how we can detect collisions with objects!
		//GameObject::Sptr trigger = scene->CreateGameObject("Trigger");
		//{
		//	TriggerVolume::Sptr volume = trigger->Add<TriggerVolume>();
		//	CylinderCollider::Sptr collider = CylinderCollider::Create(glm::vec3(3.0f, 3.0f, 1.0f));
		//	collider->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
		//	volume->AddCollider(collider);

		//	trigger->Add<TriggerVolumeEnterBehaviour>();
		//}

		/////////////////////////// UI //////////////////////////////
		/*
		GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas");
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 256, 256 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();


			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 128, 128 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/upArrow.png"));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);

				monkey1->Get<JumpBehaviour>()->Panel = text;
			}

			canvas->AddChild(subPanel);
		}
		*/

		/*GameObject::Sptr particles = scene->CreateGameObject("Particles");
		{
			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();  
			particleManager->AddEmitter(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 10.0f), 10.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); 
		}*/

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
