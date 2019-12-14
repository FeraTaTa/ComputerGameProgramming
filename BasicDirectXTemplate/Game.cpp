//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

float rotation = 0;
float pi = 3.14159265359f;
float toRadians = pi / 180.0f;

namespace
{
    const XMVECTORF32 START_POSITION = { 0.f, 0.f, -5.f, 0.f };
    const XMVECTORF32 ROOM_BOUNDS = { 8.f, 6.f, 12.f, 0.f };
    const float ROTATION_GAIN = 0.01f;
    const float MOVEMENT_GAIN = 0.07f;
}

Game::Game() noexcept(false) :
    m_pitch(0),
    m_yaw(0)
{
    
    m_cameraPos = START_POSITION.v;
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}
Game::~Game()
{
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }

    m_nightLoop.reset();
}
// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();


    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_EnvironmentalReverb;
        //| AudioEngine_ReverbUseFilters;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif
    m_audEngine = std::make_unique<AudioEngine>(eflags);
    //m_audEngine->SetReverb(Reverb_ConcertHall);

    m_retryAudio = false;
    if (!m_audEngine->IsAudioDevicePresent())
    {
        // we are in 'silent mode'.
    }
    m_ambient = std::make_unique<SoundEffect>(m_audEngine.get(),
        L"King Bromeliad.wav");
    m_nightLoop = m_ambient->CreateInstance(SoundEffectInstance_Use3D);
       // | SoundEffectInstance_ReverbUseFilters);
    m_nightLoop->SetVolume(30.0f);
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    if (rotation > 360) {
        rotation = 0;
    }
    rotation++;

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    float time = float(timer.GetTotalSeconds());
    //m_world = Matrix::CreateRotationZ(cosf(time) * 2.f);
    //m_world = Matrix::CreateRotationY(time);
    m_world = Matrix::Identity;

    auto mouse = m_mouse->GetState();

    if (mouse.positionMode == Mouse::MODE_RELATIVE)
    {
        Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f)
            * ROTATION_GAIN;

        m_pitch -= delta.y;
        m_yaw -= delta.x;

        // limit pitch to straight up or straight down
        // with a little fudge-factor to avoid gimbal lock
        float limit = XM_PI / 2.0f - 0.01f;
        m_pitch = std::max(-limit, m_pitch);
        m_pitch = std::min(+limit, m_pitch);

        // keep longitude in sane range by wrapping
        if (m_yaw > XM_PI)
        {
            m_yaw -= XM_PI * 2.0f;
        }
        else if (m_yaw < -XM_PI)
        {
            m_yaw += XM_PI * 2.0f;
        }
    }

    m_mouse->SetMode(mouse.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);

    auto kb = m_keyboard->GetState();
    if (kb.Escape)
    {
        ExitGame();
    }

    if (kb.Home)
    {
        m_cameraPos = START_POSITION.v;
        m_pitch = m_yaw = 0;
    }

    Vector3 move = Vector3::Zero;

    if (kb.Up || kb.W)
        move.z += 1.f;

    if (kb.Down || kb.S)
        move.z -= 1.f;

    if (kb.Left || kb.A)
        move.x += 1.f;

    if (kb.Right || kb.D)
        move.x -= 1.f;

    if (kb.PageUp || kb.Space)
        move.y += 1.f;

    if (kb.PageDown || kb.X)
        move.y -= 1.f;

    if (kb.Q) {
        rollMatrix *= Matrix::CreateRotationZ(0.2f);
    }
    if (kb.E) {
        rollMatrix *= Matrix::CreateRotationZ(-0.2f);
    }

    Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, -m_pitch, 0.f);

    move = Vector3::Transform(move, q);

    move *= MOVEMENT_GAIN;

    m_cameraPos += move;
    //TODOF CAM BOUNDS
    //Vector3 halfBound = (Vector3(ROOM_BOUNDS.v) / Vector3(2.f))
    //    - Vector3(0.1f, 0.1f, 0.1f);

    //m_cameraPos = Vector3::Min(m_cameraPos, halfBound);
    //m_cameraPos = Vector3::Max(m_cameraPos, -halfBound);
    AudioListener listener;
    listener.SetPosition(m_cameraPos);

    AudioEmitter emitter;
    emitter.SetPosition(Vector3(0,0,0));
    m_nightLoop->Apply3D(listener, emitter,false);
    if (m_nightLoop->GetState() != SoundState::PLAYING) {
        m_nightLoop->Play(true);
    }
    if (m_retryAudio)
    {
        m_retryAudio = false;

        if (m_audEngine->Reset())
        {
            // TODO: restart any looped sounds here
            if (m_nightLoop)
                m_nightLoop->Play(true);
        }
    }
    else if (!m_audEngine->Update())
    {
        if (m_audEngine->IsCriticalError())
        {
            m_retryAudio = true;
        }
    }
    elapsedTime;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{

    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    // TODO: Add your rendering code here.
    m_world = Matrix::Identity;
    m_world *= rollMatrix;

    float y = sinf(m_pitch);
    float r = cosf(m_pitch);
    float z = r * cosf(m_yaw);
    float x = r * sinf(m_yaw);

    XMVECTOR lookAt = m_cameraPos + Vector3(x, y, z);

    XMMATRIX view = XMMatrixLookAtRH(m_cameraPos, lookAt, Vector3::Up);
    //m_room->Draw(m_world, view, m_proj, Colors::White, m_roomTex.Get()); 
    m_world = Matrix::Identity;
    //sun draw
    m_world *= Matrix::CreateRotationY(rotation * toRadians);
    m_effectSun->SetMatrices(m_world, view, m_proj);
    m_effectSun->Apply(context);
    m_shape->Draw(m_effectSun.get(), m_inputLayout.Get());
    //m_world = Matrix::Identity;
    ////3D shape ball white orbiting draw
    //earth draw
    m_world *= Matrix::CreateTranslation(5.0f, .0f, .0f);
    m_world *= Matrix::CreateRotationY(rotation * toRadians);
    m_effect->SetMatrices(m_world, view, m_proj);
    m_shape->Draw(m_effect.get(), m_inputLayout.Get());
    //m_shape->Draw(m_world, view, m_proj, Colors::White, m_texture.Get());
    
    //Move and rotate the ball
    m_world *= Matrix::CreateTranslation(2.0f, 0.0f, 0.0f);
    m_world *= Matrix::CreateRotationZ(rotation * toRadians);// *Matrix::CreateRotationY(rotation * toRadians);
    m_effectAsteroid->SetMatrices(m_world, view, m_proj);
    //m_effectAsteroid->Apply(context);
    m_shape->Draw(m_effectAsteroid.get(), m_inputLayout.Get());
    m_world = Matrix::Identity;

    //ship draw
    m_world *= Matrix::CreateScale(0.0005f);
    m_world *= Matrix::CreateTranslation(0.0f, -1.0f, 1.0f);
    m_world *= Matrix::CreateRotationY(45.f * toRadians);
    //m_world *= Matrix::CreateRotationZ(45.f * toRadians);
    //m_world *= Matrix::CreateRotationX(15.0f * toRadians);
    //m_model->UpdateEffects(m_effect);


    m_model->Draw(context, *m_states, m_world, m_view, m_proj);
    m_world = Matrix::Identity;

    RenderAimReticle(context);
    context;

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::Black);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
    m_audEngine->Suspend();
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
    m_audEngine->Resume();
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1600;
    height = 900;
}
float Game::GetRotation() const
{
    return rotation;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto size = m_deviceResources->GetOutputSize();
    // TODO: Initialize device dependent objects here (independent of window size).
    //adding a model of motorbike
    DualEffect = std::make_unique<DualTextureEffect>(device);
    DualEffect->SetVertexColorEnabled(true);
    fxFactory = std::make_unique<EffectFactory>(device);
    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    PBReffect = std::make_unique<PBREffect>(device);
    PBRfxFactory = std::make_unique<PBREffectFactory>(device);
    PBReffect->SetLightEnabled(0, true);
    //PBRfxFactory->
    //iEffect = std::make_unique<IEffect>(device);
    EffectFactory::EffectInfo info;
    info.name = L"testball";
    info.alpha = 1.f;
    info.ambientColor = XMFLOAT3(0.2f, 0.2f, 0.2f);
    info.diffuseColor = XMFLOAT3(0.8f, 0.8f, 0.8f);

    //DirectX::IEffect& effectBuilt = fxFactory->CreateEffect(info, context);
  
    //effect setup
    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetTextureEnabled(true);
    m_effect->SetPerPixelLighting(true);
    m_effect->SetLightingEnabled(true);
    m_effect->SetLightEnabled(0, true);
    m_effect->SetLightDiffuseColor(0, Colors::White);
    m_effect->SetLightDirection(0, Vector3::UnitX);

    m_effectSun = std::make_unique<BasicEffect>(device);
    m_effectSun->SetTextureEnabled(true);
    m_effectSun->SetPerPixelLighting(true);
    m_effectSun->SetLightingEnabled(true);
    m_effectSun->SetLightEnabled(0, true);
    m_effectSun->SetLightDiffuseColor(0, Colors::Orange);
    m_effectSun->SetLightDirection(0, Vector3(1,1,1));

    m_effectAsteroid = std::make_unique<BasicEffect>(device);
    m_effectAsteroid->SetTextureEnabled(true);
    m_effectAsteroid->SetPerPixelLighting(true);
    m_effectAsteroid->SetLightingEnabled(true);
    m_effectAsteroid->SetLightEnabled(0, true);
    m_effectAsteroid->SetLightDiffuseColor(0, Colors::White);
    m_effectAsteroid->SetLightDirection(0, Vector3::UnitX);

    
    //m_fxFactory->CreateTexture(L"Sun/Sun_Mesh_BaseColor.png", nullptr, m_sunTex.ReleaseAndGetAddressOf());
    //m_effect->SetTexture(m_sunTex.Get());
    //effect->SetIBLTextures(m_sunTex.ReleaseAndGetAddressOf(),0, nullptr );
    //m_model = Model::CreateFromSDKMESH(device, L"Sun/Planet.sdkmesh", *m_fxFactory);
    //m_model = Model::CreateFromSDKMESH(device, L"Futuristic-Bike.sdkmesh", *m_fxFactory);
    //m_model->UpdateEffects(&effectBuilt);
    m_model = Model::CreateFromSDKMESH(device, L"Spaceship/ND Spaceship.sdkmesh", *m_fxFactory);

    //DX::ThrowIfFailed(
    //    CreateWICTextureFromFile(device, L"Sun/Sun_Mesh_Emissive.png", nullptr,
    //        m_texture.ReleaseAndGetAddressOf()));
    //effect->SetEmissiveTexture(m_texture.Get());
    //DX::ThrowIfFailed(
    //    CreateWICTextureFromFile(device, L"Sun/Sun_Mesh_BaseColor.png", nullptr,
    //        m_texture.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"Sun/Sun_Mesh_BaseColor.png", nullptr,
            m_textureSun.ReleaseAndGetAddressOf()));
    m_effectSun->SetTexture(m_textureSun.Get());

    //effect->SET(m_texture.Get());
    //m_model = Model::CreateFromSDKMESH(device, L"Sun/Sun.sdkmesh", *m_fxFactory);

    //m_model = Model::CreateFromSDKMESH(device, L"Sun/Planet.sdkmesh", *m_fxFactory);
    //DX::ThrowIfFailed(
    //    CreateWICTextureFromFile(device, L"Sun/Sun_Mesh_BaseColor.png", nullptr,
    //        m_texture.ReleaseAndGetAddressOf()));
    //m_shape->CreateInputLayout(m_effectSun.get(),
    //    m_inputLayout.ReleaseAndGetAddressOf());
    
    //DualEffect->SetTexture(m_texture.Get());
    //DualEffect->SetTexture2(m_texture.Get());

    //m_effect->SetTexture(m_texture.Get());
    //m_model->UpdateEffects((Ieff) effect);
    m_world = Matrix::Identity;

    //3D shape ball with lighting


    m_shape = GeometricPrimitive::CreateSphere(context);
    m_shape->CreateInputLayout(m_effect.get(),
        m_inputLayout.ReleaseAndGetAddressOf());

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"(1) Planet_Mesh_BaseColor.png", nullptr,
            m_texture.ReleaseAndGetAddressOf()));
    m_effect->SetTexture(m_texture.Get());

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"(2) Planet_Mesh_BaseColor.png", nullptr,
            m_textureAsteroid.ReleaseAndGetAddressOf()));
    m_effectAsteroid->SetTexture(m_textureAsteroid.Get());
    //room
    m_room = GeometricPrimitive::CreateBox(context,
        XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]),
        false, true);

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"roomtexture.dds",
            nullptr, m_roomTex.ReleaseAndGetAddressOf()));



    //AimReticleCreateBatch();
    m_world = Matrix::Identity;

    device;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    rollMatrix = Matrix::Identity;

    //adding motorbike model
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f),
        Vector3::Zero, Vector3::UnitY);


    m_proj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(70.f),
        float(size.right) / float(size.bottom), 0.01f, 100.f);
    widthWin = size.right / 2;
    heightWin = size.bottom / 2;
    AimReticleCreateBatch();
    //ball lighting
    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    
    m_shape.reset(); //3D shapes
    m_texture.Reset();
    m_textureSun.Reset();

    m_states.reset();
    m_fxFactory.reset();
    m_model.reset();

    m_room.reset();
    m_roomTex.Reset();
    m_batch.reset();

    m_effect.reset();
    m_inputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
void Game::RenderAimReticle(ID3D11DeviceContext1* context) {
    // Render polygon for aim reticle
    context->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullNone());

    mReticle_effect->Apply(context);

    context->IASetInputLayout(m_ReticleInputLayout.Get());



    std::vector<VertexPositionColor> aimReticlePoints = {
        //Triangle1
        VertexPositionColor(Vector3(widthWin / 2, heightWin / 2 - 20.f, 0.5f), Colors::Red),
        VertexPositionColor(Vector3(widthWin / 2 - 15.f, heightWin / 2 - 40.f, 0.5f), Colors::Green),
        VertexPositionColor(Vector3(widthWin / 2 + 15.f, heightWin / 2 - 40.f, 0.5f), Colors::Blue),

        //Triangle2
        VertexPositionColor(Vector3(widthWin / 2 + 20.f, heightWin / 2, 0.5f), Colors::Red),
        VertexPositionColor(Vector3(widthWin / 2 + 40.f, heightWin / 2 - 15.f, 0.5f), Colors::Green),
        VertexPositionColor(Vector3(widthWin / 2 + 40.f, heightWin / 2 + 15.f, 0.5f), Colors::Blue),

        //Triangle3
        VertexPositionColor(Vector3(widthWin / 2, heightWin / 2 + 20.f, 0.5f), Colors::Red),
        VertexPositionColor(Vector3(widthWin / 2 + 15.f, heightWin / 2 + 40.f, 0.5f), Colors::Green),
        VertexPositionColor(Vector3(widthWin / 2 - 15.f, heightWin / 2 + 40.f, 0.5f), Colors::Blue),

        //Triangle4
        VertexPositionColor(Vector3(widthWin / 2 - 20.f, heightWin / 2, 0.5f), Colors::Red),
        VertexPositionColor(Vector3(widthWin / 2 - 40.f, heightWin / 2 + 15.f, 0.5f), Colors::Green),
        VertexPositionColor(Vector3(widthWin / 2 - 40.f, heightWin / 2 - 15.f, 0.5f), Colors::Blue)
    };

    // Loop through 3 at a time to create reticle
    m_batch->Begin();
    for (std::vector<VertexPositionColor>::size_type i = 0; i != aimReticlePoints.size(); i += 3)
    {
        m_batch->DrawTriangle(aimReticlePoints[i], aimReticlePoints[i + 1], aimReticlePoints[i + 2]);
    }
    m_batch->End();

    // End render polygon
}
void Game::AimReticleCreateBatch() {
    auto device = m_deviceResources->GetD3DDevice();

    // For initializing state and effects for Triangle Render
    mReticle_effect = std::make_unique<BasicEffect>(device);
    mReticle_effect->SetVertexColorEnabled(true);

    void const* shaderByteCode;
    size_t byteCodeLength;

    mReticle_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(VertexPositionColor::InputElements,
        VertexPositionColor::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_ReticleInputLayout.ReleaseAndGetAddressOf()));

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_deviceResources->GetD3DDeviceContext());

    auto screenSize = m_deviceResources->GetOutputSize();
    float width = screenSize.right / 2;
    float height = screenSize.bottom / 2;

    Matrix proj = Matrix::CreateScale(2.0f / widthWin, -2.0f / heightWin, 1.0f) * Matrix::CreateTranslation(-1.0f, 1.0f, 0.0f);
    mReticle_effect->SetProjection(proj);
    // End initializing state and effects for Triangle Render
}
#pragma endregion
