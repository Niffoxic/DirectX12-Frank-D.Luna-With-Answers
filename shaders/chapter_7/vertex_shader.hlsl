cbuffer cbPerObject : register(b0)
{
float4x4 gWorld;
};

cbuffer cbPass : register(b1)
{
float4x4 gView;
float4x4 gInvView;
float4x4 gProj;
float4x4 gInvProj;
float4x4 gViewProj;
float4x4 gInvViewProj;
float3 gEyePosW;
float cbPerObjectPad1;
float2 gRenderTargetSize;
float2 gInvRenderTargetSize;
float gNearZ;
float gFarZ;
float gTotalTime;
float gDeltaTime;
};

struct VSInput
{
	float3 position	: POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
	float2 uv		: TEXCOORD;
	float3 color	: COLOR;
};

struct VSOutput
{
    float4 position     : SV_POSITION;
    float3 worldPos     : POSITION;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
    float2 uv           : TEXCOORD;
    float3 color        : COLOR;
};

VSOutput main(VSInput input)
{
 VSOutput output;

    float4 posW = mul(float4(input.position, 1.0f), gWorld);
    output.worldPos = posW.xyz;

    output.position = mul(posW, gViewProj);

    output.normal  = mul(input.normal,  (float3x3)gWorld);
    output.tangent = mul(input.tangent, (float3x3)gWorld);

    output.uv    = input.uv;
    output.color = input.color;

    return output;
}
