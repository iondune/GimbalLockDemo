
#include <ionWindow.h>
#include <ionGraphics.h>
#include <ionGraphicsGL.h>
#include <ionScene.h>
#include <ionApplication.h>
#include <ionGUI.h>

#include <glm/gtc/quaternion.hpp>

using namespace ion;
using namespace ion::Scene;
using namespace ion::Graphics;



CSimpleMesh * CreateScreenQuad()
{
	CSimpleMesh * Mesh = new CSimpleMesh();

	Mesh->Vertices.resize(4);
	Mesh->Triangles.resize(2);

	Mesh->Vertices[0].Position = vec3f(-1, -1, 0);
	Mesh->Vertices[1].Position = vec3f(1, -1, 0);
	Mesh->Vertices[2].Position = vec3f(1, 1, 0);
	Mesh->Vertices[3].Position = vec3f(-1, 1, 0);

	Mesh->Triangles[0].Indices[0] = 0;
	Mesh->Triangles[0].Indices[1] = 1;
	Mesh->Triangles[0].Indices[2] = 2;
	Mesh->Triangles[1].Indices[0] = 0;
	Mesh->Triangles[1].Indices[1] = 2;
	Mesh->Triangles[1].Indices[2] = 3;

	return Mesh;
}

int main()
{
	////////////////////
	// ionEngine Init //
	////////////////////

	Log::AddDefaultOutputs();

	SingletonPointer<CGraphicsAPI> GraphicsAPI;
	SingletonPointer<CWindowManager> WindowManager;
	SingletonPointer<CTimeManager> TimeManager;
	SingletonPointer<CSceneManager> SceneManager;
	SingletonPointer<CAssetManager> AssetManager;
	SingletonPointer<CGUIManager> GUIManager;

	GraphicsAPI->Init(new COpenGLImplementation());
	WindowManager->Init(GraphicsAPI);
	TimeManager->Init(WindowManager);
	SceneManager->Init(GraphicsAPI);
	AssetManager->Init(GraphicsAPI);

	CWindow * Window = WindowManager->CreateWindow(vec2i(1920, 1080), "Shadow Maps", EWindowType::Windowed);

	GUIManager->Init(Window);
	Window->AddListener(GUIManager);

	AssetManager->AddAssetPath("Assets/");
	AssetManager->SetShaderPath("Shaders/");
	AssetManager->SetTexturePath("Textures/");
	AssetManager->SetMeshPath("Meshes/");

	SharedPointer<IGraphicsContext> Context = GraphicsAPI->GetWindowContext(Window);
	SharedPointer<IRenderTarget> BackBuffer = Context->GetBackBuffer();
	BackBuffer->SetClearColor(color3f(0.3f));

	SharedPointer<IFrameBuffer> ShadowBuffer = Context->CreateFrameBuffer();

	SharedPointer<ITexture2D> ShadowTexture = GraphicsAPI->CreateTexture2D(vec2i(4096), ITexture::EMipMaps::False, ITexture::EFormatComponents::RGBA, ITexture::EInternalFormatType::Fix8);
	SharedPointer<ITexture2D> ShadowDepth = GraphicsAPI->CreateTexture2D(vec2i(4096), ITexture::EMipMaps::False, ITexture::EFormatComponents::R, ITexture::EInternalFormatType::Depth);
	ShadowBuffer->AttachColorTexture(ShadowTexture, 0);
	ShadowBuffer->AttachDepthTexture(ShadowDepth);
	if (! ShadowBuffer->CheckCorrectness())
	{
		Log::Error("Frame buffer not valid!");
	}


	/////////////////
	// Load Assets //
	/////////////////

	CSimpleMesh * SphereMesh = CGeometryCreator::CreateSphere();
	CSimpleMesh * PlaneMesh = CGeometryCreator::CreatePlane(vec2f(100.f));
	CSimpleMesh * CubeMesh = CGeometryCreator::CreateCube();

	SharedPointer<IShader> DiffuseShader = AssetManager->LoadShader("Diffuse");
	SharedPointer<IShader> TextureShader = AssetManager->LoadShader("Texture");
	SharedPointer<IShader> ColorShader = AssetManager->LoadShader("Color");
	SharedPointer<IShader> QuadCopyShader = AssetManager->LoadShader("QuadCopy");


	////////////////////
	// ionScene Setup //
	////////////////////

	CRenderPass * ShadowPass = new CRenderPass(Context);
	ShadowPass->SetRenderTarget(ShadowBuffer);
	SceneManager->AddRenderPass(ShadowPass);

	CRenderPass * ColorPass = new CRenderPass(Context);
	ColorPass->SetRenderTarget(BackBuffer);
	SceneManager->AddRenderPass(ColorPass);

	CRenderPass * PostProcess = new CRenderPass(Context);
	PostProcess->SetRenderTarget(BackBuffer);
	SceneManager->AddRenderPass(PostProcess);

	CPerspectiveCamera * Camera = new CPerspectiveCamera(Window->GetAspectRatio());
	Camera->SetPosition(vec3f(15.25f, 7.3f, -11.85f));
	Camera->SetFocalLength(0.4f);
	Camera->SetFarPlane(200.f);
	ColorPass->SetActiveCamera(Camera);

	CCameraController * Controller = new CCameraController(Camera);
	Controller->SetTheta(2.347f);
	Controller->SetPhi(-0.326f);
	Window->AddListener(Controller);
	TimeManager->MakeUpdateTick(0.02)->AddListener(Controller);

	vec3f LightDirection = vec3f(4, -12, 4);
	float LightViewSize = 20.f;
	float LightNear = 115.f;
	float LightFar = 150.f;

	COrthographicCamera * LightCamera = new COrthographicCamera(-LightViewSize, LightViewSize, -LightViewSize, LightViewSize);
	LightCamera->SetPosition(-LightDirection * 10.f);
	LightCamera->SetLookDirection(LightDirection);
	LightCamera->SetNearPlane(LightNear);
	LightCamera->SetFarPlane(LightFar);
	ShadowPass->SetActiveCamera(LightCamera);


	/////////////////
	// Add Objects //
	/////////////////

	CSimpleMeshSceneObject * Plane = new CSimpleMeshSceneObject();
	Plane->SetMesh(PlaneMesh);
	Plane->SetShader(DiffuseShader);
	color3f const PlaneColor = color3f(0.5f);
	Plane->GetMaterial().Ambient *= PlaneColor;
	Plane->GetMaterial().Diffuse *= PlaneColor;
	Plane->SetPosition(vec3f(0, -4, 0));
	ColorPass->AddSceneObject(Plane);
	ShadowPass->AddSceneObject(Plane);

	CSimpleMesh * ShipMesh = AssetManager->LoadMeshMerged("SpaceShuttle.obj");
	CSimpleMeshSceneObject * ShipMeshObject = new CSimpleMeshSceneObject();
	ShipMeshObject->SetMesh(ShipMesh);
	ShipMeshObject->SetShader(TextureShader);
	ShipMeshObject->SetPosition(vec3f(0, 6, 0));
	ShipMeshObject->SetScale(vec3f(0.2f));
	ColorPass->AddSceneObject(ShipMeshObject);
	ShadowPass->AddSceneObject(ShipMeshObject);

	CCoordinateFrameSceneObject * WorldFrame = new CCoordinateFrameSceneObject();
	WorldFrame->SetShader(ColorShader);
	WorldFrame->SetPosition(vec3f(0, 2, 0));
	ColorPass->AddSceneObject(WorldFrame);
	ShadowPass->AddSceneObject(WorldFrame);

	CCoordinateFrameSceneObject * ShipFrame = new CCoordinateFrameSceneObject();
	ShipFrame->SetShader(ColorShader);
	ColorPass->AddSceneObject(ShipFrame);
	ShadowPass->AddSceneObject(ShipFrame);

	CSimpleMesh * RingMesh = AssetManager->LoadMeshMerged("ring.obj");
	RingMesh->CalculateNormalsPerFace();
	CSimpleMeshSceneObject * RingObjects[3] = {nullptr};
	
	for (int i = 0; i < 3; ++ i)
	{
		RingObjects[i] = new CSimpleMeshSceneObject();
		RingObjects[i]->SetMesh(RingMesh);
		RingObjects[i]->SetShader(DiffuseShader);
		RingObjects[i]->SetPosition(vec3f(0, 6, 0));
		float const Scales[3] = { 1.f, 0.8f, 0.6f };
		RingObjects[i]->SetScale(1.35f * Scales[i]);
		color3i const Colors[3] = {Color::Basic::Red, Color::Basic::Green, Color::Basic::Blue};
		RingObjects[i]->GetMaterial().Ambient *= Colors[i];
		RingObjects[i]->GetMaterial().Diffuse *= Colors[i];
		ColorPass->AddSceneObject(RingObjects[i]);
		ShadowPass->AddSceneObject(RingObjects[i]);
	}

	CSimpleMeshSceneObject * PostProcessObject = new CSimpleMeshSceneObject();
	PostProcessObject->SetMesh(CreateScreenQuad());
	PostProcessObject->SetShader(QuadCopyShader);
	PostProcessObject->SetTexture("uTexture", ShadowDepth);
	PostProcess->AddSceneObject(PostProcessObject);

	CDirectionalLight * Light1 = new CDirectionalLight();
	Light1->SetDirection(LightDirection);
	ColorPass->AddLight(Light1);
	ShadowPass->AddLight(Light1);

	CSimpleMeshSceneObject * LightSphere = new CSimpleMeshSceneObject();
	LightSphere->SetMesh(SphereMesh);
	LightSphere->SetShader(DiffuseShader);
	LightSphere->SetPosition(LightDirection * -2);
	LightSphere->SetScale(0.4f);
	LightSphere->GetMaterial().Ambient = 1.f / 0.75f;
	LightSphere->GetMaterial().Diffuse = 0;
	ColorPass->AddSceneObject(LightSphere);

	CUniform<glm::mat4> uLightMatrix;
	ColorPass->SetUniform("uLightMatrix", uLightMatrix);
	ColorPass->SetTexture("uShadowMap", ShadowDepth);

	// Obviously the shadow pass does not need these, but this will suppress warnings
	// An object that supports different shaders for different passes is needed
	ShadowPass->SetUniform("uLightMatrix", uLightMatrix);
	ShadowPass->SetTexture("uShadowMap", ShadowDepth);


	///////////////
	// Main Loop //
	///////////////

	vec3f rotation;
	glm::quat q;
	bool UseQuat = false;

	TimeManager->Init(WindowManager);
	while (WindowManager->Run())
	{
		TimeManager->Update();
		float const Elapsed = (float) TimeManager->GetElapsedTime();

		PostProcessObject->SetVisible(Window->IsKeyDown(EKey::F1));

		GUIManager->NewFrame();
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Once);
		ImGui::SetNextWindowSize(ImVec2(450, 450), ImGuiSetCond_Once);
		if (ImGui::Begin("Settings"))
		{
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("Camera position: %.3f %.3f %.3f", Camera->GetPosition().X, Camera->GetPosition().Y, Camera->GetPosition().Z);
			ImGui::Text("Camera rotation: %.3f %.3f", Controller->GetTheta(), Controller->GetPhi());

			ImGui::Separator();

			ImGui::SliderFloat("Light Camera Size", &LightViewSize, 1.f, 200.f);
			ImGui::SliderFloat("Light Near Plane", &LightNear, 1.f, 300.f);
			ImGui::SliderFloat("Light Far Plane", &LightFar, 1.f, 600.f);
			//ImGui::SliderFloat3("Light Direction", LightDirection.Values, -30.f, 30.f);
			ImGui::Text("Light Position: %.3f %.3f %.3f", LightCamera->GetPosition().X, LightCamera->GetPosition().Y, LightCamera->GetPosition().Z);

			if (ImGui::RadioButton("Euler Angles", UseQuat == false))
			{
				UseQuat = false;

				for (int i = 0; i < 3; ++ i)
				{
					RingObjects[i]->SetVisible(true);
				}
			}
			if (ImGui::RadioButton("Quaternions", UseQuat == true))
			{
				UseQuat = true;

				for (int i = 0; i < 3; ++ i)
				{
					RingObjects[i]->SetVisible(false);
				}
			}

			if (! UseQuat)
			{
				if (ImGui::Button("Reset"))
				{
					rotation = 0.f;
				}
				ImGui::SameLine();
				if (ImGui::Button("Lock"))
				{
					rotation = vec3f(90, 90, -90);
				}

				float const max = 180.f;
				ImGui::SliderFloat("Rotation (X)", &rotation.X, -max, max);
				ImGui::SliderFloat("Rotation (Y)", &rotation.Y, -max, max);
				ImGui::SliderFloat("Rotation (Z)", &rotation.Z, -max, max);

				float const RotationSpeed = 45.f * Elapsed;
				if (Window->IsKeyDown(EKey::U))
				{
					rotation.X += RotationSpeed;
				}
				if (Window->IsKeyDown(EKey::J))
				{
					rotation.X -= RotationSpeed;
				}
				if (Window->IsKeyDown(EKey::I))
				{
					rotation.Y += RotationSpeed;
				}
				if (Window->IsKeyDown(EKey::K))
				{
					rotation.Y -= RotationSpeed;
				}
				if (Window->IsKeyDown(EKey::O))
				{
					rotation.Z += RotationSpeed;
				}
				if (Window->IsKeyDown(EKey::L))
				{
					rotation.Z -= RotationSpeed;
				}
			}

			if (UseQuat)
			{
				vec3f relative;
				float const max = 2.f;
				ImGui::SliderFloat("Rotation (X)", &relative.X, -max, max);
				ImGui::SliderFloat("Rotation (Y)", &relative.Y, -max, max);
				ImGui::SliderFloat("Rotation (Z)", &relative.Z, -max, max);
				relative *= Elapsed;

				float const Speed = 1.f * Elapsed;
				if (Window->IsKeyDown(EKey::U))
				{
					relative.X += Speed;
				}
				if (Window->IsKeyDown(EKey::J))
				{
					relative.X -= Speed;
				}
				if (Window->IsKeyDown(EKey::I))
				{
					relative.Y += Speed;
				}
				if (Window->IsKeyDown(EKey::K))
				{
					relative.Y -= Speed;
				}
				if (Window->IsKeyDown(EKey::O))
				{
					relative.Z += Speed;
				}
				if (Window->IsKeyDown(EKey::L))
				{
					relative.Z -= Speed;
				}

				q = glm::rotate(q, relative.X, glm::vec3(1, 0, 0));
				q = glm::rotate(q, relative.Y, glm::vec3(0, 1, 0));
				q = glm::rotate(q, relative.Z, glm::vec3(0, 0, 1));
			}

			//ImGui::Image(GUIManager->GetTextureID(ShadowTexture), vec2f(512));
			//ImGui::Image(GUIManager->GetTextureID(ShadowDepth), vec2f(512));
		}
		ImGui::End();


		LightCamera->SetLeft(-LightViewSize);
		LightCamera->SetRight(LightViewSize);
		LightCamera->SetBottom(-LightViewSize);
		LightCamera->SetTop(LightViewSize);
		LightCamera->SetPosition(-LightDirection * 10.f);
		LightCamera->SetLookDirection(LightDirection);
		LightCamera->SetNearPlane(LightNear);
		LightCamera->SetFarPlane(LightFar);
		LightCamera->Update();
		uLightMatrix = LightCamera->GetProjectionMatrix() * LightCamera->GetViewMatrix();


		if (! UseQuat)
		{
			glm::mat4 m = glm::mat4(1.f);
			glm::mat4 top = glm::mat4(1.f);

			m = glm::rotate(m, glm::radians(rotation.X), glm::vec3(1, 0, 0));
			top = m;
			top = glm::rotate(top, glm::radians(-90.f), glm::vec3(1, 0, 0));
			top = glm::rotate(top, glm::radians(-90.f), glm::vec3(0, 1, 0));
			RingObjects[0]->SetRotation(top);

			m = glm::rotate(m, glm::radians(rotation.Y), glm::vec3(0, 1, 0));
			top = m;
			top = glm::rotate(top, glm::radians(-90.f), glm::vec3(0, 1, 0));
			top = glm::rotate(top, glm::radians(-90.f), glm::vec3(1, 0, 0));
			RingObjects[1]->SetRotation(top);

			m = glm::rotate(m, glm::radians(rotation.Z), glm::vec3(0, 0, 1));
			top = m;
			//top = glm::rotate(top, glm::radians(-90.f), glm::vec3(1, 0, 0));
			RingObjects[2]->SetRotation(top);


			m = glm::rotate(m, glm::radians(-90.f), glm::vec3(1, 0, 0));
			ShipMeshObject->SetRotation(m);

			ShipFrame->SetPosition(ShipMeshObject->GetPosition());
			ShipFrame->SetRotation(m);
			ShipFrame->SetScale(3.f);
		}

		if (UseQuat)
		{
			glm::mat4 m = glm::mat4_cast(q);
			ShipMeshObject->SetRotation(m);

			ShipFrame->SetPosition(ShipMeshObject->GetPosition());
			ShipFrame->SetRotation(m);
			ShipFrame->SetScale(3.f);
		}



		ShadowBuffer->ClearColorAndDepth();
		BackBuffer->ClearColorAndDepth();
		SceneManager->DrawAll();


		GUIManager->Draw();

		Window->SwapBuffers();
	}

	return 0;
}
