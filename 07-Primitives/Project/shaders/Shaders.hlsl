/*
 * HELPER FUNCTIONS
 */

float4 linearToSrgb(float4 c)
{
    // Based on http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
    float4 sq1 = sqrt(c);
    float4 sq2 = sqrt(sq1);
    float4 sq3 = sqrt(sq2);
    float4 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

/*
 * MAIN SHADER
 */

static const float3 cameraPosition = float3(0, 0, -2);
static const float4 backgroundColor = float4(0.4, 0.6, 0.2, 1.0);

RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);

struct RayPayload
{
    float4 color;
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
    payload.color = backgroundColor;
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float4 barycentrics = float4(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y,1);

    const float4 Red = float4(1, 0, 0, 1);
    const float4 Green = float4(0, 1, 0, 1);
    const float4 Blue = float4(0, 0, 1, 1);

    payload.color = Red * barycentrics.x + Green * barycentrics.y + Blue * barycentrics.z;
}
