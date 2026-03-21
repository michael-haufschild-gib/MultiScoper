/*
    Oscil - Crystalline Shader Implementation
*/

#include "rendering/materials/CrystallineShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

static const char* crystalVertexShader = R"(
    #version 330 core
    in vec3 position;
    in vec3 normal;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    out vec3 vNormal;
    out vec3 vWorldPos;

    void main()
    {
        vec4 worldPos = uModel * vec4(position, 1.0);
        gl_Position = uProjection * uView * worldPos;
        
        vWorldPos = worldPos.xyz;
        vNormal = mat3(uModel) * normal;
    }
)";

static const char* crystalFragmentShader = R"(
    #version 330 core
    in vec3 vNormal;
    in vec3 vWorldPos;

    uniform vec3 uCameraPos;
    uniform vec3 uLightDir;
    uniform samplerCube uEnvMap;

    // Material
    uniform vec4 uBaseColor;
    uniform float uRoughness;
    uniform float uMetallic;
    uniform float uIOR;

    out vec4 fragColor;

    // Convert sRGB to linear color space for correct rendering pipeline
    vec3 sRGBToLinear(vec3 srgb) {
        return pow(srgb, vec3(2.2));
    }

    void main()
    {
        vec3 N = normalize(vNormal);
        vec3 V = normalize(uCameraPos - vWorldPos);
        vec3 L = normalize(uLightDir);
        vec3 H = normalize(L + V);

        // Basic PBR-ish lighting
        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float NdotV = max(dot(N, V), 0.0);

        // Specular (Blinn-Phong approx for crystal spark)
        float specPower = 128.0 * (1.0 - uRoughness);
        float spec = pow(NdotH, specPower);

        // Reflection
        vec3 R = reflect(-V, N);
        vec3 reflection = texture(uEnvMap, R).rgb;
        
        // Refraction (fake via transmission approx or simple environment lookup)
        vec3 T = refract(-V, N, 1.0 / uIOR);
        vec3 refraction = texture(uEnvMap, T).rgb;

        // Fresnel
        float F0 = 0.04 + 0.96 * uMetallic; // Dielectric vs Metal
        float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

        // Convert to linear for correct tonemapping
        vec3 linearBaseColor = sRGBToLinear(uBaseColor.rgb);

        // Combine
        vec3 finalColor = mix(refraction * linearBaseColor, reflection, fresnel);
        finalColor += spec * vec3(1.0); // Add specular highlight

        fragColor = vec4(finalColor, uBaseColor.a);
    }
)";

CrystallineShader::CrystallineShader()
{
    // Set default material properties for crystal
    material_.baseColorR = 0.9f;
    material_.baseColorG = 0.9f;
    material_.baseColorB = 1.0f;
    material_.roughness = 0.05f;
    material_.metallic = 0.1f;
    material_.ior = 2.42f; // Diamond
}

CrystallineShader::~CrystallineShader()
{
#if OSCIL_ENABLE_OPENGL
    if (compiled_)
    {
        DBG("[CrystallineShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

bool CrystallineShader::compile(juce::OpenGLContext& context)
{
    if (compiled_) return true;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, crystalVertexShader, crystalFragmentShader))
    {
        shader_.reset();
        return false;
    }

    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    cameraPosLoc_ = shader_->getUniformIDFromName("uCameraPos");
    lightDirLoc_ = shader_->getUniformIDFromName("uLightDir");
    
    baseColorLoc_ = shader_->getUniformIDFromName("uBaseColor");
    roughnessLoc_ = shader_->getUniformIDFromName("uRoughness");
    metallicLoc_ = shader_->getUniformIDFromName("uMetallic");
    iorLoc_ = shader_->getUniformIDFromName("uIOR");
    envMapLoc_ = shader_->getUniformIDFromName("uEnvMap");

    context.extensions.glGenVertexArrays(1, &vao_);
    context.extensions.glGenBuffers(1, &vbo_);
    context.extensions.glGenBuffers(1, &ibo_);

    compiled_ = true;
    return true;
}

void CrystallineShader::release(juce::OpenGLContext& context)
{
    if (!compiled_) return;
    context.extensions.glDeleteBuffers(1, &vbo_);
    context.extensions.glDeleteBuffers(1, &ibo_);
    context.extensions.glDeleteVertexArrays(1, &vao_);
    shader_.reset();
    compiled_ = false;
}

void CrystallineShader::update(float deltaTime)
{
    time_ += deltaTime;
}

void CrystallineShader::setParameter(const juce::String& name, float value)
{
    if (name == "crystalScale") crystalScale_ = value;
}

float CrystallineShader::getParameter(const juce::String& name) const
{
    if (name == "crystalScale") return crystalScale_;
    return 0.0f;
}

void CrystallineShader::render(juce::OpenGLContext& context,
                               const WaveformData3D& data,
                               const Camera3D& camera,
                               const LightingConfig& lighting)
{
    if (!compiled_ || data.sampleCount < 2) return;

    float xSpread, halfHeight;
    calculateCameraSpread(camera, xSpread, halfHeight);

    generateCrystalMesh(data, xSpread);
    if (indexCount_ == 0) return;

    auto& ext = context.extensions;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_->use();

    Matrix4 model = Matrix4::translation(0, data.yOffset * halfHeight, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude * halfHeight, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);
    setLightingUniforms(ext, lightDirLoc_, -1, -1, -1, -1, lighting);
    ext.glUniform3f(cameraPosLoc_, camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
    setMaterialUniforms(ext, baseColorLoc_, metallicLoc_, roughnessLoc_, -1, iorLoc_, -1, -1, material_);
    bindEnvironmentMap(ext, 0, envMapLoc_);

    uploadAndDrawCrystalMesh(ext);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void CrystallineShader::uploadAndDrawCrystalMesh(juce::OpenGLExtensionFunctions& ext)
{
    ext.glBindVertexArray(vao_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)), vertices_.data(), GL_DYNAMIC_DRAW);
    ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)), indices_.data(), GL_DYNAMIC_DRAW);

    GLint posLoc = ext.glGetAttribLocation(shader_->getProgramID(), "position");
    GLint normLoc = ext.glGetAttribLocation(shader_->getProgramID(), "normal");
    if (posLoc < 0) posLoc = 0;
    if (normLoc < 0) normLoc = 1;

    ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(normLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(normLoc), 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

    ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(normLoc));
    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void CrystallineShader::generateCrystalVertices(const WaveformData3D& data, float xSpread, int polySegments, float radius)
{
    for (int i = 0; i < data.sampleCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(data.sampleCount - 1);
        float x = (t * 2.0f - 1.0f) * xSpread;
        float y = data.samples[i];

        for (int j = 0; j < polySegments; ++j)
        {
            float ang = static_cast<float>(j) / polySegments * 2.0f * 3.14159f + t * 10.0f;
            float c = cos(ang);
            float s = sin(ang);
            float r = radius * (1.0f + 0.5f * sin(t * 50.0f));

            vertices_.push_back(x);
            vertices_.push_back(y + s * r);
            vertices_.push_back(c * r);
            vertices_.push_back(0.0f);
            vertices_.push_back(s);
            vertices_.push_back(c);
        }
    }
}

void CrystallineShader::generateCrystalIndices(int sampleCount, int polySegments)
{
    for (int i = 0; i < sampleCount - 1; ++i)
    {
        for (int j = 0; j < polySegments; ++j)
        {
            int curr = i * polySegments + j;
            int next = i * polySegments + ((j + 1) % polySegments);
            int currNext = (i + 1) * polySegments + j;
            int nextNext = (i + 1) * polySegments + ((j + 1) % polySegments);

            indices_.push_back(static_cast<GLuint>(curr));
            indices_.push_back(static_cast<GLuint>(currNext));
            indices_.push_back(static_cast<GLuint>(next));
            indices_.push_back(static_cast<GLuint>(next));
            indices_.push_back(static_cast<GLuint>(currNext));
            indices_.push_back(static_cast<GLuint>(nextNext));
        }
    }
}

void CrystallineShader::generateCrystalMesh(const WaveformData3D& data, float xSpread)
{
    vertices_.clear();
    indices_.clear();

    int polySegments = 4;
    generateCrystalVertices(data, xSpread, polySegments, crystalScale_);
    generateCrystalIndices(data.sampleCount, polySegments);

    indexCount_ = static_cast<int>(indices_.size());
}

#endif

} // namespace oscil
