cbuffer cbPerObject: register(b0)
{
	float4x4 gWorldViewProjectionMatrix;
	float gTime;
	float gAnimate; //~ 0 or 1 only
	float ColorAnimation; //~ 0 or 1 only
	float ApplyClipping;
	float4 PulseColor;
	float ApplyPulse;
	float3 padding;
};

struct PSInput
{
	float4 position	: SV_POSITION;
	float4 color	: COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    // Answer 14 : Color animation
    float animScale = sin(gTime) * 0.5f + 1.0f; // [0.5 .. 1.5]
    float3 stepped  = step(0.35f, input.color.xyz * animScale);

    float3 answer = lerp(input.color.xyz, stepped, ColorAnimation);

    // Base final color (before pulse)
    float4 baseColor = float4(max(answer, float3(0.15f, 0.25f, 0.35f)), input.color.w);

    // Answer 15 : Apply clipping
    float clipTest = baseColor.r - 0.5f; // discard if < 0
    clipTest = lerp(1.0f, clipTest, step(0.5f, ApplyClipping));
    clip(clipTest);

    // Answer 16 : Pulsing
    const float pi = 3.14159f;
    float pulseT = 0.5f * sin(2.0f * gTime - 0.25f * pi) + 0.5f; // [0..1]

    float4 pulsedColor = lerp(baseColor, PulseColor, pulseT);
    float4 finalColor  = lerp(baseColor, pulsedColor, ApplyPulse);

    return finalColor;
}
