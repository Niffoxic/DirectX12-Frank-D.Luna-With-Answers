cbuffer cbPerObject: register(b0)
{
	float4x4 gWorldViewProjectionMatrix;
	float gTime;
	float gAnimate; //~ 0 or 1 only
	float ColorAnimation;
	float ApplyClipping;
	float4 PulseColor;
	float ApplyPulse;
	float3 padding;
};

struct VSInput
{
	float3 position	: POSITION;
	float4 color	: COLOR;
};

struct VSOutput
{
	float4 position	: SV_POSITION;
	float4 color	: COLOR;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	float3 add = input.position;
	add.xy += 0.5f * sin(add.x) * sin(3.0f * gTime);
	add.z *= 0.6f + 0.4f * sin(2.f * gTime);
	add *= gAnimate;

	float3 position = ((1.f - gAnimate) * input.position) + gAnimate * add;

	output.position = mul(float4(position, 1.0), gWorldViewProjectionMatrix);
	output.color = input.color;
	return output;
}
