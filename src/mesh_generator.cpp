//
// Created by Niffoxic (Aka Harsh Dubey) on 12/22/2025.
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
#include "utility/mesh_generator.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace DirectX;

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    inline XMFLOAT3 Add3(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    inline XMFLOAT3 Sub3(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
    inline XMFLOAT3 Mul3(const XMFLOAT3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }

    inline void Accum3(XMFLOAT3& a, const XMFLOAT3& b) { a.x += b.x; a.y += b.y; a.z += b.z; }

    inline XMFLOAT3 Cross3(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return { a.y * b.z - a.z * b.y,
                 a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x };
    }

    inline float Dot3(const XMFLOAT3& a, const XMFLOAT3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    inline XMFLOAT3 Normalize3(const XMFLOAT3& v)
    {
        const float len2 = Dot3(v, v);
        if (len2 <= FLT_EPSILON) return { 0.f, 0.f, 0.f };
        const float inv = 1.0f / std::sqrt(len2);
        return { v.x * inv, v.y * inv, v.z * inv };
    }

    inline XMFLOAT2 Sub2(const XMFLOAT2& a, const XMFLOAT2& b) { return { a.x - b.x, a.y - b.y }; }

    inline void FlipWindingInPlace(MeshData& mesh)
    {
        auto& idx = mesh.indices;
        for (size_t i = 0; i + 2 < idx.size(); i += 3)
            std::swap(idx[i + 1], idx[i + 2]);
    }

    // Subdivide each triangle into 4 triangles (standard midpoint subdivision).
    inline void Subdivide(MeshData& mesh)
    {
        MeshData out;
        out.vertices.reserve(mesh.vertices.size() * 4);
        out.indices.reserve(mesh.indices.size() * 4);

        auto MidVertex = [](const MeshVertex& a, const MeshVertex& b) -> MeshVertex
        {
            MeshVertex m{};
            m.Position = { (a.Position.x + b.Position.x) * 0.5f,
                           (a.Position.y + b.Position.y) * 0.5f,
                           (a.Position.z + b.Position.z) * 0.5f };

            m.Normal   = Normalize3({ (a.Normal.x + b.Normal.x) * 0.5f,
                                      (a.Normal.y + b.Normal.y) * 0.5f,
                                      (a.Normal.z + b.Normal.z) * 0.5f });

            m.Tangent  = Normalize3({ (a.Tangent.x + b.Tangent.x) * 0.5f,
                                      (a.Tangent.y + b.Tangent.y) * 0.5f,
                                      (a.Tangent.z + b.Tangent.z) * 0.5f });

            m.UV       = { (a.UV.x + b.UV.x) * 0.5f, (a.UV.y + b.UV.y) * 0.5f };
            m.Color    = { (a.Color.x + b.Color.x) * 0.5f,
                           (a.Color.y + b.Color.y) * 0.5f,
                           (a.Color.z + b.Color.z) * 0.5f };
            return m;
        };

        const auto& v = mesh.vertices;
        const auto& i = mesh.indices;

        for (size_t t = 0; t + 2 < i.size(); t += 3)
        {
            MeshVertex v0 = v[i[t + 0]];
            MeshVertex v1 = v[i[t + 1]];
            MeshVertex v2 = v[i[t + 2]];

            MeshVertex m0 = MidVertex(v0, v1);
            MeshVertex m1 = MidVertex(v1, v2);
            MeshVertex m2 = MidVertex(v0, v2);

            const uint32_t base = static_cast<uint32_t>(out.vertices.size());
            out.vertices.push_back(v0); // 0
            out.vertices.push_back(v1); // 1
            out.vertices.push_back(v2); // 2
            out.vertices.push_back(m0); // 3
            out.vertices.push_back(m1); // 4
            out.vertices.push_back(m2); // 5

            // 4 new triangles:
            // v0 m0 m2
            out.indices.push_back(base + 0); out.indices.push_back(base + 3); out.indices.push_back(base + 5);
            // m0 v1 m1
            out.indices.push_back(base + 3); out.indices.push_back(base + 1); out.indices.push_back(base + 4);
            // m2 m1 v2
            out.indices.push_back(base + 5); out.indices.push_back(base + 4); out.indices.push_back(base + 2);
            // m0 m1 m2
            out.indices.push_back(base + 3); out.indices.push_back(base + 4); out.indices.push_back(base + 5);
        }

        mesh = std::move(out);
    }

    inline MeshVertex MakeVertex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT3& t, const XMFLOAT2& uv, const XMFLOAT3& c)
    {
        MeshVertex v{};
        v.Position = p;
        v.Normal   = n;
        v.Tangent  = t;
        v.UV       = uv;
        v.Color    = c;
        return v;
    }

    inline void SetInsideOut(MeshData& mesh)
    {
        FlipWindingInPlace(mesh);
        for (auto& v : mesh.vertices)
        {
            v.Normal  = Mul3(v.Normal, -1.0f);
            v.Tangent = Mul3(v.Tangent, -1.0f);
        }
    }

    inline uint32_t ClampU32(uint32_t v, uint32_t lo, uint32_t hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }
}


MeshData MeshGenerator::GenerateBox(const GenerateBoxConfig& config)
{
    MeshData mesh{};

    const XMFLOAT3 e = config.Extents; // half-size
    const XMFLOAT3 c = config.Color;

    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);

    auto AddFace = [&](XMFLOAT3 p0, XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3,
                       XMFLOAT3 n, XMFLOAT3 t)
    {
        const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(MakeVertex(p0, n, t, { 0.f, 1.f }, c));
        mesh.vertices.push_back(MakeVertex(p1, n, t, { 0.f, 0.f }, c));
        mesh.vertices.push_back(MakeVertex(p2, n, t, { 1.f, 0.f }, c));
        mesh.vertices.push_back(MakeVertex(p3, n, t, { 1.f, 1.f }, c));

        // Two triangles: (0,1,2) and (0,2,3)
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);

        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    };

    // Positions:
    const float ex = e.x, ey = e.y, ez = e.z;

    // +Z (Front)
    AddFace(
        { -ex, +ey, +ez }, { -ex, -ey, +ez }, { +ex, -ey, +ez }, { +ex, +ey, +ez },
        { 0.f, 0.f, +1.f }, { +1.f, 0.f, 0.f }
    );

    // -Z (Back)
    AddFace(
        { +ex, +ey, -ez }, { +ex, -ey, -ez }, { -ex, -ey, -ez }, { -ex, +ey, -ez },
        { 0.f, 0.f, -1.f }, { -1.f, 0.f, 0.f }
    );

    // -X (Left)
    AddFace(
        { -ex, +ey, -ez }, { -ex, -ey, -ez }, { -ex, -ey, +ez }, { -ex, +ey, +ez },
        { -1.f, 0.f, 0.f }, { 0.f, 0.f, +1.f }
    );

    // +X (Right)
    AddFace(
        { +ex, +ey, +ez }, { +ex, -ey, +ez }, { +ex, -ey, -ez }, { +ex, +ey, -ez },
        { +1.f, 0.f, 0.f }, { 0.f, 0.f, -1.f }
    );

    // +Y (Top)
    AddFace(
        { -ex, +ey, -ez }, { -ex, +ey, +ez }, { +ex, +ey, +ez }, { +ex, +ey, -ez },
        { 0.f, +1.f, 0.f }, { +1.f, 0.f, 0.f }
    );

    // -Y (Bottom)
    AddFace(
        { -ex, -ey, +ez }, { -ex, -ey, -ez }, { +ex, -ey, -ez }, { +ex, -ey, +ez },
        { 0.f, -1.f, 0.f }, { +1.f, 0.f, 0.f }
    );

    uint32_t sub = config.Subdivisions;
    sub = ClampU32(sub, 0, 6); // keep it sane
    for (uint32_t s = 0; s < sub; ++s)
        Subdivide(mesh);

    if (!config.GenerateTangents)
    {
        for (auto& v : mesh.vertices) v.Tangent = { 0.f, 0.f, 0.f };
    }

    if (config.InsideOut)
        SetInsideOut(mesh);

    if (config.FlipWinding)
        FlipWindingInPlace(mesh);

    if (config.Subdivisions > 0)
        ComputeNormals(mesh, config.InsideOut);

    if (config.GenerateTangents)
        ComputeTangents(mesh, config.InsideOut);

    return mesh;
}

MeshData MeshGenerator::GenerateMountain(const GenerateMountainConfig& config)
{
    MeshData mesh{};

    const uint32_t nx = std::max(1u, config.SubdivisionsX);
    const uint32_t nz = std::max(1u, config.SubdivisionsZ);

    const float w = std::max(FLT_EPSILON, config.Width);
    const float d = std::max(FLT_EPSILON, config.Depth);

    const float halfW = w * 0.5f;
    const float halfD = d * 0.5f;

    const DirectX::XMFLOAT3 N = Normalize3(config.Normal);

    DirectX::XMFLOAT3 up = (std::fabs(N.y) < 0.999f) ? DirectX::XMFLOAT3{ 0.f, 1.f, 0.f } : DirectX::XMFLOAT3{ 1.f, 0.f, 0.f };
    DirectX::XMFLOAT3 T = Normalize3(Cross3(up, N));
    DirectX::XMFLOAT3 B = Normalize3(Cross3(N, T));

    const uint32_t vertX = nx + 1;
    const uint32_t vertZ = nz + 1;

    mesh.vertices.resize(static_cast<size_t>(vertX) * static_cast<size_t>(vertZ));
    mesh.indices.reserve(static_cast<size_t>(nx) * static_cast<size_t>(nz) * 6);

    std::vector<float> heights;
    heights.resize(mesh.vertices.size(), 0.0f);

    auto Saturate = [](float x) -> float
    {
        return std::min(1.0f, std::max(0.0f, x));
    };

    auto Lerp3 = [&](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t) -> DirectX::XMFLOAT3
    {
        t = Saturate(t);
        return DirectX::XMFLOAT3{
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };
    };

    auto Height = [&](float u, float v, float xPos, float zPos) -> float
    {
        const float h =
            std::sin(xPos * config.Freq1) * std::cos(zPos * config.Freq1) * config.Amp1 +
            std::sin(xPos * config.Freq2 + 1.7f) * std::cos(zPos * config.Freq2 + 0.3f) * config.Amp2;

        const float cx = (u - 0.5f);
        const float cz = (v - 0.5f);
        const float falloff = std::exp(-(cx * cx + cz * cz) * config.Falloff);

        float shaped = std::copysign(std::pow(std::fabs(h), config.Harshness), h);
        return shaped * falloff * config.HeightScale;
    };

    float minH = FLT_MAX;
    float maxH = -FLT_MAX;

    for (uint32_t z = 0; z < vertZ; ++z)
    {
        const float v = static_cast<float>(z) / static_cast<float>(nz);
        const float zPos = (config.Centered ? (-halfD + v * d) : (v * d));

        for (uint32_t x = 0; x < vertX; ++x)
        {
            const float u = static_cast<float>(x) / static_cast<float>(nx);
            const float xPos = (config.Centered ? (-halfW + u * w) : (u * w));

            const float h = Height(u, v, xPos, zPos);

            const size_t idx = static_cast<size_t>(z) * vertX + x;
            heights[idx] = h;

            minH = std::min(minH, h);
            maxH = std::max(maxH, h);

            DirectX::XMFLOAT3 pos = Add3(Mul3(T, xPos), Mul3(B, zPos));
            pos = Add3(pos, Mul3(N, h));

            MeshVertex mv{};
            mv.Position = pos;
            mv.Normal   = N;
            mv.Tangent  = (config.GenerateTangents ? T : DirectX::XMFLOAT3{ 0.f, 0.f, 0.f });
            mv.UV       = { u, 1.0f - v };
            mv.Color    = DirectX::XMFLOAT3{ 1.f, 1.f, 1.f };

            mesh.vertices[idx] = mv;
        }
    }

    const float invRange = (maxH > minH) ? (1.0f / (maxH - minH)) : 0.0f;

    for (size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        const float tH = (heights[i] - minH) * invRange;

        const float groundMix = 0.5f + 0.5f * std::sin(heights[i] * 0.25f);
        const DirectX::XMFLOAT3 ground = Lerp3(config.GroundBrown, config.GroundGreen, groundMix);

        const float snowT = Saturate((tH - config.SnowStart) / std::max(FLT_EPSILON, config.SnowBlend));
        mesh.vertices[i].Color = Lerp3(ground, config.SnowColor, snowT);
    }

    for (uint32_t z = 0; z < nz; ++z)
    {
        for (uint32_t x = 0; x < nx; ++x)
        {
            const uint32_t i0 = (z * vertX) + x;
            const uint32_t i1 = (z * vertX) + (x + 1);
            const uint32_t i2 = ((z + 1) * vertX) + (x + 1);
            const uint32_t i3 = ((z + 1) * vertX) + x;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    if (config.FlipWinding)
        FlipWindingInPlace(mesh);

    ComputeNormals(mesh, config.FlipWinding);

    if (config.GenerateTangents)
        ComputeTangents(mesh, config.FlipWinding);

    return mesh;
}

MeshData MeshGenerator::GenerateSphere(const GenerateSphereConfig& config)
{
    MeshData mesh{};

    const float r = std::max(FLT_EPSILON, config.Radius);
    const uint32_t slice = std::max(3u, config.SliceCount);
    const uint32_t stack = std::max(2u, config.StackCount);
    const DirectX::XMFLOAT3 C = config.Color;

    const uint32_t ringVerts = slice + 1;
    mesh.vertices.reserve(static_cast<size_t>(ringVerts) * static_cast<size_t>(stack + 1));
    mesh.indices.reserve(static_cast<size_t>(slice) * static_cast<size_t>(stack) * 6ull);

    for (uint32_t i = 0; i <= stack; ++i)
    {
        const float v = static_cast<float>(i) / static_cast<float>(stack);      // 0..1
        const float phi = v * kPi;                                              // 0..pi

        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (uint32_t j = 0; j <= slice; ++j)
        {
            const float u = static_cast<float>(j) / static_cast<float>(slice);  // 0..1
            const float theta = u * (2.0f * kPi);                               // 0..2pi

            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            // Unit normal on sphere
            DirectX::XMFLOAT3 n = { sinPhi * cosTheta, cosPhi, sinPhi * sinTheta };

            // Position
            DirectX::XMFLOAT3 p = Mul3(n, r);

            DirectX::XMFLOAT3 t = { -sinTheta, 0.0f, cosTheta };

            MeshVertex mv{};
            mv.Position = p;

            if (config.InsideOut)
            {
                mv.Normal  = Mul3(n, -1.0f);
                mv.Tangent = (config.GenerateTangents ? Mul3(t, -1.0f) : DirectX::XMFLOAT3{0.f, 0.f, 0.f});
            }
            else
            {
                mv.Normal  = n;
                mv.Tangent = (config.GenerateTangents ? t : DirectX::XMFLOAT3{0.f, 0.f, 0.f});
            }

            mv.UV    = { u, 1.0f - v };
            mv.Color = C;

            mesh.vertices.push_back(mv);
        }
    }

    // Indices
    for (uint32_t i = 0; i < stack; ++i)
    {
        for (uint32_t j = 0; j < slice; ++j)
        {
            const uint32_t i0 = (i * ringVerts) + j;
            const uint32_t i1 = (i * ringVerts) + (j + 1);
            const uint32_t i2 = ((i + 1) * ringVerts) + (j + 1);
            const uint32_t i3 = ((i + 1) * ringVerts) + j;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    if (config.InsideOut)
    {
        FlipWindingInPlace(mesh);
    }

    if (config.FlipWinding)
    {
        FlipWindingInPlace(mesh);
    }

    return mesh;
}


MeshData MeshGenerator::GenerateCylinder(const GenerateCylinderConfig& config)
{
    MeshData mesh{};

    const float h = std::max(FLT_EPSILON, config.Height);
    const float r0 = std::max(0.0f, config.BottomRadius);
    const float r1 = std::max(0.0f, config.TopRadius);

    const uint32_t slice = std::max(3u, config.SliceCount);
    const uint32_t stack = std::max(1u, config.StackCount);

    const XMFLOAT3 C = config.Color;

    const float halfH = h * 0.5f;

    // Side surface
    const uint32_t ringVerts = slice + 1;
    const uint32_t rings = stack + 1;

    const size_t sideVertCount = static_cast<size_t>(ringVerts) * rings;
    mesh.vertices.reserve(sideVertCount + (config.CapTop ? ringVerts + 1 : 0) + (config.CapBottom ? ringVerts + 1 : 0));
    mesh.indices.reserve(static_cast<size_t>(slice) * stack * 6 + (config.CapTop ? slice * 3 : 0) + (config.CapBottom ? slice * 3 : 0));

    for (uint32_t i = 0; i <= stack; ++i)
    {
        const float v = static_cast<float>(i) / static_cast<float>(stack);
        const float y = -halfH + v * h;
        const float r = r0 + v * (r1 - r0);

        for (uint32_t j = 0; j <= slice; ++j)
        {
            const float u = static_cast<float>(j) / static_cast<float>(slice);
            const float theta = u * (2.0f * kPi);
            const float s = std::sin(theta);
            const float cth = std::cos(theta);

            XMFLOAT3 pos = { r * cth, y, r * s };

            const float dr = (r1 - r0);
            const float slope = (h > FLT_EPSILON) ? (-dr / h) : 0.0f;
            XMFLOAT3 n = Normalize3({ cth, slope, s });

            XMFLOAT3 t = Normalize3({ -s, 0.0f, cth }); // along theta

            MeshVertex mv{};
            mv.Position = pos;
            mv.Normal   = (config.InsideOut ? Mul3(n, -1.0f) : n);
            mv.Tangent  = (config.GenerateTangents ? (config.InsideOut ? Mul3(t, -1.0f) : t) : XMFLOAT3{ 0.f, 0.f, 0.f });
            mv.UV       = { u, 1.0f - v };
            mv.Color    = C;

            mesh.vertices.push_back(mv);
        }
    }

    // Side indices
    for (uint32_t i = 0; i < stack; ++i)
    {
        for (uint32_t j = 0; j < slice; ++j)
        {
            const uint32_t i0 = (i * ringVerts) + j;
            const uint32_t i1 = (i * ringVerts) + (j + 1);
            const uint32_t i2 = ((i + 1) * ringVerts) + (j + 1);
            const uint32_t i3 = ((i + 1) * ringVerts) + j;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    auto AddCap = [&](bool top)
    {
        const float y = top ? +halfH : -halfH;
        const float r = top ? r1 : r0;
        if (r <= FLT_EPSILON) return;

        const XMFLOAT3 n = top ? XMFLOAT3{ 0.f, +1.f, 0.f } : XMFLOAT3{ 0.f, -1.f, 0.f };
        const XMFLOAT3 nFinal = config.InsideOut ? Mul3(n, -1.0f) : n;

        // Tangent for cap: +X (arbitrary, but consistent)
        const XMFLOAT3 t = config.InsideOut ? XMFLOAT3{ -1.f, 0.f, 0.f } : XMFLOAT3{ +1.f, 0.f, 0.f };

        const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());

        // Center vertex
        mesh.vertices.push_back(MakeVertex({ 0.f, y, 0.f }, nFinal, (config.GenerateTangents ? t : XMFLOAT3{0,0,0}), { 0.5f, 0.5f }, C));

        // Rim
        for (uint32_t j = 0; j <= slice; ++j)
        {
            const float u = static_cast<float>(j) / static_cast<float>(slice);
            const float theta = u * (2.0f * kPi);
            const float s = std::sin(theta);
            const float cth = std::cos(theta);

            const float x = r * cth;
            const float z = r * s;

            // Map to [0,1] in disc space
            const float uDisc = (cth * 0.5f) + 0.5f;
            const float vDisc = (s * 0.5f) + 0.5f;

            mesh.vertices.push_back(MakeVertex({ x, y, z }, nFinal, (config.GenerateTangents ? t : XMFLOAT3{0,0,0}), { uDisc, 1.0f - vDisc }, C));
        }

        // Triangles: fan around center.
        for (uint32_t j = 0; j < slice; ++j)
        {
            const uint32_t center = base + 0;
            const uint32_t v0 = base + 1 + j;
            const uint32_t v1 = base + 1 + (j + 1);

            if (top)
            {
                mesh.indices.push_back(center);
                mesh.indices.push_back(v1);
                mesh.indices.push_back(v0);
            }
            else
            {
                mesh.indices.push_back(center);
                mesh.indices.push_back(v0);
                mesh.indices.push_back(v1);
            }
        }
    };

    if (config.CapTop)    AddCap(true);
    if (config.CapBottom) AddCap(false);

    if (config.InsideOut)
    {
        // Flip winding globally to match inside-out expectation.
        FlipWindingInPlace(mesh);
    }

    if (config.FlipWinding)
        FlipWindingInPlace(mesh);

    if (config.GenerateTangents)
        ComputeTangents(mesh,config.InsideOut);

    return mesh;
}

void MeshGenerator::ComputeNormals(MeshData& mesh, bool flip)
{
    auto& v = mesh.vertices;
    const auto& idx = mesh.indices;

    for (auto& vert : v)
        vert.Normal = { 0.f, 0.f, 0.f };

    for (size_t i = 0; i + 2 < idx.size(); i += 3)
    {
        const uint32_t i0 = idx[i + 0];
        const uint32_t i1 = idx[i + 1];
        const uint32_t i2 = idx[i + 2];

        const XMFLOAT3& p0 = v[i0].Position;
        const XMFLOAT3& p1 = v[i1].Position;
        const XMFLOAT3& p2 = v[i2].Position;

        const XMFLOAT3 e0 = Sub3(p1, p0);
        const XMFLOAT3 e1 = Sub3(p2, p0);

        XMFLOAT3 n = Cross3(e0, e1); // area-weighted
        if (flip) n = Mul3(n, -1.0f);

        Accum3(v[i0].Normal, n);
        Accum3(v[i1].Normal, n);
        Accum3(v[i2].Normal, n);
    }

    for (auto& vert : v)
        vert.Normal = Normalize3(vert.Normal);
}

void MeshGenerator::ComputeTangents(MeshData& mesh, bool flip)
{
    auto& v = mesh.vertices;
    const auto& idx = mesh.indices;

    // We’ll accumulate tangents and then orthonormalize against normal.
    std::vector<XMFLOAT3> tan(v.size(), { 0.f, 0.f, 0.f });

    for (size_t i = 0; i + 2 < idx.size(); i += 3)
    {
        const uint32_t i0 = idx[i + 0];
        const uint32_t i1 = idx[i + 1];
        const uint32_t i2 = idx[i + 2];

        const XMFLOAT3& p0 = v[i0].Position;
        const XMFLOAT3& p1 = v[i1].Position;
        const XMFLOAT3& p2 = v[i2].Position;

        const XMFLOAT2& w0 = v[i0].UV;
        const XMFLOAT2& w1 = v[i1].UV;
        const XMFLOAT2& w2 = v[i2].UV;

        const XMFLOAT3 e1 = Sub3(p1, p0);
        const XMFLOAT3 e2 = Sub3(p2, p0);

        const XMFLOAT2 d1 = Sub2(w1, w0);
        const XMFLOAT2 d2 = Sub2(w2, w0);

        const float denom = (d1.x * d2.y - d1.y * d2.x);
        if (std::fabs(denom) < 1e-8f)
            continue;

        const float r = 1.0f / denom;

        XMFLOAT3 t = Mul3(Sub3(Mul3(e1, d2.y), Mul3(e2, d1.y)), r);
        if (flip) t = Mul3(t, -1.0f);

        Accum3(tan[i0], t);
        Accum3(tan[i1], t);
        Accum3(tan[i2], t);
    }

    for (size_t i = 0; i < v.size(); ++i)
    {
        const XMFLOAT3 n = v[i].Normal;
        XMFLOAT3 t = tan[i];

        // Gram-Schmidt: t = normalize(t - n * dot(n,t))
        const float ndott = Dot3(n, t);
        t = Sub3(t, Mul3(n, ndott));
        t = Normalize3(t);

        // If degenerate, pick something orthogonal-ish.
        if (Dot3(t, t) <= FLT_EPSILON)
        {
            XMFLOAT3 axis = (std::fabs(n.y) < 0.999f) ? XMFLOAT3{ 0.f, 1.f, 0.f } : XMFLOAT3{ 1.f, 0.f, 0.f };
            t = Normalize3(Cross3(axis, n));
        }

        v[i].Tangent = t;
    }
}

void MeshGenerator::Transform(MeshData& mesh, DirectX::CXMMATRIX M)
{
    XMMATRIX mat = M;

    XMMATRIX invT = XMMatrixTranspose(XMMatrixInverse(nullptr, mat));

    for (auto& vert : mesh.vertices)
    {
        XMVECTOR p = XMLoadFloat3(&vert.Position);
        XMVECTOR n = XMLoadFloat3(&vert.Normal);
        XMVECTOR t = XMLoadFloat3(&vert.Tangent);

        p = XMVector3TransformCoord(p, mat);
        n = XMVector3TransformNormal(n, invT);
        t = XMVector3TransformNormal(t, invT);

        n = XMVector3Normalize(n);
        t = XMVector3Normalize(t);

        XMStoreFloat3(&vert.Position, p);
        XMStoreFloat3(&vert.Normal, n);
        XMStoreFloat3(&vert.Tangent, t);
    }
}

void MeshGenerator::Append(MeshData& dst, const MeshData& src)
{
    const uint32_t base = static_cast<uint32_t>(dst.vertices.size());

    dst.vertices.insert(dst.vertices.end(), src.vertices.begin(), src.vertices.end());

    dst.indices.reserve(dst.indices.size() + src.indices.size());
    for (uint32_t i : src.indices)
        dst.indices.push_back(base + i);
}
