#include"GraphicsClass.h"

GraphicsClass::GraphicsClass(int ScreenWidth, int ScreenHeight, HWND hwnd, HINSTANCE hinstance)
{
	Initialize(ScreenWidth, ScreenHeight, hwnd, hinstance);
}

GraphicsClass::~GraphicsClass()
{
	CloseDebugConsole();
}

GraphicsClass::GraphicsClass(const GraphicsClass&other)
{

}

bool GraphicsClass::Initialize(int ScreenWidth, int ScreenHeight, HWND hwnd,HINSTANCE hinstance)
{

	mhwnd = hwnd;

	m_nScreenWidth = ScreenWidth;
	m_nScreenHeight = ScreenHeight;

	//创建D3DClass类并且初始化,D3DClass是第一个被创建并且初始化
	GDirectxCore->Init(ScreenWidth, ScreenHeight, VSYNC_ENABLE, hwnd, FULL_SCREEN, SCREEN_NEAR, SCREEN_FAR);

	//Console的Debug窗口
	InitDebugConsole();

	//创建相机
	GCamera->SetProjParams(XM_PI / 3.0f, (float)ScreenWidth / (float)ScreenHeight, SCREEN_NEAR, SCREEN_FAR);
	GCamera->SetUIOrthoParams((float)ScreenWidth, (float)ScreenHeight);

	//给游戏添加灯光
	shared_ptr<DirectionLight> m_spDirLight = shared_ptr<DirectionLight>(new DirectionLight());
	m_spDirLight->SetLightPostion(XMFLOAT3(12.0f, 12.0f, 12.0f));
	m_spDirLight->SetLookAtPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	m_spDirLight->SetAmbientLight(XMFLOAT3(0.05f, 0.05f, 0.05f));
	m_spDirLight->SetLightColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	GLightManager->Add(m_spDirLight);

	//创建mesh
	shared_ptr<Mesh> pHeadMesh = shared_ptr<Mesh>(new Mesh("Resource\\FBXModel\\head\\head.FBX"));
	pHeadMesh->m_eMaterialType = MaterialType::DIFFUSE;

	shared_ptr<Mesh> pSphereMesh = shared_ptr<Mesh>(new Mesh("Resource\\FBXModel\\sphere\\sphere.FBX"));
	pSphereMesh->m_eMaterialType = MaterialType::PURE_COLOR;
	pSphereMesh->pureColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	shared_ptr<Mesh> pSponzaBottom = shared_ptr<Mesh>(new Mesh("Resource\\FBXModel\\sponza\\sponza_bottom.FBX"));
	pSponzaBottom->m_eMaterialType = MaterialType::DIFFUSE;
	
	shared_ptr<Mesh> pSponzaNoBottom = shared_ptr<Mesh>(new Mesh("Resource\\FBXModel\\sponza\\sponza_no_bottom.FBX"));
	pSponzaNoBottom->bReflect = true;
	pSponzaNoBottom->bTransparent = true;
	pSponzaNoBottom->m_eMaterialType = MaterialType::DIFFUSE;

	//创建GameObject
	mHeadObject = shared_ptr<GameObject>(new GameObject());
	mHeadObject->SetMesh(pHeadMesh);
	mHeadObject->m_pTransform->localPosition = 

	mSphereObject = shared_ptr<GameObject>(new GameObject());
	mSphereObject->SetMesh(pSphereMesh);

	mSponzaBottom = shared_ptr<GameObject>(new GameObject());
	mSponzaBottom->SetMesh(pSponzaBottom);

	mSponzaNoBottom = shared_ptr<GameObject>(new GameObject());
	mSponzaNoBottom->SetMesh(pSponzaNoBottom);

	//创建输入类
	mInputClass = shared_ptr<Input>(new Input(hinstance, hwnd, ScreenWidth, ScreenHeight));

	mSrcRT = shared_ptr<RenderTexture>(
		new RenderTexture(ScreenWidth, ScreenHeight));

	mSSRRT = shared_ptr<RenderTexture>(
		new RenderTexture(ScreenWidth, ScreenHeight));

	mGeometryBuffer = shared_ptr<GeometryBuffer>(new 
		GeometryBuffer(ScreenWidth,ScreenHeight,SCREEN_FAR,SCREEN_NEAR));

	mQuad = shared_ptr<Quad>(new Quad());

	mDebugWindow = shared_ptr<DebugWindow>(new DebugWindow(ScreenWidth,ScreenHeight,120,120));

	mBackDepthBufferRT = shared_ptr<DepthBufferRT>(new DepthBufferRT(ScreenWidth, ScreenHeight));

	mSSRBuffer = shared_ptr<SSRGBuffer>(new 
		SSRGBuffer(ScreenWidth, ScreenHeight, SCREEN_FAR, SCREEN_NEAR));

	return true;
}


bool GraphicsClass::Frame()
{
	bool result;
	float deltaTime;
	int mouseXOffset, mouseYOffset;
	static float rotateY = 0.0f;

	//进行InputClass对象的帧函数
	result = mInputClass->Frame();
	if (!result)
	{
	return false;
	}
	
	FPS* fpsPtr = FPS::GetInstance();
	fpsPtr->Frame();

	int fps = fpsPtr->GetFPS();

	deltaTime = fpsPtr->GetDeltaTime();
	float currentTime = fpsPtr->GetTime();
	mInputClass->GetMousePositionOffset(mouseXOffset, mouseYOffset);


	//鼠标右键处于按下的状态才能进行（左右移动）（前后移动）（旋转的操作）
	if (mInputClass->IsMouseRightButtuonPressed()&& fps >=5&& fps <=1000000)
	{
		//"W","S"键操作
		if (mInputClass->IsWPressed())
		{
			GCamera->Walk(deltaTime*CAMERA_SPEED);
		}
		else if (mInputClass->IsSPressed())
		{
			GCamera->Walk(-deltaTime*CAMERA_SPEED);
		}

		//"A","D"键操作
		if (mInputClass->IsAPressed())
		{
			GCamera->Strafe(-deltaTime*CAMERA_SPEED);
		}
		else if (mInputClass->IsDPressed())
		{
			GCamera->Strafe(deltaTime*CAMERA_SPEED);
		}

		//"Q","E"键操作
		if (mInputClass->IsQPressed())
		{
			GCamera->UpDown(-deltaTime*CAMERA_SPEED);
		}
		else if (mInputClass->IsEPressed())
		{
			GCamera->UpDown(deltaTime*CAMERA_SPEED);
		}


		//进行视角上下的旋转(跟刚开始的旋转角度在正负90度之间)
		if (rotateY <= 90.0f&&rotateY >= -90.0f)
		{
			rotateY += (float)mouseYOffset*deltaTime;
			GCamera->Pitch((float)mouseYOffset*deltaTime*2.0f);
		}
	
		//进行视角左右的旋转
		GCamera->RotateY((float)mouseXOffset*deltaTime*2.0f);
	}


	//1与2切换渲染模式
	if (mInputClass->IsKeyDown(DIK_1))
	{
		GDirectxCore->TurnOnSolidRender();
	}
	if (mInputClass->IsKeyDown(DIK_2))
	{
		GDirectxCore->TurnOnWireFrameRender();
	}
	if (mInputClass->IsKeyDown(DIK_3))
	{
		materialType = MaterialType::DIFFUSE;
	}
	if (mInputClass->IsKeyDown(DIK_4))
	{
		materialType = MaterialType::DIFFUSE_NORMAL;
	}
	if (mInputClass->IsKeyDown(DIK_5))
	{
		materialType = MaterialType::DIFFUSE_SPECULAR;
	}
	if (mInputClass->IsKeyDown(DIK_6))
	{
		materialType = MaterialType::DIFFUSE_NORMAL_SPECULAR;
	}

	GCamera->UpdateViewMatrix();
	//如果按下ESC，则破坏窗口
	if (mInputClass->IsEscapePressed())
	{
		DestroyWindow(mhwnd);
	}

	GCamera->UpdateViewMatrix();

	return true;
}


void GraphicsClass::Render()
{
	//**************************************************************************
	//绘制整个场景
	//*************************************************************************

	GDirectxCore->BeginScene(0.3f, 0.0f, 1.0f, 1.0f);

	//绘制不透明物体
	RenderOpacity();

	//获取整个场景的BackDepthBuffer
	//RenderSceneBackDepthBuffer();

	//绘制透明物体(普通的透明物体，SSR)
	RenderTransparency();

	#if defined(POST_EFFECT)
		RenderPostEffectPass();
	#endif // POST_EFFECT

	#if defined(DEBUG_GBUFFER)
		RenderDebugWindow();
	#endif


	//**************************************************************************
	//结束绘制
	//*************************************************************************
	GDirectxCore->EndScene();
}



void GraphicsClass::RenderFBXMesh()
{
	//渲染头
	mHeadObject->mTransform->localScale = XMFLOAT3(5.0f, 5.0f, 5.0f);
	mHeadObject->mTransform->localPosition = XMFLOAT3(0.0f, 10.0f, 0.0f);
	mHeadObject->mTransform->localRotation = XMFLOAT3(0.0f, 90.0f, 0.0f);
	mHeadObject->Render(materialType);

	//渲染SponzaBottom(用于SSR反射)
	GDirectxCore->TurnOnMaskReflectDSS();
	mSponzaBottom->mTransform->localPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSponzaBottom->Render(MaterialType::DIFFUSE);
	GDirectxCore->TurnOnZBuffer();//恢复默认的DSS

	//渲染SponzaNoBottm
	mSponzaNoBottom->mTransform->localPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSponzaNoBottom->Render(MaterialType::DIFFUSE);

	//球渲染
	mSphereObject->mTransform->localScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	//平行光源球
	mSphereObject->mTransform->localPosition = GLightManager->GetMainLight()->GetLightDirection();
	mSphereObject->Render(MaterialType::PURE_COLOR, XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f));

	//原点球
	mSphereObject->mTransform->localPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSphereObject->Render(MaterialType::PURE_COLOR, XMVectorSet(1.0f,1.0f,0.0f,1.0f));
}

void GraphicsClass::RenderGeometryPass()
{
	mGeometryBuffer->SetRenderTarget(XMFLOAT3(0.0f, 0.0f, 0.5f));

	RenderFBXMesh();
}

void GraphicsClass::RenderLightingPass()
{
	mSrcRT->SetRenderTarget();
	GDirectxCore->TurnOffZBuffer();
	ID3D11ShaderResourceView* shaderResourceView[4];
	shaderResourceView[0] = mGeometryBuffer->GetGBufferSRV(GBufferType::Diffuse);
	shaderResourceView[1] = mGeometryBuffer->GetGBufferSRV(GBufferType::Pos);
	shaderResourceView[2] = mGeometryBuffer->GetGBufferSRV(GBufferType::Normal);
	shaderResourceView[3] = mGeometryBuffer->GetGBufferSRV(GBufferType::Specular);
	GShaderManager->SetDefferLighingShader(shaderResourceView);
	mQuad->Render();

	GDirectxCore->SetBackBufferRender();
	GDirectxCore->SetViewPort();
	GShaderManager->SetGraphcisBlitShader(mSrcRT->GetSRV());
	mQuad->Render();

	GDirectxCore->TurnOnZBuffer();
}



void GraphicsClass::RenderPostEffectPass()
{

}


void GraphicsClass::RenderDebugWindow()
{
	GDirectxCore->SetBackBufferRender();
	GDirectxCore->SetViewPort();
	GDirectxCore->TurnOffZBuffer();

	//diffuse
	GShaderManager->SetUIShader
	(mGeometryBuffer->GetGBufferSRV(GBufferType::Diffuse));
	mDebugWindow->Render(10, 600);

	//pos
	GShaderManager->SetUIShader
	(mGeometryBuffer->GetGBufferSRV(GBufferType::Pos));
	mDebugWindow->Render(130, 600);

	//normal
	GShaderManager->SetUIShader
	(mGeometryBuffer->GetGBufferSRV(GBufferType::Normal));
	mDebugWindow->Render(250, 600);

	//specular
	GShaderManager->SetUIShader
	(mGeometryBuffer->GetGBufferSRV(GBufferType::Specular));
	mDebugWindow->Render(370, 600);

	GShaderManager->SetDepthShader
	(mGeometryBuffer->GetGBufferSRV(GBufferType::Depth));
	mDebugWindow->Render(490, 600);


	GShaderManager->SetDepthShader
	(mBackDepthBufferRT->GetShaderResourceView());
	mDebugWindow->Render(610, 600);

	GShaderManager->SetUIShader
	(mSSRBuffer->GetGBufferSRV(SSRBufferType::VIEW_NORMAL));
	mDebugWindow->Render(10, 480);

	GShaderManager->SetUIShader
	(mSSRBuffer->GetGBufferSRV(SSRBufferType::VIEW_POS));
	mDebugWindow->Render(10, 360);

	GDirectxCore->TurnOnZBuffer();
}

void GraphicsClass::RenderSSRPass()
{
	//mSSRRT->SetRenderTarget();
	ID3D11RenderTargetView* backRTV =GDirectxCore->GetRTV();
	//靠模板緩存值來判斷
	g_pDeviceContext->OMSetRenderTargets(1, &backRTV, nullptr);
	GDirectxCore->SetViewPort();
	GDirectxCore->TurnOnAlphaBlend();
	GDirectxCore->TurnOffZBuffer();
	XMMATRIX worldMatrix = mSponzaBottom->GetWorldMatrix();
	XMMATRIX projMatrix = GCamera->GetProjectionMatrix();
	XMFLOAT4X4 projFloat4X4;
	XMStoreFloat4x4(&projFloat4X4, projMatrix);
	XMFLOAT2 perspectiveValues;
	perspectiveValues.x = projFloat4X4.m[3][2];
	perspectiveValues.y = -projFloat4X4.m[2][2];
	ID3D11ShaderResourceView* arraySRV[5];
	arraySRV[0] = mSrcRT->GetSRV();
	arraySRV[1] = mGeometryBuffer->GetGBufferSRV(GBufferType::Depth);
	arraySRV[2] = mBackDepthBufferRT->GetShaderResourceView();
	arraySRV[3] = mSSRBuffer->GetGBufferSRV(SSRBufferType::VIEW_POS);
	arraySRV[4] = mSSRBuffer->GetGBufferSRV(SSRBufferType::VIEW_NORMAL);
	GShaderManager->SetSSRShader(worldMatrix,arraySRV,perspectiveValues);
	mQuad->Render();
	GDirectxCore->TurnOnZBuffer();
	GDirectxCore->TurnOffAlphaBlend();
}

void GraphicsClass::RenderOpacity()
{
	RenderGeometryPass();

	RenderLightingPass();
}

//绘制透明物体分为绘制透明
void GraphicsClass::RenderTransparency()
{
	RenderGeneralTransparency();
	#if defined(SSR)
	RenderSSR();
	#endif
}

void GraphicsClass::RenderGeneralTransparency()
{
	ID3D11DeviceContext* d3dDeviceContext = GDirectxCore->GetDeviceContext();
	ID3D11RenderTargetView* backRTV = GDirectxCore->GetRTV();
	ID3D11DepthStencilView* opacityDSV = mGeometryBuffer->GetDSV();
	d3dDeviceContext->OMSetRenderTargets(1, &backRTV, opacityDSV);
	GDirectxCore->SetViewPort();
	GDirectxCore->TurnOnDisbleZWriteDSS();
	GDirectxCore->TurnOnAlphaBlend();

	mSphereObject->mTransform->localPosition = XMFLOAT3(3.0, 9.0, 0.0);
	mSphereObject->mTransform->localScale = XMFLOAT3(3.0, 3.0, 3.0);
	XMMATRIX worldMatrix = mSphereObject->GetWorldMatrix();
	GShaderManager->SetForwardPureColorShader(worldMatrix, XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f));
	mSphereObject->RenderMesh();
	GDirectxCore->TurnOffAlphaBlend();
	GDirectxCore->RecoverDefaultDSS();
}


void GraphicsClass::RenderSceneBackDepthBuffer()
{
	mBackDepthBufferRT->SetRenderTarget();
	GDirectxCore->TurnOnCullFront();

	//渲染头
	mHeadObject->mTransform->localScale = XMFLOAT3(5.0f, 5.0f, 5.0f);
	mHeadObject->mTransform->localPosition = XMFLOAT3(0.0f, 10.0f, 0.0f);
	mHeadObject->mTransform->localRotation = XMFLOAT3(0.0f, 90.0f, 0.0f);
	mHeadObject->Render(MaterialType::DEPTH_BUFFER);

	//渲染SponzaBottom
	mSponzaBottom->mTransform->localPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSponzaBottom->Render(MaterialType::DEPTH_BUFFER);

    //渲染SponzaNoBottm
	mSponzaNoBottom->mTransform->localPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSponzaNoBottom->Render(MaterialType::DEPTH_BUFFER);

	//球渲染
	mSphereObject->mTransform->localScale = XMFLOAT3(1.0f, 1.0f, 1.0f);


	//平行光源球
	mSphereObject->mTransform->localPosition = GLightManager->GetMainLight()->GetLightDirection();
	mSphereObject->Render(MaterialType::DEPTH_BUFFER);


	//原点球
	mSphereObject->mTransform->localPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSphereObject->Render(MaterialType::DEPTH_BUFFER);


	//恢复默认的RS
	GDirectxCore->TurnOnSolidRender();

}

void GraphicsClass::RenderSSRBufferPass()
{
	ID3D11DepthStencilView* backDSV = mGeometryBuffer->GetDSV();
	mSSRBuffer->SetRenderTarget(backDSV);
	GDirectxCore->TurnOnEnableReflectDSS();
	ID3D11ShaderResourceView* arraySRV[2];
	arraySRV[0] = mGeometryBuffer->GetGBufferSRV(GBufferType::Pos);
	arraySRV[1] = mGeometryBuffer->GetGBufferSRV(GBufferType::Normal);
	GShaderManager->SetSSRGBufferShader(arraySRV);
	mQuad->Render();
	GDirectxCore->RecoverDefaultDSS();
	GDirectxCore->SetBackBufferRender();

}

void  GraphicsClass::RenderSSR()
{
	RenderSSRBufferPass();
	RenderSSRPass();
}


void GraphicsClass::InitDebugConsole()
{
	AllocConsole();
	FILE* pf;
	freopen_s(&pf, "CONOUT$", "w", stdout);   // 重定向输出
}


void GraphicsClass::CloseDebugConsole()
{
	FreeConsole();
}
