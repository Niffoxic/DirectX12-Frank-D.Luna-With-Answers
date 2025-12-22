//
// Created by Niffoxic (Aka Harsh Dubey) on 12/20/2025.
//
// -----------------------------------------------------------------------------
// Project   : DirectX12
// Purpose   : Academic and self-learning computer graphics project.
// Codebase  : DirectX 12 implementation based on the Luna Graphics Programming
//             textbook. This repository includes practical scene-style
//             implementations as well as answers and solutions to the
//             end-of-chapter questions for learning and reference purposes.
// License   : MIT License
// -----------------------------------------------------------------------------
//
// Copyright (c) 2025 Niffoxic
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------------

#ifndef DIRECTX12_SCENE_CHAPTER_6_H
#define DIRECTX12_SCENE_CHAPTER_6_H

#include "interface_scene.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::PackedVector::XMCOLOR Color;

	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetElementLayout()
	{
		return{
			{
				"POSITION",
				0u,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0u,
				0u,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				0u
			},
			{
				"COLOR",
				0u,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				0u,
				12u,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				0u
			}
		};
	}
};

struct Vertex_Answer1
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex0;
	DirectX::XMFLOAT2 Tex1;
	DirectX::PackedVector::XMCOLOR Color;

	static std::vector<D3D12_INPUT_ELEMENT_DESC> GetElementLayout()
	{
		return
		{
			//~ position
			{
				"POSITION",
				0u,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0u,
				0u,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				0u
			},
			//~ Tangent
			{
				"TANGENT",
				0u,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0u,
				12u,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				0u
			},
			//~ normal
			{
				"NORMAL",
				0u,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0u,
				24u,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				0u
			},
			//~ Tex0
			{
			"TEXCOORD0",
			0u,
			DXGI_FORMAT_R32G32_FLOAT,
			0u,
			36u,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0u
			},
			//~ Tex1
		{
			"TEXCOORD1",
			0u,
			DXGI_FORMAT_R32G32_FLOAT,
			0u,
			44u,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0u
			},
		{
			"COLOR",
			0u,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			0u,
			52u,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0u
			}
		};
	}
};

struct VPosData
{
	DirectX::XMFLOAT3 Position;

	static D3D12_INPUT_ELEMENT_DESC GetElementLayout() //~ At slot 0
	{
		return {
			"POSITION",
			0u,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0u,
			0u,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0u
		};
	}
};

struct VColorData
{
	DirectX::XMFLOAT4 Color;

	static D3D12_INPUT_ELEMENT_DESC GetElementLayout() //~ At slot 1
	{
		return
		{
			"COLOR",
			0u,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			1u,
			0u,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0u
		};
	}
};

struct Constants
{
	DirectX::XMFLOAT4X4 WorldViewProjection;
	float Time;
	float Animate;
	float ColorAnimation;
	float ApplyClipping;
	DirectX::XMFLOAT4 PulseColor;
	float ApplyPulse;
	float padding[3];
};

enum class EPrimitiveMode : std::uint8_t
{
    PointList      = 0,
    LineStrip      = 1,
    LineList       = 2,
    TriangleStrip  = 3,
    TriangleList   = 4,
};

inline static D3D_PRIMITIVE_TOPOLOGY GetTopologyType(const EPrimitiveMode mode)
{
    switch (mode)
    {
    case EPrimitiveMode::PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case EPrimitiveMode::LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case EPrimitiveMode::LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case EPrimitiveMode::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case EPrimitiveMode::TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    default:                           return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

inline static std::string ToString(const EPrimitiveMode mode)
{
    switch (mode)
    {
    case EPrimitiveMode::PointList:     return "PointList";
    case EPrimitiveMode::LineStrip:     return "LineStrip";
    case EPrimitiveMode::LineList:      return "LineList";
    case EPrimitiveMode::TriangleStrip: return "TriangleStrip";
    case EPrimitiveMode::TriangleList:  return "TriangleList";
    default:                           return "Unknown";
    }
}

enum class EFillMode : std::uint8_t
{
    WireFrame = 0,
    Solid,
};

inline static D3D12_FILL_MODE GetFillMode(const EFillMode mode)
{
    switch (mode)
    {
    case EFillMode::WireFrame: return D3D12_FILL_MODE_WIREFRAME;
    case EFillMode::Solid:     return D3D12_FILL_MODE_SOLID;
    default:                  return D3D12_FILL_MODE_SOLID;
    }
}

inline static std::string ToString(const EFillMode mode)
{
    switch (mode)
    {
    case EFillMode::WireFrame: return "WireFrame";
    case EFillMode::Solid:     return "Solid";
    default:                  return "Unknown";
    }
}

enum class ECullMode : std::uint8_t
{
    None,
    Front,
    Back
};

inline static D3D12_CULL_MODE GetCullMode(const ECullMode mode)
{
    switch (mode)
    {
    case ECullMode::None:  return D3D12_CULL_MODE_NONE;
    case ECullMode::Front: return D3D12_CULL_MODE_FRONT;
    case ECullMode::Back:  return D3D12_CULL_MODE_BACK;
    default:               return D3D12_CULL_MODE_BACK;
    }
}

inline static std::string ToString(const ECullMode mode)
{
    switch (mode)
    {
    case ECullMode::None:  return "None";
    case ECullMode::Front: return "Front";
    case ECullMode::Back:  return "Back";
    default:               return "Unknown";
    }
}

class SceneChapter6 final: public IScene
{
public:
	 explicit SceneChapter6(framework::DxRenderManager& renderer);
	~SceneChapter6() override;

	bool Initialize() override;
	void Shutdown  () override;

	void FrameBegin	  (float deltaTime) override;
	void FrameEnd	  (float deltaTime) override;
	void ImguiView	  (float deltaTime) override;

private:
	//~ Create resources
	void CreateVertexBuffer	 ();
	void CreateIndexBuffer	 ();
	void CreateResourceHeap  ();
	void CreateConstantBuffer();
	void CreateRootSignature ();
	void CreateShaders		 ();
	void CreatePipelineState ();

	//~ Create Resources for answer 2
	void CreatePipelineState_answer_2();
	void CreateGeometry_answer_2	 ();

	void LoadData();
	void SaveData() const;

	void UpdateConstantBuffer(float deltaTime);

	//~ Draws
	void DrawMainCubes() const;

private:
	HANDLE m_waitEvent;

	//~ Vertex Buffer
	bool m_bVertexBufferInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexView{};

	//~ Index Buffer
	bool m_bIndexBufferInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadIndex;
	D3D12_INDEX_BUFFER_VIEW m_indexView{};

	//~ Create CSV heap
	bool m_bResourceHeapInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_resourceHeap;

	//~ Constant buffer
	bool m_bConstantBufferInitialized{ false };
	std::uint16_t m_constantsNumbers{ 3u };
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_constantBufferView{};
	void* m_mapped{ nullptr };

	//~ Root Signature
	bool m_bRootSignatureInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

	//~ Shaders
	bool m_bShadersInitialized{ false };
	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;

	//~ Pipeline state
	bool m_bPipelineInitialized{ false };
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	// ================= Answer 2 ============================
	bool m_drawAnswer2{ false };
	//~ Pipeline state with different input slots
	bool m_bPipeline2Initialized{ false };
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline2State;

	//~ Create Vertices (one for geometry and one of color)
	bool m_bAnswer2_Initialized{ false };
	Microsoft::WRL::ComPtr<ID3D12Resource>	m_answer2Buffer;
	Microsoft::WRL::ComPtr<ID3D12Resource>	m_answer2Uploader;
	D3D12_VERTEX_BUFFER_VIEW				m_geometryView{};
	D3D12_VERTEX_BUFFER_VIEW				m_colorView{};
	D3D12_CONSTANT_BUFFER_VIEW_DESC			m_answer2_CBV{};
	D3D12_CPU_DESCRIPTOR_HANDLE			    m_answer2_ConstantHandle;

	// ====================== Answer 3 ===================================
	bool m_drawAnswer3{ false };
	EPrimitiveMode	m_primitiveMode	{ EPrimitiveMode::TriangleList };
	ECullMode		m_cullMode		{ ECullMode::Back };
	EFillMode		m_fillMode		{ EFillMode::Solid };
	DirectX::XMFLOAT4X4 m_world_answer_2{};

	// ======================= Answer 4 ===================================
	bool m_drawAnswer4{ false };
	D3D12_VERTEX_BUFFER_VIEW		m_pyramidVertexView		{};
	D3D12_INDEX_BUFFER_VIEW			m_pyramidIndexView		{};
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_pyramidCBV			{};
	D3D12_GPU_DESCRIPTOR_HANDLE		m_pyramidGPUHandle		{};
	DirectX::XMFLOAT4X4				m_pyramidTransformation	{};

	// ==================== Answer 6 ===========================
	bool m_drawAnswer6{ false };

	// ================= Answer 12 ===========================
	D3D12_VIEWPORT	m_viewport{};

	// ================= Answer 13 ===========================
	D3D12_RECT		m_scissorRect{};

	// ================= Answer 14 ===========================
	bool m_drawAnswer14_PixelColor{ false };

	// ================= Answer 15 ===========================
	bool m_drawAnswer15_ApplyClipping{ false };

	// ================= Answer 16 ===========================
	bool m_drawAnswer16_ApplyPulsing{ false };
	DirectX::XMFLOAT4 m_pulseColor{0.24f, 0.24f, 1.f, 1.f};

	//~ configurations
	float m_totalTime{ 0.0f };
	float m_phi		 { 0.1f };
	float m_theta	 { 0.1f };
	float m_radius	 { 5.0f };
	DirectX::XMFLOAT4X4 m_view {};
	DirectX::XMFLOAT4X4 m_world{};
	DirectX::XMFLOAT4X4 m_proj {};
};

#endif //DIRECTX12_SCENE_CHAPTER_6_H
