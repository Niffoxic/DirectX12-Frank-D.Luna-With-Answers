#include "common.hlsl"

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
    float3 color    : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 toEyeW = normalize(gEyePosW - input.worldPos);

    float3 base = saturate(input.color);

    float4 matAlbedo = float4(base, 1.0f) * gMat.DiffuseAlbedo;

    float4 ambient = gAmbientLight * matAlbedo;

    // Direct
    float shininess = 1.0f - saturate(gMat.Roughness);

    Material mat;
    mat.DiffuseAlbedo = matAlbedo;
    mat.FresnelR0     = gMat.FresnelR0;
    mat.Shininess     = shininess;

    float3 shadowFactor = 1.0f;

    float4 directLight = ComputeLighting(mat, input.worldPos, N, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    litColor.a = matAlbedo.a;

    return litColor;
}
