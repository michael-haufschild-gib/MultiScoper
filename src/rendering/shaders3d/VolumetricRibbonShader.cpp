/*
    Oscil - Volumetric Ribbon Shader Implementation
*/

#include "rendering/shaders3d/VolumetricRibbonShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

static const char* ribbonVertexShader = R"(
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

static const char* ribbonFragmentShader = R"(
    #version 330 core
    uniform vec4 uColor;
    uniform vec3 uLightDir;
    uniform vec3 uAmbient;
    uniform vec3 uDiffuse;
    uniform vec4 uSpecular;
    uniform float uSpecPower;
    uniform float uTime;
    uniform float uGlowIntensity;
    uniform vec3 uCameraPos;
    uniform sampler2D uNoiseTexture;

    in vec3 vWorldPos;
    in vec3 vNormal;
    in vec2 vTexCoord;

    out vec4 fragColor;

    void main()
    {
        vec3 normal = normalize(vNormal);
        vec3 lightDir = normalize(uLightDir);
        vec3 viewDir = normalize(uCameraPos - vWorldPos);

        // Diffuse lighting
        float diff = max(dot(normal, lightDir), 0.0);

        // Specular lighting (Blinn-Phong)
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), uSpecPower);

        // Fresnel-like rim lighting for glow effect
        float rim = 1.0 - max(dot(viewDir, normal), 0.0);
        rim = pow(rim, 2.0) * uGlowIntensity;

        // Flowing noise effect
        // Scroll UVs along the tube length (x)
        vec2 flowUV = vec2(vTexCoord.x * 2.0 - uTime * 0.5, vTexCoord.y);
        float noise = texture(uNoiseTexture, flowUV).r;
        
        // Modulate pulse with noise
        // Mix between smooth pulse and noisy flow based on glow intensity (higher glow = more turbulent)
        float pulseBase = sin(vTexCoord.x * 20.0 - uTime * 3.0) * 0.5 + 0.5;
        float flow = mix(pulseBase, noise, 0.7); 
        
        // Enhance contrast
        flow = pow(flow, 1.5);
        
        // Combine lighting
        vec3 ambient = uAmbient * uColor.rgb;
        vec3 diffuse = uDiffuse * diff * uColor.rgb;
        vec3 specular = uSpecular.rgb * spec * uSpecular.a;
        vec3 glow = uColor.rgb * rim;

        vec3 result = (ambient + diffuse + specular + glow) * flow;

        // Add core brightness
        float coreBrightness = 1.0 + uGlowIntensity * 0.5;
        result *= coreBrightness;

        fragColor = vec4(result, uColor.a);
    }
)";

VolumetricRibbonShader::VolumetricRibbonShader() = default;

VolumetricRibbonShader::~VolumetricRibbonShader() = default;

bool VolumetricRibbonShader::compile(juce::OpenGLContext& context)
{
    if (compiled_)
        return true;

    auto& ext = context.extensions;

    // Compile shader
    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, ribbonVertexShader, ribbonFragmentShader))
    {
        DBG("VolumetricRibbonShader: Failed to compile shader");
        shader_.reset();
        return false;
    }

    // Get uniform locations
    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    lightDirLoc_ = shader_->getUniformIDFromName("uLightDir");
    ambientLoc_ = shader_->getUniformIDFromName("uAmbient");
    diffuseLoc_ = shader_->getUniformIDFromName("uDiffuse");
    specularLoc_ = shader_->getUniformIDFromName("uSpecular");
    specPowerLoc_ = shader_->getUniformIDFromName("uSpecPower");
    timeLoc_ = shader_->getUniformIDFromName("uTime");
    glowIntensityLoc_ = shader_->getUniformIDFromName("uGlowIntensity");
    cameraPosLoc_ = shader_->getUniformIDFromName("uCameraPos");
    noiseTextureLoc_ = shader_->getUniformIDFromName("uNoiseTexture");

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
    DBG("VolumetricRibbonShader: Compiled successfully");
    return true;
}

void VolumetricRibbonShader::release(juce::OpenGLContext& context)
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

void VolumetricRibbonShader::render(juce::OpenGLContext& context,
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

    // Update mesh with current waveform data
    updateMesh(data, xSpread);

    if (indexCount_ == 0)
        return;

    auto& ext = context.extensions;

    // Enable depth testing for 3D
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable blending for glow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_->use();

    // Set matrix uniforms - apply Y offset for stereo separation and Z offset for depth
    // Scale amplitude by halfHeight to map normalized amplitude to world space
    Matrix4 model = Matrix4::translation(0, data.yOffset * halfHeight, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude * halfHeight, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    // Set color
    ext.glUniform4f(colorLoc_,
                    data.color.getFloatRed(),
                    data.color.getFloatGreen(),
                    data.color.getFloatBlue(),
                    data.color.getFloatAlpha());

    // Set lighting
    setLightingUniforms(ext, lightDirLoc_, ambientLoc_, diffuseLoc_,
                        specularLoc_, specPowerLoc_, lighting);

    // Set additional uniforms
    ext.glUniform1f(timeLoc_, time_);
    ext.glUniform1f(glowIntensityLoc_, glowIntensity_);

    Vec3 camPos = camera.getPosition();
    ext.glUniform3f(cameraPosLoc_, camPos.x, camPos.y, camPos.z);

    // Bind noise texture
    if (noiseTextureID_ != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, noiseTextureID_);
        ext.glUniform1i(noiseTextureLoc_, 0);
    }

    // Bind VAO and draw
    ext.glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    ext.glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void VolumetricRibbonShader::update(float deltaTime)
{
    time_ += deltaTime * pulseSpeed_;
    if (time_ > 1000.0f)
        time_ = std::fmod(time_, 1000.0f);
}

void VolumetricRibbonShader::updateMesh(const WaveformData3D& data, float xSpread)
{
    // Generate tube mesh
    generateTubeMesh(data.samples, data.sampleCount, xSpread, tubeRadius_, tubeSegments_,
                     vertices_, indices_);

    if (vertices_.empty() || indices_.empty())
    {
        indexCount_ = 0;
        return;
    }

    // Upload to GPU (reusing existing buffers)
    auto* ext = juce::OpenGLContext::getCurrentContext();
    if (!ext)
        return;

    auto& extensions = ext->extensions;

    extensions.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    extensions.glBufferData(GL_ARRAY_BUFFER,
                            static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
                            vertices_.data(), GL_DYNAMIC_DRAW);

    extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                            static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)),
                            indices_.data(), GL_DYNAMIC_DRAW);

    indexCount_ = static_cast<int>(indices_.size());
}

void VolumetricRibbonShader::setParameter(const juce::String& name, float value)
{
    if (name == "tubeRadius")
        tubeRadius_ = value;
    else if (name == "glowIntensity")
        glowIntensity_ = value;
    else if (name == "pulseSpeed")
        pulseSpeed_ = value;
}

float VolumetricRibbonShader::getParameter(const juce::String& name) const
{
    if (name == "tubeRadius")
        return tubeRadius_;
    if (name == "glowIntensity")
        return glowIntensity_;
    if (name == "pulseSpeed")
        return pulseSpeed_;
    return 0.0f;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
