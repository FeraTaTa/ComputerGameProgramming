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
    void PostProcess();
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

    //std::unique_ptr<DirectX::DualTextureEffect> DualEffect;
    std::unique_ptr<DirectX::EffectFactory> m_fxFactory;


    //drawing a model
    std::unique_ptr<DirectX::Model> ship_model;

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
    //Text
    std::unique_ptr<DirectX::SpriteFont> m_font;
    DirectX::SimpleMath::Vector2 m_fontPos;
    // bloom variables
    //std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    ////std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_background;
    //DirectX::SimpleMath::Matrix m_world;
    //DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_projection;
    RECT m_fullscreenRect;

    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_bloomExtractPS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_bloomCombinePS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_gaussianBlurPS;

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_bloomParams;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurParamsWidth;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurParamsHeight;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_backBuffer;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_sceneTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneSRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_sceneRT;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rt1SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rt1RT;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rt2SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rt2RT;

    RECT m_bloomRect;

};
