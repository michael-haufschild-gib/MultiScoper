/*
    Oscil - Particle System Shaders & Noise
    GPU shader source code and noise generation for particle rendering
*/

#include "rendering/particles/ParticleSystem.h"
#include <random>

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// ============================================================================
// Noise Implementation (Internal)
// ============================================================================

class Noise3D
{
public:
    static constexpr int SIZE = 32;

    static std::vector<float> generate()
    {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        std::vector<float> data(SIZE * SIZE * SIZE * 4);

        for (int z = 0; z < SIZE; ++z)
        {
            for (int y = 0; y < SIZE; ++y)
            {
                for (int x = 0; x < SIZE; ++x)
                {
                    int idx = ((z * SIZE + y) * SIZE + x) * 4;
                    float baseNoise = dist(rng);

                    // Multi-octave for richer turbulence
                    float noise2 = dist(rng) * 0.5f;
                    float noise3 = dist(rng) * 0.25f;
                    float combined = (baseNoise + noise2 + noise3) / 1.75f;

                    data[static_cast<size_t>(idx + 0)] = combined;
                    data[static_cast<size_t>(idx + 1)] = dist(rng);
                    data[static_cast<size_t>(idx + 2)] = dist(rng);
                    data[static_cast<size_t>(idx + 3)] = 1.0f;
                }
            }
        }

        return data;
    }
};

// ============================================================================
// Shaders
// ============================================================================

static const char* particleVertexShader = R"(
    #version 330 core
    in vec2 aPosition; // Quad vertex (-0.5 to 0.5)
    in vec2 aInstancePos;
    in vec4 aInstanceColor;
    in float aInstanceSize;
    in float aInstanceRotation;
    in float aInstanceAge;

    uniform mat4 uProjection;
    uniform vec2 uFrameInfo; // x=rows, y=cols

    // Turbulence Uniforms
    uniform sampler3D uNoiseTexture;
    uniform vec3 uTurbulence; // strength, scale, speed
    uniform float uTime;

    out vec4 vColor;
    out vec2 vTexCoord;
    out float vAge;

    void main()
    {
        // Rotate the quad vertex
        float c = cos(aInstanceRotation);
        float s = sin(aInstanceRotation);
        vec2 rotated = vec2(
            aPosition.x * c - aPosition.y * s,
            aPosition.x * s + aPosition.y * c
        );

        // Turbulence Calculation
        vec2 turbOffset = vec2(0.0);
        if (uTurbulence.x > 0.001) {
            float strength = uTurbulence.x;
            float scale = uTurbulence.y;
            float speed = uTurbulence.z;

            vec3 noiseCoord = vec3(aInstancePos * scale * 0.01, uTime * speed * 0.1);
            vec4 noiseVal = texture(uNoiseTexture, noiseCoord);
            vec2 force = (noiseVal.xy - 0.5) * 2.0;
            turbOffset = force * strength * 50.0;
        }

        // Scale and translate
        vec2 worldPos = aInstancePos + turbOffset + rotated * aInstanceSize;

        gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);

        vColor = aInstanceColor;
        vAge = aInstanceAge;

        // Texture Coordinates
        vec2 baseUV = aPosition + 0.5;

        // Sprite Sheet Calculation
        float rows = uFrameInfo.x;
        float cols = uFrameInfo.y;

        if (rows > 1.0 || cols > 1.0)
        {
            float totalFrames = rows * cols;
            float currentFrame = floor(aInstanceAge * totalFrames);
            currentFrame = clamp(currentFrame, 0.0, totalFrames - 1.0);

            float col = mod(currentFrame, cols);
            float row = floor(currentFrame / cols);

            vec2 size = vec2(1.0 / cols, 1.0 / rows);

            vTexCoord = (baseUV * size) + vec2(col * size.x, 1.0 - (row + 1.0) * size.y);
        }
        else
        {
            vTexCoord = baseUV;
        }
    }
)";

static const char* basicFragmentShader = R"(
    #version 330 core
    in vec4 vColor;
    in vec2 vTexCoord;
    in float vAge;

    out vec4 fragColor;

    void main()
    {
        vec2 center = vTexCoord - vec2(0.5);
        float dist = length(center) * 2.0;
        float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
        fragColor = vec4(vColor.rgb, vColor.a * alpha);
    }
)";

static const char* texturedFragmentShader = R"(
    #version 330 core
    in vec4 vColor;
    in vec2 vTexCoord;
    in float vAge;

    uniform sampler2D uTexture;

    out vec4 fragColor;

    void main()
    {
        vec4 texColor = texture(uTexture, vTexCoord);
        fragColor = vColor * texColor;
    }
)";

static const char* softFragmentShader = R"(
    #version 330 core
    in vec4 vColor;
    in vec2 vTexCoord;
    in float vAge;

    uniform sampler2D uTexture;
    uniform sampler2D uDepthMap;
    uniform vec2 uViewportSize;
    uniform float uSoftDepthParam;

    out vec4 fragColor;

    void main()
    {
        vec4 texColor = texture(uTexture, vTexCoord);
        vec2 screenUV = gl_FragCoord.xy / uViewportSize;
        float sceneDepth = texture(uDepthMap, screenUV).r;
        float partDepth = gl_FragCoord.z;
        float depthDiff = sceneDepth - partDepth;
        float alphaFade = clamp(depthDiff * uSoftDepthParam * 100.0, 0.0, 1.0);
        fragColor = vColor * texColor;
        fragColor.a *= alphaFade;
    }
)";

// compileShaders uses the shader strings defined above
bool ParticleSystem::compileShaders(juce::OpenGLContext& context)
{
    auto compile = [&](ShaderProgram& shader, const char* fragSource) -> bool {
        shader.program = std::make_unique<juce::OpenGLShaderProgram>(context);

        if (!shader.program->addVertexShader(particleVertexShader))
        {
            DBG("ParticleSystem: Vertex Shader Error: " << shader.program->getLastError());
            shader.program.reset();
            return false;
        }

        if (!shader.program->addFragmentShader(fragSource))
        {
            DBG("ParticleSystem: Fragment Shader Error: " << shader.program->getLastError());
            shader.program.reset();
            return false;
        }

        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 0, "aPosition");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 1, "aInstancePos");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 2, "aInstanceColor");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 3, "aInstanceSize");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 4, "aInstanceRotation");
        context.extensions.glBindAttribLocation(shader.program->getProgramID(), 5, "aInstanceAge");

        if (!shader.program->link())
        {
            DBG("ParticleSystem: Link Error: " << shader.program->getLastError());
            shader.program.reset();
            return false;
        }

        shader.projectionLoc = shader.program->getUniformIDFromName("uProjection");
        shader.textureLoc = shader.program->getUniformIDFromName("uTexture");
        shader.depthLoc = shader.program->getUniformIDFromName("uDepthMap");
        shader.noiseTextureLoc = shader.program->getUniformIDFromName("uNoiseTexture");
        shader.viewportSizeLoc = shader.program->getUniformIDFromName("uViewportSize");
        shader.softDepthParamLoc = shader.program->getUniformIDFromName("uSoftDepthParam");
        shader.frameInfoLoc = shader.program->getUniformIDFromName("uFrameInfo");
        shader.turbulenceLoc = shader.program->getUniformIDFromName("uTurbulence");
        shader.timeLoc = shader.program->getUniformIDFromName("uTime");
        return true;
    };

    bool ok = compile(basicShader_, basicFragmentShader);
    ok &= compile(texturedShader_, texturedFragmentShader);
    ok &= compile(softShader_, softFragmentShader);

    return ok;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
