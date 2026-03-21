/*
    Oscil - Electric Flower Shader Implementation
    Based on webglsamples.org/electricflower demo technique
*/

#include "rendering/shaders3d/ElectricFlowerShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Vertex shader - preserves waveform geometry, applies Electric Flower visual style
// Uses irrational constants for organic color/size animation without distorting shape
static const char* electricFlowerVertexShader = R"(
    #version 330 core
    in vec3 aPosition;
    in float aIndex;    // Normalized index (0-1) for color interpolation
    in float aAmplitude; // Original waveform amplitude for visual effects

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;
    uniform float uTime;
    uniform float uPointSize;

    out float vIndex;
    out float vAmplitude;
    out float vPulse;

    // Irrational constants for organic animation
    const float PHI = 1.6180339887;
    const float SQRT2 = 1.4142135623;
    const float EULER = 2.7182818284;

    void main()
    {
        // Preserve waveform geometry - no rotations!
        vec3 pos = aPosition;

        vec4 worldPos = uModel * vec4(pos, 1.0);
        gl_Position = uProjection * uView * worldPos;

        // Scale point size based on distance for depth perception
        vec4 viewPos = uView * worldPos;
        float dist = length(viewPos.xyz);

        // Use irrational multipliers for organic pulsing animation
        // Each point pulses slightly differently based on position
        float pulse1 = sin(uTime * PHI + aIndex * 6.283185);
        float pulse2 = sin(uTime * SQRT2 * 0.7 + aIndex * 6.283185 * PHI);
        float pulse3 = sin(uTime * EULER * 0.5 + aIndex * 6.283185 * SQRT2);
        float combinedPulse = (pulse1 + pulse2 + pulse3) / 3.0;

        // Amplitude affects point size - louder = bigger points
        float ampScale = 1.0 + abs(aAmplitude) * 0.5;
        float pulseScale = 1.0 + combinedPulse * 0.2;

        gl_PointSize = uPointSize * ampScale * pulseScale * (5.0 / max(dist, 0.1));

        vIndex = aIndex;
        vAmplitude = aAmplitude;
        vPulse = combinedPulse;
    }
)";

// Fragment shader with amplitude-driven color effects
static const char* electricFlowerFragmentShader = R"(
    #version 330 core
    uniform vec4 uColor;     // Base color from waveform
    uniform vec4 uColor2;    // Derived secondary color
    uniform float uGlowIntensity;
    uniform float uTime;

    in float vIndex;
    in float vAmplitude;
    in float vPulse;
    out vec4 fragColor;

    // Irrational constants for organic color animation
    const float PHI = 1.6180339887;
    const float SQRT2 = 1.4142135623;

    // Convert sRGB to linear color space for correct rendering pipeline
    vec3 sRGBToLinear(vec3 srgb) {
        return pow(srgb, vec3(2.2));
    }

    void main()
    {
        // Create soft point with smooth edge falloff
        vec2 coord = gl_PointCoord - vec2(0.5);
        float dist = length(coord);

        // Soft circle falloff
        float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

        // Use amplitude to drive color interpolation
        // High amplitude = more color2, creating dynamic color shifts with waveform
        float ampFactor = abs(vAmplitude);
        float colorMix = mix(vIndex, ampFactor, 0.5);

        // Add time-based color cycling using irrational multipliers
        float colorCycle = sin(uTime * PHI * 0.3 + vIndex * 6.283185) * 0.2 + 0.5;
        colorMix = mix(colorMix, colorCycle, 0.3);

        // Convert colors to linear for correct blending
        vec3 linearColor1 = sRGBToLinear(uColor.rgb);
        vec3 linearColor2 = sRGBToLinear(uColor2.rgb);
        vec3 color = mix(linearColor1, linearColor2, colorMix);

        // Amplitude affects brightness - louder = brighter
        float brightness = 1.0 + ampFactor * 0.8;
        color *= brightness;

        // Add glow intensity
        color *= uGlowIntensity;

        // Add pulse-driven glow variation
        color *= 1.0 + vPulse * 0.15;

        // Add bright core
        float core = 1.0 - smoothstep(0.0, 0.3, dist);
        color += color * core * 0.6;

        // Amplitude affects alpha - louder parts are more visible
        float alphaBoost = 0.7 + ampFactor * 0.3;

        fragColor = vec4(color, alpha * uColor.a * alphaBoost);
    }
)";

ElectricFlowerShader::ElectricFlowerShader() = default;

ElectricFlowerShader::~ElectricFlowerShader()
{
#if OSCIL_ENABLE_OPENGL
    if (compiled_)
    {
        DBG("[ElectricFlowerShader] LEAK DETECTED: Destructor called without release()");
    }
#endif
}

bool ElectricFlowerShader::compile(juce::OpenGLContext& context)
{
    if (compiled_) return true;

    auto& ext = context.extensions;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, electricFlowerVertexShader, electricFlowerFragmentShader))
    {
        shader_.reset();
        return false;
    }

    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    color2Loc_ = shader_->getUniformIDFromName("uColor2");
    timeLoc_ = shader_->getUniformIDFromName("uTime");
    pointSizeLoc_ = shader_->getUniformIDFromName("uPointSize");
    glowIntensityLoc_ = shader_->getUniformIDFromName("uGlowIntensity");

    ext.glGenVertexArrays(1, &vao_);
    ext.glBindVertexArray(vao_);
    ext.glGenBuffers(1, &vbo_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Layout: position(3) + index(1) + amplitude(1), stride = 5 floats
    GLint posAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aPosition");
    GLint indexAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aIndex");
    GLint ampAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aAmplitude");
    const int stride = 5 * sizeof(float);

    if (posAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(posAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(posAttrib), 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    }
    if (indexAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(indexAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(indexAttrib), 1, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void*>(3 * sizeof(float)));
    }
    if (ampAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(ampAttrib));
        ext.glVertexAttribPointer(static_cast<GLuint>(ampAttrib), 1, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void*>(4 * sizeof(float)));
    }

    ext.glBindVertexArray(0);
    compiled_ = true;
    return true;
}

void ElectricFlowerShader::release(juce::OpenGLContext& context)
{
    if (!compiled_)
        return;

    auto& ext = context.extensions;

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

bool ElectricFlowerShader::isCompiled() const
{
    return compiled_;
}

void ElectricFlowerShader::render(juce::OpenGLContext& context,
                                   const WaveformData3D& data,
                                   const Camera3D& camera,
                                   const LightingConfig& lighting)
{
    juce::ignoreUnused(lighting);

    if (!compiled_ || !data.samples || data.sampleCount < 2)
        return;

    float xSpread, halfHeight;
    calculateCameraSpread(camera, xSpread, halfHeight);

    updateVertices(data, xSpread);

    if (vertexCount_ == 0)
        return;

    auto& ext = context.extensions;

    // Enable point rendering features
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow

    // Disable depth testing for additive effect
    glDisable(GL_DEPTH_TEST);

    shader_->use();

    // Set matrix uniforms with Y/Z offsets
    Matrix4 model = Matrix4::translation(0, data.yOffset * halfHeight, data.zOffset) *
                    Matrix4::scale(1.0f, data.amplitude * halfHeight, 1.0f);
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    // Set base color from waveform
    ext.glUniform4f(colorLoc_,
                    data.color.getFloatRed(),
                    data.color.getFloatGreen(),
                    data.color.getFloatBlue(),
                    data.color.getFloatAlpha());

    // Calculate secondary color (complementary/shifted hue)
    float h, s, l;
    data.color.getHSL(h, s, l);
    h = std::fmod(h + colorShift_, 1.0f);  // Shift hue
    l = std::min(l * 1.2f, 1.0f);          // Slightly brighter
    juce::Colour color2 = juce::Colour::fromHSL(h, s, l, data.color.getFloatAlpha());

    ext.glUniform4f(color2Loc_,
                    color2.getFloatRed(),
                    color2.getFloatGreen(),
                    color2.getFloatBlue(),
                    color2.getFloatAlpha());

    // Set animation time and other uniforms
    ext.glUniform1f(timeLoc_, time_);
    ext.glUniform1f(pointSizeLoc_, pointSize_);
    ext.glUniform1f(glowIntensityLoc_, glowIntensity_);

    // Bind VAO and draw points
    ext.glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, vertexCount_);

    // Also draw as line strip for connected look
    glDrawArrays(GL_LINE_STRIP, 0, vertexCount_);

    ext.glBindVertexArray(0);

    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void ElectricFlowerShader::update(float deltaTime)
{
    time_ += deltaTime * rotationSpeed_;
    if (time_ > 1000.0f)
        time_ = std::fmod(time_, 1000.0f);
}

void ElectricFlowerShader::updateVertices(const WaveformData3D& data, float xSpread)
{
    // Generate vertex data from waveform samples
    // Each vertex: position (x, y, z) + normalized index + amplitude

    vertices_.clear();

    // Guard against division by zero (need at least 2 samples for interpolation)
    if (data.sampleCount < 2)
        return;

    vertices_.reserve(static_cast<size_t>(data.sampleCount) * 5);

    for (int i = 0; i < data.sampleCount; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(data.sampleCount - 1);

        // X position spans the view width
        float x = (t * 2.0f - 1.0f) * xSpread;

        // Y position from waveform sample - this is the actual waveform shape
        float y = data.samples[i];

        // Z position stays flat to preserve waveform geometry
        float z = 0.0f;

        // Position (preserves waveform shape)
        vertices_.push_back(x);
        vertices_.push_back(y);
        vertices_.push_back(z);

        // Normalized index for color interpolation
        vertices_.push_back(t);

        // Original amplitude for visual effects (brightness, size, color)
        vertices_.push_back(y);
    }

    // Upload to GPU
    auto* ctx = juce::OpenGLContext::getCurrentContext();
    if (!ctx)
        return;

    auto& ext = ctx->extensions;

    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
                     vertices_.data(), GL_DYNAMIC_DRAW);

    vertexCount_ = data.sampleCount;
}

void ElectricFlowerShader::setParameter(const juce::String& name, float value)
{
    if (name == "rotationSpeed")
        rotationSpeed_ = value;
    else if (name == "pointSize")
        pointSize_ = value;
    else if (name == "glowIntensity")
        glowIntensity_ = value;
    else if (name == "colorShift")
        colorShift_ = value;
}

float ElectricFlowerShader::getParameter(const juce::String& name) const
{
    if (name == "rotationSpeed")
        return rotationSpeed_;
    if (name == "pointSize")
        return pointSize_;
    if (name == "glowIntensity")
        return glowIntensity_;
    if (name == "colorShift")
        return colorShift_;
    return 0.0f;
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
