/*
    Oscil - Glass Refraction Shader Implementation
*/

#include "rendering/materials/GlassRefractionShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* glassVertexShader = R"(
    #version 330 core
    in vec3 aPosition;
    in vec3 aNormal;
    in vec2 aTexCoord;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    out vec3 vWorldPos;
    out vec3 vNormal;
    out vec2 vTexCoord;

    void main()
    {
        vec4 worldPos = uModel * vec4(aPosition, 1.0);
        vWorldPos = worldPos.xyz;
        vNormal = mat3(uModel) * aNormal;
        vTexCoord = aTexCoord;

        gl_Position = uProjection * uView * worldPos;
    }
)";

static const char* glassFragmentShader = R"(
    #version 330 core
    uniform vec4 uColor;
    uniform vec3 uCameraPos;
    uniform float uIOR;
    uniform float uRefractionStrength;
    uniform float uFresnelPower;
    uniform float uDispersion;
    uniform float uTime;
    uniform samplerCube uEnvMap;
    uniform vec3 uLightDir;
    uniform vec4 uSpecular;

    in vec3 vWorldPos;
    in vec3 vNormal;
    in vec2 vTexCoord;

    out vec4 fragColor;

    // Convert sRGB to linear color space for correct rendering pipeline
    vec3 sRGBToLinear(vec3 srgb) {
        return pow(srgb, vec3(2.2));
    }

    void main()
    {
        vec3 normal = normalize(vNormal);
        vec3 viewDir = normalize(uCameraPos - vWorldPos);
        vec3 lightDir = normalize(uLightDir);

        // Fresnel effect
        float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), uFresnelPower);

        // Reflection
        vec3 reflectDir = reflect(-viewDir, normal);
        vec3 reflectColor = texture(uEnvMap, reflectDir).rgb;

        // Refraction with chromatic dispersion
        float iorR = uIOR - uDispersion;
        float iorG = uIOR;
        float iorB = uIOR + uDispersion;

        vec3 refractDirR = refract(-viewDir, normal, 1.0 / iorR);
        vec3 refractDirG = refract(-viewDir, normal, 1.0 / iorG);
        vec3 refractDirB = refract(-viewDir, normal, 1.0 / iorB);

        float refractR = texture(uEnvMap, refractDirR).r;
        float refractG = texture(uEnvMap, refractDirG).g;
        float refractB = texture(uEnvMap, refractDirB).b;
        vec3 refractColor = vec3(refractR, refractG, refractB);

        // Convert to linear for correct color operations
        vec3 linearColor = sRGBToLinear(uColor.rgb);

        // Blend refraction with base color
        refractColor = mix(linearColor, refractColor, uRefractionStrength);

        // Specular highlight
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(normal, halfDir), 0.0);
        float specular = pow(specAngle, uSpecular.a * 128.0) * uSpecular.r;

        // Combine reflection and refraction based on fresnel
        vec3 finalColor = mix(refractColor, reflectColor, fresnel);

        // Add specular
        finalColor += vec3(specular);

        // Subtle edge glow
        float edgeGlow = fresnel * 0.3;
        finalColor += linearColor * edgeGlow;

        // Animated shimmer
        float shimmer = sin(vTexCoord.x * 30.0 + uTime * 2.0) * 0.02 + 0.98;
        finalColor *= shimmer;

        fragColor = vec4(finalColor, uColor.a * (0.5 + fresnel * 0.5));
    }
)";

GlassRefractionShader::GlassRefractionShader()
{
    // Set default glass material properties
    material_.ior = 1.5f;
    material_.refractionStrength = 0.8f;
    material_.fresnelPower = 3.0f;
    material_.reflectivity = 0.5f;
    material_.roughness = 0.0f;
}

GlassRefractionShader::~GlassRefractionShader()
{
#if OSCIL_ENABLE_OPENGL
    if (compiled_)
    {
        DBG("[GlassRefractionShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

bool GlassRefractionShader::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    auto& ext = context.extensions;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, glassVertexShader, glassFragmentShader))
    {
        shader_.reset();
        return false;
    }

    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    cameraPosLoc_ = shader_->getUniformIDFromName("uCameraPos");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    iorLoc_ = shader_->getUniformIDFromName("uIOR");
    refractionStrengthLoc_ = shader_->getUniformIDFromName("uRefractionStrength");
    fresnelPowerLoc_ = shader_->getUniformIDFromName("uFresnelPower");
    dispersionLoc_ = shader_->getUniformIDFromName("uDispersion");
    timeLoc_ = shader_->getUniformIDFromName("uTime");
    envMapLoc_ = shader_->getUniformIDFromName("uEnvMap");
    lightDirLoc_ = shader_->getUniformIDFromName("uLightDir");
    specularLoc_ = shader_->getUniformIDFromName("uSpecular");

    ext.glGenVertexArrays(1, &vao_);
    ext.glBindVertexArray(vao_);
    ext.glGenBuffers(1, &vbo_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glGenBuffers(1, &ibo_);
    ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

    setupPosNormTexAttribs(ext, shader_->getProgramID());

    ext.glBindVertexArray(0);
    compiled_ = true;
    return true;
}

void GlassRefractionShader::release(juce::OpenGLContext& context)
{
    if (!compiled_)
        return;

    auto& ext = context.extensions;

    if (ibo_ != 0)
    {
        ext.glDeleteBuffers(1, &ibo_);
        ibo_ = 0;
    }

    if (vbo_ != 0)
    {
        ext.glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0)
    {
        ext.glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    shader_.reset();
    compiled_ = false;
}

void GlassRefractionShader::setGlassUniforms(juce::OpenGLExtensionFunctions& ext,
                                              const WaveformData3D& data,
                                              const Camera3D& camera,
                                              const LightingConfig& lighting,
                                              float halfHeight)
{
    Matrix4 model = Matrix4::translation(0, data.yOffset * halfHeight, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude * halfHeight, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    Vec3 camPos = camera.getPosition();
    ext.glUniform3f(cameraPosLoc_, camPos.x, camPos.y, camPos.z);

    ext.glUniform4f(colorLoc_, data.color.getFloatRed(), data.color.getFloatGreen(),
                    data.color.getFloatBlue(), data.color.getFloatAlpha());

    ext.glUniform1f(iorLoc_, material_.ior);
    ext.glUniform1f(refractionStrengthLoc_, material_.refractionStrength);
    ext.glUniform1f(fresnelPowerLoc_, material_.fresnelPower);
    ext.glUniform1f(dispersionLoc_, dispersion_);
    ext.glUniform1f(timeLoc_, time_);

    setLightingUniforms(ext, lightDirLoc_, -1, -1, specularLoc_, -1, lighting);
}

void GlassRefractionShader::render(juce::OpenGLContext& context,
                                   const WaveformData3D& data,
                                   const Camera3D& camera,
                                   const LightingConfig& lighting)
{
    if (!compiled_ || !data.samples || data.sampleCount < 2)
        return;

    float xSpread, halfHeight;
    calculateCameraSpread(camera, xSpread, halfHeight);

    updateMesh(data, xSpread);
    if (indexCount_ == 0) return;

    auto& ext = context.extensions;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    shader_->use();
    setGlassUniforms(ext, data, camera, lighting, halfHeight);

    if (hasEnvironmentMap())
        bindEnvironmentMap(ext, 0, envMapLoc_);

    ext.glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glCullFace(GL_BACK);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    ext.glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

void GlassRefractionShader::update(float deltaTime)
{
    time_ += deltaTime;
    if (time_ > 1000.0f)
        time_ = std::fmod(time_, 1000.0f);
}

void GlassRefractionShader::updateMesh(const WaveformData3D& data, float xSpread)
{
    // Generate tube mesh using base class helper
    generateTubeMesh(data.samples, data.sampleCount, xSpread, thickness_, 12,
                     vertices_, indices_);

    if (vertices_.empty() || indices_.empty())
    {
        indexCount_ = 0;
        return;
    }

    // Upload to GPU
    auto* glContext = juce::OpenGLContext::getCurrentContext();
    if (!glContext)
        return;

    auto& ext = glContext->extensions;

    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
                     vertices_.data(), GL_DYNAMIC_DRAW);

    ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)),
                     indices_.data(), GL_DYNAMIC_DRAW);

    indexCount_ = static_cast<int>(indices_.size());
}

void GlassRefractionShader::setParameter(const juce::String& name, float value)
{
    if (name == "ior" || name == "refractionIndex")
        material_.ior = value;
    else if (name == "refractionStrength")
        material_.refractionStrength = value;
    else if (name == "fresnelPower")
        material_.fresnelPower = value;
    else if (name == "dispersion")
        dispersion_ = value;
    else if (name == "thickness")
        thickness_ = value;
}

float GlassRefractionShader::getParameter(const juce::String& name) const
{
    if (name == "ior" || name == "refractionIndex")
        return material_.ior;
    if (name == "refractionStrength")
        return material_.refractionStrength;
    if (name == "fresnelPower")
        return material_.fresnelPower;
    if (name == "dispersion")
        return dispersion_;
    if (name == "thickness")
        return thickness_;
    return 0.0f;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
