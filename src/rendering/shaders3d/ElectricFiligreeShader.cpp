/*
    Oscil - Electric Filigree Shader Implementation
*/

#include "rendering/shaders3d/ElectricFiligreeShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

using namespace juce::gl;

// Vertex shader with passthrough
static const char* filigreeVertexShader = R"(
    #version 330 core
    in vec3 aPosition;
    in vec3 aNormal;
    in vec2 aTexCoord;

    uniform mat4 uModel;
    uniform mat4 uView;
    uniform mat4 uProjection;

    out vec3 vWorldPos;
    out vec2 vTexCoord;

    void main()
    {
        vec4 worldPos = uModel * vec4(aPosition, 1.0);
        vWorldPos = worldPos.xyz;
        vTexCoord = aTexCoord;
        gl_Position = uProjection * uView * worldPos;
    }
)";

// Fragment shader with Ridged Multifractal Noise
static const char* filigreeFragmentShader = R"(
    #version 330 core
    
    uniform vec4 uColor;
    uniform float uTime;
    uniform float uNoiseScale;
    uniform float uNoiseSpeed;
    uniform float uBranchIntensity;

    in vec3 vWorldPos;
    in vec2 vTexCoord;

    out vec4 fragColor;

    // Simplex noise 3D implementation
    vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }
    vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

    float snoise(vec3 v)
    {
        const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
        const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

        // First corner
        vec3 i  = floor(v + dot(v, C.yyy) );
        vec3 x0 = v - i + dot(i, C.xxx) ;

        // Other corners
        vec3 g = step(x0.yzx, x0.xyz);
        vec3 l = 1.0 - g;
        vec3 i1 = min( g.xyz, l.zxy );
        vec3 i2 = max( g.xyz, l.zxy );

        //   x0 = x0 - 0.0 + 0.0 * C.xxx;
        //   x1 = x0 - i1  + 1.0 * C.xxx;
        //   x2 = x0 - i2  + 2.0 * C.xxx;
        //   x3 = x0 - 1.0 + 3.0 * C.xxx;
        vec3 x1 = x0 - i1 + C.xxx;
        vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
        vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

        // Permutations
        i = mod289(i);
        vec4 p = permute( permute( permute(
                    i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
                  + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
                  + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

        // Gradients: 7x7 points over a square, mapped onto an octahedron.
        // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
        float n_ = 0.142857142857; // 1.0/7.0
        vec3  ns = n_ * D.wyz - D.xzx;

        vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

        vec4 x_ = floor(j * ns.z);
        vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

        vec4 x = x_ *ns.x + ns.yyyy;
        vec4 y = y_ *ns.x + ns.yyyy;
        vec4 h = 1.0 - abs(x) - abs(y);

        vec4 b0 = vec4( x.xy, y.xy );
        vec4 b1 = vec4( x.zw, y.zw );

        //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
        //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
        vec4 s0 = floor(b0)*2.0 + 1.0;
        vec4 s1 = floor(b1)*2.0 + 1.0;
        vec4 sh = -step(h, vec4(0.0));

        vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
        vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

        vec3 p0 = vec3(a0.xy,h.x);
        vec3 p1 = vec3(a0.zw,h.y);
        vec3 p2 = vec3(a1.xy,h.z);
        vec3 p3 = vec3(a1.zw,h.w);

        //Normalise gradients
        vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
        p0 *= norm.x;
        p1 *= norm.y;
        p2 *= norm.z;
        p3 *= norm.w;

        // Mix final noise value
        vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
        m = m * m;
        return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                      dot(p2,x2), dot(p3,x3) ) );
    }

    // Ridged Multifractal
    // 1.0 - abs(noise) produces sharp valleys.
    // Inverted, it produces sharp ridges (lightning/branches).
    float ridgedNoise(vec3 p)
    {
        float n = snoise(p);
        return 1.0 - abs(n);
    }

    void main()
    {
        // Coordinate for noise
        // Use UV for ribbon-space stability, but mix world Y for chaos
        vec3 p = vec3(vTexCoord.x * 10.0, vTexCoord.y * 2.0, uTime * uNoiseSpeed);
        p.y += vWorldPos.y * 2.0; 
        
        float n = ridgedNoise(p * uNoiseScale);
        
        // Sharpen the ridges to make thin lines
        float electric = pow(n, 16.0); // Higher power = thinner lines
        
        // Add a second layer for detail
        float n2 = ridgedNoise(p * uNoiseScale * 2.5 + 10.0);
        float electric2 = pow(n2, 8.0) * 0.5;
        
        float intensity = electric + electric2 * uBranchIntensity;
        
        // Core beam
        float core = 1.0 - abs(vTexCoord.y - 0.5) * 2.0;
        core = pow(core, 4.0);
        
        float combined = core + intensity * 2.0;
        
        fragColor = uColor * combined;
        fragColor.a = uColor.a * clamp(combined, 0.0, 1.0);
    }
)";

ElectricFiligreeShader::ElectricFiligreeShader() = default;
ElectricFiligreeShader::~ElectricFiligreeShader() = default;

bool ElectricFiligreeShader::compile(juce::OpenGLContext& context)
{
    if (compiled_) return true;

    auto& ext = context.extensions;

    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!compileShaderProgram(*shader_, filigreeVertexShader, filigreeFragmentShader))
    {
        DBG("ElectricFiligreeShader: Compile error");
        shader_.reset();
        return false;
    }

    modelLoc_ = shader_->getUniformIDFromName("uModel");
    viewLoc_ = shader_->getUniformIDFromName("uView");
    projLoc_ = shader_->getUniformIDFromName("uProjection");
    colorLoc_ = shader_->getUniformIDFromName("uColor");
    timeLoc_ = shader_->getUniformIDFromName("uTime");
    noiseScaleLoc_ = shader_->getUniformIDFromName("uNoiseScale");
    noiseSpeedLoc_ = shader_->getUniformIDFromName("uNoiseSpeed");
    branchIntensityLoc_ = shader_->getUniformIDFromName("uBranchIntensity");

    // Create buffers
    ext.glGenVertexArrays(1, &vao_);
    ext.glBindVertexArray(vao_);
    ext.glGenBuffers(1, &vbo_);
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glGenBuffers(1, &ibo_);
    ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

    // Attributes
    GLint posAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aPosition");
    GLint normAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aNormal");
    GLint texAttrib = ext.glGetAttribLocation(shader_->getProgramID(), "aTexCoord");

    const int stride = 8 * sizeof(float);
    if (posAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(posAttrib);
        ext.glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    }
    if (normAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(normAttrib);
        ext.glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    }
    if (texAttrib >= 0)
    {
        ext.glEnableVertexAttribArray(texAttrib);
        ext.glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    ext.glBindVertexArray(0);
    compiled_ = true;
    return true;
}

void ElectricFiligreeShader::release(juce::OpenGLContext& context)
{
    if (!compiled_) return;
    auto& ext = context.extensions;
    ext.glDeleteBuffers(1, &vbo_);
    ext.glDeleteBuffers(1, &ibo_);
    ext.glDeleteVertexArrays(1, &vao_);
    shader_.reset();
    compiled_ = false;
}

void ElectricFiligreeShader::render(juce::OpenGLContext& context,
                                    const WaveformData3D& data,
                                    const Camera3D& camera,
                                    const LightingConfig& lighting)
{
    if (!compiled_ || !data.samples || data.sampleCount < 2) return;

    float xSpread = 1.0f; // Adjust based on projection like other shaders
    
    // Use ribbon mesh generation (flat strip)
    // Generate a flat ribbon facing camera-ish
    // For simplicity, use generateRibbonMesh from base class
    
    if (camera.getProjection() == CameraProjection::Orthographic)
    {
        float height = camera.getOrthoSize();
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
    }
    else
    {
        float dist = (camera.getPosition() - camera.getTarget()).length();
        float fovRad = camera.getFOV() * 3.14159265f / 180.0f;
        float height = 2.0f * dist * std::tan(fovRad * 0.5f);
        float width = height * camera.getAspectRatio();
        xSpread = width * 0.5f;
    }

    // Update mesh
    generateRibbonMesh(data.samples, data.sampleCount, xSpread, 0.1f * data.amplitude, vertices_, indices_);
    
    if (vertices_.empty()) return;

    auto& ext = context.extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    ext.glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float), vertices_.data(), GL_DYNAMIC_DRAW);
    ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(GLuint), indices_.data(), GL_DYNAMIC_DRAW);
    indexCount_ = (int)indices_.size();

    // Render
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive for electric look
    glDisable(GL_DEPTH_TEST); // Electric arcs usually glow on top

    shader_->use();
    
    Matrix4 model = Matrix4::translation(0, data.yOffset, data.zOffset); // Simplified model matrix
    setMatrixUniforms(ext, modelLoc_, viewLoc_, projLoc_, camera, &model);

    ext.glUniform4f(colorLoc_, data.color.getFloatRed(), data.color.getFloatGreen(), data.color.getFloatBlue(), data.color.getFloatAlpha());
    ext.glUniform1f(timeLoc_, time_);
    ext.glUniform1f(noiseScaleLoc_, noiseScale_);
    ext.glUniform1f(noiseSpeedLoc_, noiseSpeed_);
    ext.glUniform1f(branchIntensityLoc_, branchIntensity_);

    ext.glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    ext.glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void ElectricFiligreeShader::update(float deltaTime)
{
    time_ += deltaTime;
}

void ElectricFiligreeShader::setParameter(const juce::String& name, float value)
{
    if (name == "scale") noiseScale_ = value;
    if (name == "speed") noiseSpeed_ = value;
    if (name == "branches") branchIntensity_ = value;
}

float ElectricFiligreeShader::getParameter(const juce::String& name) const
{
    if (name == "scale") return noiseScale_;
    if (name == "speed") return noiseSpeed_;
    if (name == "branches") return branchIntensity_;
    return 0.0f;
}

void ElectricFiligreeShader::updateMesh(const WaveformData3D& data, float xSpread)
{
    // Implemented inline in render for now
}

} // namespace oscil

#endif // OSCIL_ENABLE_OPENGL
