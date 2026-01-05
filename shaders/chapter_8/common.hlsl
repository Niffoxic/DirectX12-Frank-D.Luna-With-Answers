#ifndef FOX_COMMON_HLSL
#define FOX_COMMON_HLSL

// ============================================================
// Config
// ============================================================
#ifndef MAX_LIGHTS
#define MAX_LIGHTS 16
#endif

// ============================================================
// Per-object / Pass / Material
// ============================================================

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot: start
    float3 Direction;
    float FalloffEnd;
    // dir/spot: direction, point/spot: end
    float3 Position;
    float SpotPower;    // point/spot: position, spot: power
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;

    float3   gEyePosW;
    float    cbPerObjectPad1;

    float2   gRenderTargetSize;
    float2   gInvRenderTargetSize;

    float    gNearZ;
    float    gFarZ;
    float    gTotalTime;
    float    gDeltaTime;

    float4   gAmbientLight;
    uint     gNumDirLights;
    uint     gNumPointLights;
    uint     gNumSpotLights;
    uint     cbPassPad2;

    Light    gLights[MAX_LIGHTS];
};

struct MaterialConstants
{
    float4   DiffuseAlbedo;
    float3   FresnelR0;
    float    Roughness;     // 0..1
    float4x4 MatTransform;

    float IsAlbedoAttached;
    float IsNormalAttached; // 0 means no 1 means yes
    float IsARMAttached;
    float IsDisplacementAttached;

    float UseLinearWrap;
    float UseLinearClamp;
    float UsePointClamp;
    float UseAnisoWrap;
    float UseEnvMap;
    float3 padding;
};

cbuffer cbMaterial : register(b2)
{
    MaterialConstants gMat;
};

// ============================================================
// Material helper struct for lighting code
// ============================================================
struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float  Shininess; // we derive from roughness
};

// ============================================================
// Lighting helpers
// ============================================================

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    float denom = max(falloffEnd - falloffStart, 1e-4f);
    return saturate((falloffEnd - d) / denom);
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    // computed dot(normal, lightVec).
    float cosIncidentAngle = saturate(dot(normal, lightVec));
    float f0 = 1.0f - cosIncidentAngle;
    return R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    // Ensure unit vectors
    float3 N = normalize(normal);
    float3 L = normalize(lightVec);
    float3 V = normalize(toEye);

    // Half vector
    float3 H = normalize(V + L);

    float m = mat.Shininess * 256.0f;

    // Specular
    float ndoth = max(dot(H, N), 0.0f);
    float roughnessFactor = (m + 8.0f) * pow(ndoth, m) / 8.0f;

    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, H, L);
    float3 specAlbedo = fresnelFactor * roughnessFactor;

    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    // Return lit contribution (diffuse + spec) * incoming light
    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // Light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    float ndotl = max(dot(normalize(lightVec), normalize(normal)), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = L.Position - pos;
    float d = length(lightVec);

    if (d > L.FalloffEnd)
        return 0.0f;

    lightVec /= max(d, 1e-4f);

    float ndotl = max(dot(lightVec, normalize(normal)), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = L.Position - pos;
    float d = length(lightVec);

    if (d > L.FalloffEnd)
        return 0.0f;

    lightVec /= max(d, 1e-4f);

    float ndotl = max(dot(lightVec, normalize(normal)), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Spotlight cone factor
    float spotFactor = pow(max(dot(-lightVec, normalize(L.Direction)), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Material mat, float3 pos, float3 normal, float3 toEye, float3 shadowFactor)
{
    float3 result = 0.0f;

    uint i = 0;

    // Directionals first
    [loop]
    for (i = 0; i < gNumDirLights && i < MAX_LIGHTS; ++i)
    {
        // shadowFactor expected to have at least gNumDirLights entries.
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }

    // Points next
    uint startP = gNumDirLights;
    uint endP = min(startP + gNumPointLights, (uint)MAX_LIGHTS);

    [loop]
    for (i = startP; i < endP; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }

    // Spots last
    uint startS = endP;
    uint endS = min(startS + gNumSpotLights, (uint)MAX_LIGHTS);

    [loop]
    for (i = startS; i < endS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }

    return float4(result, 0.0f);
}

#endif // FOX_COMMON_HLSL
