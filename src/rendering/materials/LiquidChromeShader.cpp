/*
    Oscil - Liquid Chrome Shader Implementation
*/

#include "rendering/materials/LiquidChromeShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* chromeVertexShader = R"(
    #version 330 core
    in vec3 aPosition;
    in vec3 aNormal;
    in vec2 aTexCoord;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;
    uniform float uTime;
    uniform float uRippleFreq;
    uniform float uRippleAmp;

    out vec3 vWorldPos;
    out vec3 vNormal;
    out vec2 vTexCoord;

    void main()
    {
        // Apply liquid ripple displacement to position
        vec3 pos = aPosition;

        // Multiple wave frequencies for organic liquid feel
        float wave1 = sin(aTexCoord.x * uRippleFreq + uTime * 2.0) * uRippleAmp;
        float wave2 = sin(aTexCoord.x * uRippleFreq * 1.7 - uTime * 1.5) * uRippleAmp * 0.5;
        float wave3 = cos(aTexCoord.y * uRippleFreq * 0.5 + uTime) * uRippleAmp * 0.3;

        float displacement = wave1 + wave2 + wave3;

        // Displace along normal
        pos += aNormal * displacement;

        // Also perturb the normal for realistic lighting
        vec3 normal = aNormal;
        float normalPerturb = cos(aTexCoord.x * uRippleFreq + uTime * 2.0) * uRippleAmp * 2.0;
        normal = normalize(normal + vec3(normalPerturb, normalPerturb * 0.5, 0.0));

        vec4 worldPos = uModel * vec4(pos, 1.0);
        vWorldPos = worldPos.xyz;
        vNormal = mat3(uModel) * normal;
        vTexCoord = aTexCoord;

        gl_Position = uProjection * uView * worldPos;
    }
)";

static const char* chromeFragmentShader = R"(
    #version 330 core
    uniform vec4 uColor;
    uniform vec3 uCameraPos;
    uniform float uMetallic;
    uniform float uRoughness;
    uniform float uFresnelPower;
    uniform float uTime;
    uniform samplerCube uEnvMap;
    uniform vec3 uLightDir;
    uniform vec4 uSpecular;

    in vec3 vWorldPos;
    in vec3 vNormal;
    in vec2 vTexCoord;

    out vec4 fragColor;

    // Schlick Fresnel approximation
    vec3 fresnelSchlick(float cosTheta, vec3 F0)
    {
        return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    }

    void main()
    {
        vec3 normal = normalize(vNormal);
        vec3 viewDir = normalize(uCameraPos - vWorldPos);
        vec3 lightDir = normalize(uLightDir);

        // Base reflectance for chrome (very high)
        vec3 F0 = mix(vec3(0.04), uColor.rgb, uMetallic);

        // Fresnel
        float NdotV = max(dot(normal, viewDir), 0.0);
        vec3 fresnel = fresnelSchlick(NdotV, F0);

        // Environment reflection
        vec3 reflectDir = reflect(-viewDir, normal);

        // Perturb reflection for liquid motion
        float flowOffset = uTime * 0.3;
        reflectDir.x += sin(vTexCoord.x * 10.0 + flowOffset) * 0.05 * (1.0 - uRoughness);
        reflectDir.y += cos(vTexCoord.y * 8.0 + flowOffset) * 0.03 * (1.0 - uRoughness);
        reflectDir = normalize(reflectDir);

        vec3 envColor = texture(uEnvMap, reflectDir).rgb;

        // Roughness affects reflection sharpness (fake blur via mip level)
        // Note: In real PBR we'd sample different mip levels

        // Specular highlight
        vec3 halfDir = normalize(lightDir + viewDir);
        float NdotH = max(dot(normal, halfDir), 0.0);
        float specPower = mix(256.0, 8.0, uRoughness);
        float spec = pow(NdotH, specPower);

        // Chrome reflection color
        vec3 chromeColor = envColor * fresnel;

        // Add base color tint for colored chrome
        // Increased mix to 0.8 to ensure user color choice is clearly visible
        chromeColor = mix(chromeColor, chromeColor * uColor.rgb, 0.8);

        // Add specular highlights
        vec3 specColor = uSpecular.rgb * spec * (1.0 - uRoughness);
        chromeColor += specColor;

        // Metallic edge highlight
        float edgeFactor = 1.0 - NdotV;
        edgeFactor = pow(edgeFactor, uFresnelPower);
        chromeColor += uColor.rgb * edgeFactor * 0.2;

        // Animated shimmer for liquid effect
        float shimmer = sin(vTexCoord.x * 50.0 + uTime * 4.0) * 0.03 + 0.97;
        chromeColor *= shimmer;

        fragColor = vec4(chromeColor, uColor.a);
    }
)";

LiquidChromeShader::LiquidChromeShader()
{
    // Set default chrome material properties
    material_.metallic = 1.0f;
    material_.roughness = 0.05f;
    material_.fresnelPower = 5.0f;
    material_.reflectivity = 1.0f;
    material_.baseColorR = 0.8f;
    material_.baseColorG = 0.8f;
    material_.baseColorB = 0.85f;
}

LiquidChromeShader::~LiquidChromeShader() = default;

bool LiquidChromeShader::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    auto& ext = context.extensions;

    // Compile shader
    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, chromeVertexShader, chromeFragmentShader))
    {
        DBG("LiquidChromeShader: Failed to compile shader");
        shader_.reset();
        return false;
    }

    // Get uniform locations
    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    cameraPosLoc_ = shader_->getUniformIDFromName("uCameraPos");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    metallicLoc_ = shader_->getUniformIDFromName("uMetallic");
    roughnessLoc_ = shader_->getUniformIDFromName("uRoughness");
    fresnelPowerLoc_ = shader_->getUniformIDFromName("uFresnelPower");
    timeLoc_ = shader_->getUniformIDFromName("uTime");
    rippleFreqLoc_ = shader_->getUniformIDFromName("uRippleFreq");
    rippleAmpLoc_ = shader_->getUniformIDFromName("uRippleAmp");
    envMapLoc_ = shader_->getUniformIDFromName("uEnvMap");
    lightDirLoc_ = shader_->getUniformIDFromName("uLightDir");
    specularLoc_ = shader_->getUniformIDFromName("uSpecular");

    // Create VAO
    ext.glGenVertexArrays(1, &vao_);
    ext.glBindVertexArray(vao_);

    // Create VBO
    ext.glGenBuffers(1, &vbo_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Create IBO
    ext.glGenBuffers(1, &ibo_);
    ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

    // Setup vertex attributes
    GLint posAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aPosition");
    GLint normAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aNormal");
    GLint texAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aTexCoord");

    const int stride = 8 * sizeof(float);

    if (posAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(posAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(posAttrib), 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    }

    if (normAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(normAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(normAttrib), 3, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void*>(3 * sizeof(float)));
    }

    if (texAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(texAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(texAttrib), 2, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void*>(6 * sizeof(float)));
    }

    ext.glBindVertexArray(0);

    compiled_ = true;
    DBG("LiquidChromeShader: Compiled successfully");
    return true;
}

void LiquidChromeShader::release(juce::OpenGLContext& context)
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

void LiquidChromeShader::render(juce::OpenGLContext& context,
                                const WaveformData3D& data,
                                const Camera3D& camera,
                                const LightingConfig& lighting)
{
    if (!compiled_ || !data.samples || data.sampleCount < 2)
        return;

    // Calculate xSpread to fill the screen width and halfHeight for vertical scaling
    float xSpread = 1.0f;
    float halfHeight = 1.0f;

    if (camera.getProjection() == CameraProjection::Orthographic)
    {
        float height = camera.getOrthoSize();
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
        halfHeight = height * 0.5f;
    }
    else
    {
        float dist = (camera.getPosition() - camera.getTarget()).length();
        float fovRad = camera.getFOV() * 3.14159265f / 180.0f;
        float height = 2.0f * dist * std::tan(fovRad * 0.5f);
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
        halfHeight = height * 0.5f;
    }

    // Update mesh
    updateMesh(data, xSpread);

    if (indexCount_ == 0)
        return;

    auto& ext = context.extensions;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Chrome is opaque
    glDisable(GL_BLEND);

    // Back-face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    shader_->use();

    // Set matrix uniforms - apply Y offset for stereo separation
    // Scale amplitude by halfHeight to map normalized amplitude to world space
    Matrix4 model = Matrix4::translation(0, data.yOffset * halfHeight, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude * halfHeight, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    // Camera position
    Vec3 camPos = camera.getPosition();
    ext.glUniform3f(cameraPosLoc_, camPos.x, camPos.y, camPos.z);

    // Color tint
    ext.glUniform4f(colorLoc_,
                    data.color.getFloatRed(),
                    data.color.getFloatGreen(),
                    data.color.getFloatBlue(),
                    data.color.getFloatAlpha());

    // Material properties
    ext.glUniform1f(metallicLoc_, material_.metallic);
    ext.glUniform1f(roughnessLoc_, material_.roughness);
    ext.glUniform1f(fresnelPowerLoc_, material_.fresnelPower);

    // Animation
    ext.glUniform1f(timeLoc_, time_);
    ext.glUniform1f(rippleFreqLoc_, rippleFrequency_);
    ext.glUniform1f(rippleAmpLoc_, rippleAmplitude_);

    // Light
    float lx = lighting.lightDirX;
    float ly = lighting.lightDirY;
    float lz = lighting.lightDirZ;
    float lLen = std::sqrt(lx*lx + ly*ly + lz*lz);
    if (lLen > 0) { lx /= lLen; ly /= lLen; lz /= lLen; }
    ext.glUniform3f(lightDirLoc_, lx, ly, lz);
    ext.glUniform4f(specularLoc_, lighting.specularR, lighting.specularG,
                    lighting.specularB, lighting.specularPower);

    // Bind environment map
    if (hasEnvironmentMap())
    {
        bindEnvironmentMap(ext, 0, envMapLoc_);
    }

    // Draw
    ext.glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    ext.glBindVertexArray(0);

    glDisable(GL_CULL_FACE);
}

void LiquidChromeShader::update(float deltaTime)
{
    time_ += deltaTime * flowSpeed_;
    if (time_ > 1000.0f)
        time_ = std::fmod(time_, 1000.0f);
}

void LiquidChromeShader::updateMesh(const WaveformData3D& data, float xSpread)
{
    // Generate tube mesh
    generateTubeMesh(data.samples, data.sampleCount, xSpread, tubeRadius_, 16,
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

void LiquidChromeShader::setParameter(const juce::String& name, float value)
{
    if (name == "rippleFrequency")
        rippleFrequency_ = value;
    else if (name == "rippleAmplitude")
        rippleAmplitude_ = value;
    else if (name == "rippleSpeed")
        rippleSpeed_ = value;
    else if (name == "flowSpeed")
        flowSpeed_ = value;
    else if (name == "tubeRadius")
        tubeRadius_ = value;
    else if (name == "metallic")
        material_.metallic = value;
    else if (name == "roughness")
        material_.roughness = value;
}

float LiquidChromeShader::getParameter(const juce::String& name) const
{
    if (name == "rippleFrequency")
        return rippleFrequency_;
    if (name == "rippleAmplitude")
        return rippleAmplitude_;
    if (name == "rippleSpeed")
        return rippleSpeed_;
    if (name == "flowSpeed")
        return flowSpeed_;
    if (name == "tubeRadius")
        return tubeRadius_;
    if (name == "metallic")
        return material_.metallic;
    if (name == "roughness")
        return material_.roughness;
    return 0.0f;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
