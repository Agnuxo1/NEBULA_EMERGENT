// OptiX Implementation for NEBULA
// OptiXRayTracing.cu

#include <optix.h>
#include <cuda_runtime.h>

// OptiX ray generation program
extern "C" __global__ void __raygen__nebula()
{
    const uint3 idx = optixGetLaunchIndex();
    const uint3 dim = optixGetLaunchDimensions();
    
    // Camera setup
    const float3 eye = params.eye;
    const float3 U = params.U;
    const float3 V = params.V;
    const float3 W = params.W;
    
    // Generate ray
    const float2 d = 2.0f * make_float2(
        static_cast<float>(idx.x) / static_cast<float>(dim.x),
        static_cast<float>(idx.y) / static_cast<float>(dim.y)
    ) - 1.0f;
    
    const float3 direction = normalize(d.x * U + d.y * V + W);
    
    // Trace ray through neural galaxy
    unsigned int p0, p1, p2;
    p0 = __float_as_uint(1.0f);  // Current luminosity
    p1 = __float_as_uint(0.0f);  // Accumulated spectrum R
    p2 = __float_as_uint(0.0f);  // Accumulated spectrum G
    
    optixTrace(
        params.handle,
        eye,
        direction,
        0.01f,                // tmin
        1e16f,                // tmax
        0.0f,                 // rayTime
        OptixVisibilityMask(1),
        OPTIX_RAY_FLAG_NONE,
        0,                    // SBT offset
        0,                    // SBT stride
        0,                    // missSBTIndex
        p0, p1, p2
    );
    
    // Write result
    params.frame_buffer[idx.y * dim.x + idx.x] = make_float4(
        __uint_as_float(p0),
        __uint_as_float(p1),
        __uint_as_float(p2),
        1.0f
    );
}

// Sphere intersection for neurons
extern "C" __global__ void __intersection__sphere()
{
    const float3 ray_orig = optixGetObjectRayOrigin();
    const float3 ray_dir = optixGetObjectRayDirection();
    const float ray_tmin = optixGetRayTmin();
    const float ray_tmax = optixGetRayTmax();
    
    const unsigned int primIdx = optixGetPrimitiveIndex();
    const float3 center = params.neurons[primIdx].position;
    const float radius = params.neurons[primIdx].radius;
    
    const float3 O = ray_orig - center;
    const float b = dot(O, ray_dir);
    const float c = dot(O, O) - radius * radius;
    const float discriminant = b * b - c;
    
    if (discriminant > 0.0f)
    {
        const float sqrtd = sqrtf(discriminant);
        const float t0 = -b - sqrtd;
        const float t1 = -b + sqrtd;
        
        if (t0 >= ray_tmin && t0 <= ray_tmax)
        {
            optixReportIntersection(t0, 0);
        }
        else if (t1 >= ray_tmin && t1 <= ray_tmax)
        {
            optixReportIntersection(t1, 0);
        }
    }
}

// Closest hit program for neurons
extern "C" __global__ void __closesthit__neuron()
{
    const unsigned int primIdx = optixGetPrimitiveIndex();
    
    // Get neuron data
    NeuronData& neuron = params.neurons[primIdx];
    
    // Calculate illumination
    float3 hitPoint = optixGetWorldRayOrigin() + optixGetRayTmax() * optixGetWorldRayDirection();
    float3 normal = normalize(hitPoint - neuron.position);
    
    // Accumulate neuron's light contribution
    float luminosity = neuron.luminosity;
    float3 spectrum = make_float3(
        neuron.spectrum.x,
        neuron.spectrum.y,
        neuron.spectrum.z
    );
    
    // Pass accumulated light
    unsigned int p0 = optixGetPayload_0();
    unsigned int p1 = optixGetPayload_1();
    unsigned int p2 = optixGetPayload_2();
    
    float current_lum = __uint_as_float(p0);
    float current_r = __uint_as_float(p1);
    float current_g = __uint_as_float(p2);
    
    // Add this neuron's contribution
    current_lum += luminosity * neuron.activation;
    current_r += spectrum.x * luminosity;
    current_g += spectrum.y * luminosity;
    
    optixSetPayload_0(__float_as_uint(current_lum));
    optixSetPayload_1(__float_as_uint(current_r));
    optixSetPayload_2(__float_as_uint(current_g));
    
    // Continue tracing (for volumetric effect)
    if (current_lum < 10.0f)  // Max accumulation
    {
        float3 newDirection = optixGetWorldRayDirection();
        
        // Scatter direction (simplified)
        newDirection = normalize(newDirection + normal * 0.1f);
        
        optixTrace(
            params.handle,
            hitPoint + normal * 0.01f,
            newDirection,
            0.01f,
            1e16f,
            0.0f,
            OptixVisibilityMask(1),
            OPTIX_RAY_FLAG_NONE,
            0, 0, 0,
            p0, p1, p2
        );
    }
}