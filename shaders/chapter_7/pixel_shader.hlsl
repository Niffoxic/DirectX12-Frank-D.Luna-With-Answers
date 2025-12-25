
struct PSInput
{
    float4 position     : SV_POSITION;
    float3 worldPos     : POSITION;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
    float2 uv           : TEXCOORD;
    float3 color        : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
	return float4(input.color, 1.f);
}
