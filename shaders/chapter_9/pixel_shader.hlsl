#include "../chapter_8/common.hlsl"

Texture2D gTex0   : register(t0);
Texture2D gNormal : register(t1);
Texture2D gARM    : register(t2);

SamplerState gStaticSampler_LinearWrap  : register(s0);
SamplerState gStaticSampler_LinearClamp : register(s1);
SamplerState gStaticSampler_PointClamp  : register(s2);
SamplerState gStaticSampler_AnisoWrap   : register(s3);
SamplerState gStaticSampler_EnvMap      : register(s4);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
    float3 color    : COLOR;
};

float3 UnpackNormal(float4 n)
{
    return normalize(n.xyz * 2.0f - 1.0f);
}

float4 SampleTex(Texture2D tex, float2 uv)
{
    if (gMat.UseLinearWrap  >= 0.5f) return tex.Sample(gStaticSampler_LinearWrap,  uv);
    if (gMat.UseLinearClamp >= 0.5f) return tex.Sample(gStaticSampler_LinearClamp, uv);
    if (gMat.UsePointClamp  >= 0.5f) return tex.Sample(gStaticSampler_PointClamp,  uv);
    if (gMat.UseAnisoWrap   >= 0.5f) return tex.Sample(gStaticSampler_AnisoWrap,   uv);
    if (gMat.UseEnvMap      >= 0.5f) return tex.Sample(gStaticSampler_EnvMap,      uv);
    return tex.Sample(gStaticSampler_LinearWrap, uv);
}

float2 TransformUV(float2 uv)
{
    float4 t = mul(gMat.MatTransform, float4(uv, 0.0f, 1.0f));
    return t.xy;
}

float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(cross(N, T));
    float3 toEyeW = normalize(gEyePosW - input.worldPos);

    float2 uv = TransformUV(input.uv);

    float3 baseRgb = float3(1.0f, 1.0f, 1.0f);
    if (gMat.IsAlbedoAttached >= 0.5f)
        baseRgb = SampleTex(gTex0, uv).rgb;

    float4 matAlbedo = float4(baseRgb, 1.0f) * gMat.DiffuseAlbedo;

    if (gMat.IsNormalAttached >= 0.5f)
    {
        float3 nTS = UnpackNormal(SampleTex(gNormal, uv));
        float3x3 TBN = float3x3(T, B, N);
        N = normalize(mul(nTS, TBN));
    }

    float ao = 1.0f;
    float rough = gMat.Roughness;
    float metal = 0.0f;

    if (gMat.IsARMAttached >= 0.5f)
    {
        float3 arm = SampleTex(gARM, uv).rgb;
        ao = arm.r;
        rough = arm.g;
        metal = arm.b;
    }

    float4 ambient = gAmbientLight * matAlbedo * ao;

    float shininess = 1.0f - saturate(rough);

    Material mat;
    mat.DiffuseAlbedo = matAlbedo;
    mat.FresnelR0 = lerp(gMat.FresnelR0, matAlbedo.rgb, saturate(metal));
    mat.Shininess = shininess;

    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(mat, input.worldPos, N, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    litColor.a = matAlbedo.a;

    return litColor;
}
