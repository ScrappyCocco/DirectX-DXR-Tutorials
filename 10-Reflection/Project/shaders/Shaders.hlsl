/*
 * ----------------------------------------
 * HELPER FUNCTIONS
 * ----------------------------------------
 */

static const float3 cameraPosition = float3(0, 0, -2.6);
static const float4 backgroundColor = float4(0.4, 0.6, 0.2, 1.0);
static const float4 lightAmbientColor = float4(0.1f, 0.1f, 0.1f, 1.0);
static const float3 lightPosition = float3(2.0, 2.0, -2.0);
static const float4 lightDiffuseColor = float4(0.2, 0.2, 0.2, 1.0);
static const float4 primitiveAlbedo = float4(0.8, 0.0, 0.0, 1.0);
static const float4 groundAlbedo = float4(1.0, 1.0, 1.0, 1.0);
static const float InShadowRadiance = 0.35f;
static const uint MaxRecursionDepth = 4;
static const float diffuseCoef = 0.9;
static const float specularCoef = 0.7;
static const float specularPower = 50;
static const float reflectanceCoef = 0.9;

struct VertexPositionNormalTangentTexture
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float2 TexCoord;
};

float4 linearToSrgb(float4 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float4 sq1 = sqrt(c);
    float4 sq2 = sqrt(sq1);
    float4 sq3 = sqrt(sq2);
    float4 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Diffuse lighting calculation.
float CalculateDiffuseCoefficient(in float3 hitPosition, in float3 incidentLightRay, in float3 normal)
{
    float fNDotL = saturate(dot(-incidentLightRay, normal));
    return fNDotL;
}

// Phong lighting specular component
float4 CalculateSpecularCoefficient(in float3 hitPosition, in float3 incidentLightRay, in float3 normal, in float specularPower)
{
    float3 reflectedLightRay = normalize(reflect(incidentLightRay, normal));
    return pow(saturate(dot(reflectedLightRay, normalize(-WorldRayDirection()))), specularPower);
}


// Phong lighting model = ambient + diffuse + specular components.
float4 CalculatePhongLighting(in float4 albedo, in float3 normal, in bool isInShadow, in float diffuseCoef = 1.0, in float specularCoef = 1.0, in float specularPower = 50)
{
    float3 hitPosition = HitWorldPosition();
    float shadowFactor = isInShadow ? InShadowRadiance : 1.0;
    float3 incidentLightRay = normalize(hitPosition - lightPosition);

    // Diffuse component.
    float Kd = CalculateDiffuseCoefficient(hitPosition, incidentLightRay, normal);
    float4 diffuseColor = shadowFactor * diffuseCoef* Kd* lightDiffuseColor* albedo;

    // Specular component.
    float4 specularColor = float4(0, 0, 0, 0);
    if (!isInShadow)
    {
        float4 lightSpecularColor = float4(1, 1, 1, 1);
        float4 Ks = CalculateSpecularCoefficient(hitPosition, incidentLightRay, normal, specularPower);
        specularColor = specularCoef * Ks * lightSpecularColor;
    }

    // Ambient component.
    // Fake AO: Darken faces with normal facing downwards/away from the sky a little bit.
    float4 ambientColor = lightAmbientColor;
    float4 ambientColorMin = lightAmbientColor - 0.15;
    float4 ambientColorMax = lightAmbientColor;
    float a = 1 - saturate(dot(normal, float3(0, -1, 0)));
    ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, a);

    return ambientColor + diffuseColor + specularColor;
}

// Fresnel reflectance - schlick approximation.
float3 FresnelReflectanceSchlick(in float3 I, in float3 N, in float3 f0)
{
    float cosi = saturate(dot(-I, N));
    return f0 + (1 - f0) * pow(1 - cosi, 5);
}

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(ByteAddressBuffer Indices, uint offsetBytes)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;
    const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);

    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

/*
 * ----------------------------------------
 * MAIN SHADER
 * ----------------------------------------
 */

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);

ByteAddressBuffer Indices : register(t1);
StructuredBuffer<VertexPositionNormalTangentTexture> Vertices : register(t2);

struct RayPayload
{
    float4 color;
    uint recursionDepth;
};

struct ShadowPayload
{
    bool hit;
};


[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd / dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;

    RayDesc ray;
    ray.Origin = cameraPosition;
    ray.Direction = normalize(float3(d.x * aspectRatio, -d.y, 1));

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    payload.recursionDepth = 0;
    TraceRay(gRtScene,
        0 /*rayFlags*/,
        0xFF,
        0 /* ray index*/,
        0 /* Multiplies */,
        0 /* Miss index */,
        ray,
        payload);

    gOutput[launchIndex.xy] = linearToSrgb(payload.color);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    float4 gradientStart = float4(0.4, 0.6, 0.2, 1.0);
    float4 gradientEnd = float4(0.5, 0.6, 1.0, 1.0);
    float3 unitDir = normalize(WorldRayDirection());
    float t = 0.5 * unitDir.y;
    payload.color = (1.0 - t) * gradientStart + t * gradientEnd;
    //payload.color = backgroundColor;
}

float3 HitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 hitPosition = HitWorldPosition();

    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = 2;
    uint indicesPerTriangle = 3;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(Indices, baseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 vertexNormals[3] = {
        Vertices[indices[0]].Normal,
        Vertices[indices[1]].Normal,
        Vertices[indices[2]].Normal
    };

    float3 hitNormal = (InstanceID() == 0) ? float3(0, 1, 0) : HitAttribute(vertexNormals, attribs);

    float4 color;
    float4 diffuseColor = (InstanceID() == 0) ? groundAlbedo : primitiveAlbedo;
    if (payload.recursionDepth < MaxRecursionDepth)
    {
        // Shadow
        RayDesc shadowRay;
        shadowRay.Origin = hitPosition;
        shadowRay.Direction = normalize(lightPosition - shadowRay.Origin);
        shadowRay.TMin = 0.01;
        shadowRay.TMax = 100000;
        ShadowPayload shadowPayload;
        TraceRay(gRtScene,
            0  /*rayFlags*/,
            0xFF,
            1 /* ray index*/,
            0 /* Multiplies */,
            1 /* Miss index (shadow) */,
            shadowRay,
            shadowPayload);

        // Reflection    
        RayDesc reflectionRay;
        reflectionRay.Origin = hitPosition;
        reflectionRay.Direction = reflect(WorldRayDirection(), hitNormal);
        reflectionRay.TMin = 0.01;
        reflectionRay.TMax = 100000;
        RayPayload reflectionPayload;
        reflectionPayload.recursionDepth = payload.recursionDepth + 1;
        TraceRay(gRtScene,
            0  /*rayFlags*/,
            0xFF,
            0 /* ray index*/,
            0 /* Multiplies */,
            0 /* Miss index (raytrace) */,
            reflectionRay,
            reflectionPayload);
        float4 reflectionColor = reflectionPayload.color;

        float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), hitNormal, diffuseColor);
        float4 reflectedColor = reflectanceCoef * float4(fresnelR, 1) * reflectionColor;

        // Calculate final color.
        float4 phongColor = CalculatePhongLighting(diffuseColor, hitNormal, shadowPayload.hit, diffuseCoef, specularCoef, specularPower);
        color = phongColor + reflectedColor;
    }
    else
    {
        color = CalculatePhongLighting(diffuseColor, hitNormal, false, diffuseCoef, specularCoef, specularPower);
    }

    // Apply visibility falloff.
    float t = RayTCurrent();
    color = lerp(color, backgroundColor, 1.0 - exp(-0.000002 * t * t * t));

    payload.color = color;
}

[shader("miss")]
void shadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}
