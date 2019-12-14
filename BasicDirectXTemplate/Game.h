//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
    float GetRotation() const;


    void OnNewAudioDevice() { m_retryAudio = true; }
private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void RenderAimReticle(ID3D11DeviceContext1* context);
    void AimReticleCreateBatch();
    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;
    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;

    std::unique_ptr<DirectX::GeometricPrimitive> m_room;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_roomTex;
    DirectX::SimpleMath::Matrix m_proj;
    DirectX::SimpleMath::Vector3 m_cameraPos;
    float m_pitch;
    float m_yaw;
    float widthWin;
    float heightWin;

    //3D shapes tutorial
    DirectX::SimpleMath::Matrix m_world;
    DirectX::SimpleMath::Matrix m_view;
    std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
    std::unique_ptr<DirectX::BasicEffect> m_effect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sunTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureSun;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureAsteroid;
    std::unique_ptr<DirectX::BasicEffect> m_effectSun;
    std::unique_ptr<DirectX::BasicEffect> m_effectAsteroid;

    std::unique_ptr<DirectX::PBREffect> PBReffect;
    std::unique_ptr<DirectX::PBREffectFactory> PBRfxFactory;

    std::unique_ptr<DirectX::DualTextureEffect> DualEffect;
    std::unique_ptr<DirectX::EffectFactory> fxFactory;


    //drawing a model
    std::unique_ptr<DirectX::IEffectFactory> m_fxFactory;
    std::unique_ptr<DirectX::Model> m_model;

    //roll matrix
    DirectX::SimpleMath::Matrix rollMatrix;

    // For rendering aim reticle
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
    std::unique_ptr<DirectX::BasicEffect> mReticle_effect;
    std::unique_ptr<DirectX::CommonStates> m_states;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_ReticleInputLayout;

    //audio setup
    std::unique_ptr<DirectX::AudioEngine> m_audEngine;
    bool m_retryAudio;
    std::unique_ptr<DirectX::SoundEffect> m_ambient;
    std::unique_ptr<DirectX::SoundEffectInstance> m_nightLoop;
};
