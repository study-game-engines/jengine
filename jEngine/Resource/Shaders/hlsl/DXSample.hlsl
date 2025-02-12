#include "common.hlsl"

#ifndef USE_VERTEX_COLOR
// #define USE_VERTEX_COLOR 0
#define USE_VERTEX_COLOR 1
#endif

#ifndef USE_ALBEDO_TEXTURE
#define USE_ALBEDO_TEXTURE 0
#endif

struct VSInput
{
    float3 Position : POSITION0;
#if USE_VERTEX_COLOR
    float4 Color : COLOR0;
#endif
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float2 TexCoord : TEXCOORD0;
};

cbuffer ViewParam : register(b0, space0) { ViewUniformBuffer ViewParam; }
cbuffer RenderObjectParam : register(b1, space0) { RenderObjectUniformBuffer RenderObjectParam; }

struct VSOutput
{
    float4 Pos : SV_POSITION;
#if USE_VERTEX_COLOR
    float4 Color : COLOR0;
#endif
    float2 TexCoord : TEXCOORD0;
    float3 Normal : NORMAL0;
    float4 WorldPos : TEXCOORD1;
    float3x3 TBN : TEXCOORD2;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output = (VSOutput)0;

    output.WorldPos = mul(RenderObjectParam.M, float4(input.Position, 1.0));
    output.Pos = mul(ViewParam.VP, output.WorldPos);
#if USE_VERTEX_COLOR
    output.Color = input.Color;
#endif
    output.TexCoord = input.TexCoord;
    output.Normal = mul((float3x3)RenderObjectParam.M, input.Normal);

#if USE_ALBEDO_TEXTURE
    float4 Bitangent = float4(cross(input.Normal, input.Tangent), 0.0);
    float3 T = normalize(mul(RenderObjectParam.M, float4(input.Tangent, 0.0)).xyz);
    float3 B = normalize(mul(RenderObjectParam.M, Bitangent).xyz);
    float3 N = normalize(mul(RenderObjectParam.M, float4(input.Normal, 0.0)).xyz);
    output.TBN = float3x3(T, B, N);
    output.TBN = transpose(output.TBN);
#endif

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return input.Color;
}
