/*
    Oscil - Plasma Sine Shader Implementation
*/

#include "rendering/shaders/PlasmaSineShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

// =============================================================================
// SHADER DEFINITIONS
// =============================================================================

// Vertex Shader with Noise-Based Displacement (Jitter)
static const char* plasmaVertexShader = R"(
    #version 330 core
    in vec2 position;
    in float vParam; // -1 to 1 across width
    in float tParam; // 0 to 1 along length

    uniform mat4 projection;
    uniform float time;
    uniform float jitterAmount;
    uniform float jitterSpeed;

    out vec2 vPos;
    out float vV;
    out float vT;

    // Simple pseudo-random noise
    float hash(vec2 p) {
        return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    }

    float noise(vec2 p) {
        vec2 i = floor(p);
        vec2 f = fract(p);
        f = f * f * (3.0 - 2.0 * f);
        return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), f.x),
                   mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
    }

    void main()
    {
        vec2 pos = position;
        vPos = pos;
        vV = vParam;
        vT = tParam;

        // Apply Jitter (Vertex Displacement)
        // We displace based on the 'tParam' (position along wave) so the whole
        // cross-section moves together (keeping the ribbon intact but wavy).
        if (jitterAmount > 0.0) {
            float n = noise(vec2(tParam * 10.0, time * jitterSpeed));
            // Map 0..1 to -1..1
            float displacement = (n - 0.5) * 2.0;
            
            // Displace perpendicular to the wave? 
            // For simplicity, just vertical displacement is often enough for audio waves,
            // but 2D displacement is better.
            // Let's just add to Y for now as it aligns with the audio amplitude.
            pos.y += displacement * jitterAmount;
        }

        gl_Position = projection * vec4(pos, 0.0, 1.0);
    }
)";

static const char* plasmaFragmentShader = R"(
    #version 330 core
    in vec2 vPos;
    in float vV;
    in float vT;

    uniform vec4 baseColor;
    uniform float opacity;
    uniform float time;
    uniform int passIndex; // 0=Haze, 1=Arcs, 2=Core

    out vec4 fragColor;

    // High Quality Noise for Texture
    vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

    float snoise(vec2 v) {
        const vec4 C = vec4(0.211324865405187, 0.366025403784439,
                 -0.577350269189626, 0.024390243902439);
        vec2 i  = floor(v + dot(v, C.yy) );
        vec2 x0 = v -   i + dot(i, C.xx);
        vec2 i1;
        i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
        vec4 x12 = x0.xyxy + C.xxzz;
        x12.xy -= i1;
        i = mod289(i);
        vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
        + i.x + vec3(0.0, i1.x, 1.0 ));
        vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
        m = m*m ;
        m = m*m ;
        vec3 x = 2.0 * fract(p * C.www) - 1.0;
        vec3 h = abs(x) - 0.5;
        vec3 ox = floor(x + 0.5);
        vec3 a0 = x - ox;
        m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
        vec3 g;
        g.x  = a0.x  * x0.x  + h.x  * x0.y;
        g.yz = a0.yz * x12.xz + h.yz * x12.yw;
        return 130.0 * dot(m, g);
    }

    float fbm(vec2 p) {
        float f = 0.0;
        float w = 0.5;
        for (int i = 0; i < 3; i++) {
            f += w * snoise(p);
            p *= 2.0;
            w *= 0.5;
        }
        return f;
    }

    void main()
    {
        float dist = abs(vV);
        
        // Soft edge for all layers to avoid hard polygon look
        float edgeFade = pow(max(1.0 - dist, 0.0), 2.0);
        
        // Waveform end fades
        float endFade = smoothstep(0.0, 0.05, vT) * (1.0 - smoothstep(0.95, 1.0, vT));

        vec4 finalColor = vec4(0.0);

        // --- PASS 0: HAZE (Volumetric Glow) ---
        if (passIndex == 0) {
            // Slow flowing smoke
            float n = fbm(vec2(vT * 5.0 - time * 0.2, vV * 2.0));
            n = n * 0.5 + 0.5; // 0..1
            
            vec3 col = baseColor.rgb * 0.8; // Deep base color
            float a = 0.1 * n * edgeFade; // Low opacity
            finalColor = vec4(col * a, a);
        }
        
        // --- PASS 1: ARCS (Electric Jitter) ---
        else if (passIndex == 1) {
            // High contrast electric texture
            float n = snoise(vec2(vT * 20.0 - time * 1.0, vV * 5.0));
            float spark = 1.0 - abs(n);
            spark = pow(spark, 4.0); // Sharp lines
            
            vec3 col = mix(baseColor.rgb, vec3(1.0), 0.5); // Brighter
            float a = 0.6 * spark * edgeFade;
            finalColor = vec4(col * a, a);
        }
        
        // --- PASS 2: CORE (Solid Beam) ---
        else if (passIndex == 2) {
            // Solid core with glow
            float core = smoothstep(0.6, 0.0, dist); // Sharp center
            vec3 col = vec3(1.0); // White
            float a = core * 0.9;
            finalColor = vec4(col * a, a);
        }

        fragColor = finalColor * opacity * endFade;
    }
)";

struct PlasmaSineShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
    GLint timeLoc = -1;
    GLint passIndexLoc = -1;
    GLint jitterAmountLoc = -1;
    GLint jitterSpeedLoc = -1;
};
#endif

PlasmaSineShader::PlasmaSineShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

PlasmaSineShader::~PlasmaSineShader() = default;

#if OSCIL_ENABLE_OPENGL
bool PlasmaSineShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled) return true;

    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);
    
    if (!gl_->program->addVertexShader(plasmaVertexShader) || 
        !gl_->program->addFragmentShader(plasmaFragmentShader) || 
        !gl_->program->link())
    {
        gl_->program.reset();
        return false;
    }

    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");
    gl_->timeLoc = gl_->program->getUniformIDFromName("time");
    gl_->passIndexLoc = gl_->program->getUniformIDFromName("passIndex");
    gl_->jitterAmountLoc = gl_->program->getUniformIDFromName("jitterAmount");
    gl_->jitterSpeedLoc = gl_->program->getUniformIDFromName("jitterSpeed");

    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void PlasmaSineShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled) return;
    context.extensions.glDeleteBuffers(1, &gl_->vbo);
    context.extensions.glDeleteVertexArrays(1, &gl_->vao);
    gl_->program.reset();
    gl_->compiled = false;
}

bool PlasmaSineShader::isCompiled() const
{
    return gl_->compiled;
}

void PlasmaSineShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2) return;

    auto& ext = context.extensions;
    
    // Additive Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); 
    glDisable(GL_CULL_FACE);

    gl_->program->use();

    auto* target = context.getTargetComponent();
    if (!target) return;

    float w = static_cast<float>(target->getWidth());
    float h = static_cast<float>(target->getHeight());

    float projection[16] = {
        2.0f / w, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);
    ext.glUniform1f(gl_->opacityLoc, params.opacity);
    ext.glUniform1f(gl_->timeLoc, params.time);

    // Geometry Config
    float height = params.bounds.getHeight();
    float centerY1, centerY2;
    float amp1, amp2;

    if (params.isStereo && channel2 != nullptr)
    {
        float halfHeight = height * 0.5f;
        centerY1 = params.bounds.getY() + halfHeight * 0.5f;
        centerY2 = params.bounds.getY() + halfHeight * 1.5f;
        amp1 = halfHeight * 0.45f * params.verticalScale;
        amp2 = halfHeight * 0.45f * params.verticalScale;
    }
    else
    {
        centerY1 = params.bounds.getCentreY();
        centerY2 = centerY1;
        amp1 = height * 0.45f * params.verticalScale;
        amp2 = amp1;
    }

    // Attributes
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    GLint posLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "position");
    GLint vLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "vParam");
    GLint tLoc = ext.glGetAttribLocation(gl_->program->getProgramID(), "tParam");

    if (posLoc >= 0) {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(posLoc), 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    }
    if (vLoc >= 0) {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(vLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(vLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    if (tLoc >= 0) {
        ext.glEnableVertexAttribArray(static_cast<GLuint>(tLoc));
        ext.glVertexAttribPointer(static_cast<GLuint>(tLoc), 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    // Helper: Draw Jittered Layers
    auto drawChannel = [&](const std::vector<float>& channel, float centerY, float amp) {
        std::vector<float> vertices;
        
        // --- LAYER 1: OUTER HAZE (Static, Wide) ---
        float hazeWidth = 250.0f * params.verticalScale;
        buildLineGeometry(vertices, channel, centerY, amp, hazeWidth, params.bounds.getX(), params.bounds.getWidth());
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        ext.glUniform1i(gl_->passIndexLoc, 0);
        ext.glUniform1f(gl_->jitterAmountLoc, 0.0f); // No jitter for haze
        ext.glUniform4f(gl_->baseColorLoc, params.colour.getFloatRed(), params.colour.getFloatGreen(), params.colour.getFloatBlue(), params.colour.getFloatAlpha());
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

        // --- LAYER 2: ELECTRIC ARCS (High Jitter, Medium Width) ---
        vertices.clear();
        float arcWidth = 80.0f * params.verticalScale;
        buildLineGeometry(vertices, channel, centerY, amp, arcWidth, params.bounds.getX(), params.bounds.getWidth());
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        ext.glUniform1i(gl_->passIndexLoc, 1);
        ext.glUniform1f(gl_->jitterAmountLoc, 30.0f * params.verticalScale); // Heavy Jitter
        ext.glUniform1f(gl_->jitterSpeedLoc, 5.0f);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));

        // --- LAYER 3: CORE (No Jitter, Thin) ---
        vertices.clear();
        float coreWidth = 6.0f * params.verticalScale;
        buildLineGeometry(vertices, channel, centerY, amp, coreWidth, params.bounds.getX(), params.bounds.getWidth());
        ext.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_DYNAMIC_DRAW);
        
        ext.glUniform1i(gl_->passIndexLoc, 2);
        ext.glUniform1f(gl_->jitterAmountLoc, 0.0f); // Stable core
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 4));
    };

    // Render Channels
    drawChannel(channel1, centerY1, amp1);

    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        drawChannel(*channel2, centerY2, amp2);
    }

    if (posLoc >= 0) ext.glDisableVertexAttribArray(static_cast<GLuint>(posLoc));
    if (vLoc >= 0) ext.glDisableVertexAttribArray(static_cast<GLuint>(vLoc));
    if (tLoc >= 0) ext.glDisableVertexAttribArray(static_cast<GLuint>(tLoc));

    ext.glBindVertexArray(0);
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_BLEND);
}
#endif

} // namespace oscil