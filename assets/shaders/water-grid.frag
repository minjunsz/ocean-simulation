#version 410 core

uniform sampler2D depthTexture2D;
uniform sampler2D localReflectionsTexture2D;
uniform sampler2D localRefractionsTexture2D;
uniform samplerCube skybox;
uniform float softEdgesDeltaDepthThreshold;
uniform vec3 sunPosition;
uniform float sunShininess;
uniform float sunStrength;
uniform float tintDeltaDepthThreshold;
uniform vec2 viewportWidthHeight;
uniform float waterClarity;
uniform float zFar;
uniform float zNear;

in vec3 normal;
in vec3 normalVecInViewSpaceOnlyYaw;
in vec3 viewVecRaw;
in vec2 xyPositionNDCSpaceHeight0;
in float jacobianDeterminant;

out vec4 colour;

// Precomputed constant for Fresnel reflectance (air-to-water transition)
const float fresnel_f_0 = 0.02f;

// Tint colors
const vec3 DEEP_TINT_COLOUR_AT_NOON = vec3(0.0f, 0.341f, 0.482f);
const vec3 SHALLOW_TINT_COLOUR_AT_NOON = vec3(0.3f, 0.941f, 0.903f);

// Converts non-linear depth to linear depth
float linearizeDepth(float depth) {
    return (zNear * depth) / (zFar - depth * (zFar - zNear));
}

void main() {
    float viewVecLength = length(viewVecRaw);
    float viewVecDepthClamped = clamp(viewVecLength / zFar, 0.0f, 1.0f);
    vec3 viewVec = viewVecRaw / viewVecLength;

    vec2 uvViewportSpace = gl_FragCoord.xy / viewportWidthHeight;
    vec2 uvViewportSpaceHeight0 = (xyPositionNDCSpaceHeight0 + vec2(1.0f, 1.0f)) * 0.5f;

    float depthFragBack = linearizeDepth(texture(depthTexture2D, uvViewportSpace).x);
    float hack_skybox_in_back = step(0.999f, depthFragBack);
    float depthFrag = linearizeDepth(gl_FragCoord.z);
    float deltaDepthClamped = clamp(depthFragBack - depthFrag, 0.0f, 1.0f);

    vec3 R = reflect(-viewVec, normal);
    vec3 skyboxReflection = texture(skybox, R).rgb;
    vec3 sunReflection = sunStrength * pow(clamp(dot(R, sunPosition), 0.0f, 1.0f), sunShininess) * vec3(1.0);

    float fresnelCosTheta = dot(normal, viewVec);
    float fresnel_f_theta = fresnel_f_0 + (1.0f - fresnel_f_0) * pow(1.0f - fresnelCosTheta, 5);

    float distortionScalar = 1.0f - viewVecDepthClamped;
    vec2 uvDistortionXZ = vec2(normalVecInViewSpaceOnlyYaw.x, -normalVecInViewSpaceOnlyYaw.z);

    vec2 uvLocalReflections = clamp(
        mix(uvViewportSpace, uvViewportSpaceHeight0, 0.1f * distortionScalar) + 
        0.1f * distortionScalar * uvDistortionXZ,
        vec2(0.0f),
        vec2(1.0f)
    );
    vec2 uvLocalRefractions = clamp(
        mix(uvViewportSpace, uvViewportSpaceHeight0, 0.1f * distortionScalar) + 
        0.1f * distortionScalar * -uvDistortionXZ,
        vec2(0.0f),
        vec2(1.0f)
    );

    vec4 localReflectionColour = texture(localReflectionsTexture2D, uvLocalReflections);
    vec4 localRefractionColour = texture(localRefractionsTexture2D, uvLocalRefractions);

    float tintFactor = hack_skybox_in_back * (1.0f - clamp(deltaDepthClamped / tintDeltaDepthThreshold, 0.0f, 1.0f));
    vec3 tintColour = mix(DEEP_TINT_COLOUR_AT_NOON, SHALLOW_TINT_COLOUR_AT_NOON, tintFactor) *
                      clamp(sunPosition.y, 0.0f, 1.0f);

    vec3 waterTransmission = mix(tintColour, localRefractionColour.rgb, waterClarity * localRefractionColour.a * (1.0f - deltaDepthClamped));
    vec3 waterReflection = mix(skyboxReflection + sunReflection, localReflectionColour.rgb, localReflectionColour.a);

    colour.rgb = mix(waterTransmission, (jacobianDeterminant > 0.0f) ? waterReflection : vec3(1.0f, 1.0f, 1.0f), fresnel_f_theta);

    float edgeHardness = 1.0f - hack_skybox_in_back * (1.0f - clamp(deltaDepthClamped / softEdgesDeltaDepthThreshold, 0.0f, 1.0f));
    colour.a = edgeHardness;
}