# Shader Development Guide for LLM Coding Agents

**Purpose**: Instructions for adding GPU-accelerated waveform shaders to Oscil.
**Tech Stack**: C++20, JUCE 8.0.5 OpenGL, GLSL 3.30 Core (no legacy translation)

## Quick Reference

```
Files to create:
  include/rendering/shaders/MyShader.h
  src/rendering/shaders/MyShader.cpp

Files to modify:
  src/rendering/ShaderRegistry.cpp  (register the shader)
  CMakeLists.txt                    (add source file)
```

## Architecture Overview

```
ShaderRegistry (singleton)
    ├── registerBuiltInShaders()     ← Register your shader here
    ├── compileAll(context)          ← Called when OpenGL ready
    └── releaseAll(context)          ← Called when context closing

WaveformShader (abstract base)
    ├── getId()                      ← Unique identifier
    ├── getDisplayName()             ← UI display name
    ├── getDescription()             ← Tooltip text
    ├── compile(context)             ← Initialize OpenGL resources
    ├── release(context)             ← Clean up resources
    ├── isCompiled()                 ← Check if ready
    ├── render(context, ...)         ← GPU rendering
    └── renderSoftware(g, ...)       ← CPU fallback (REQUIRED)

WaveformGLRenderer
    └── renderWaveform(data)         ← Calls your shader's render()
```

## JUCE 8 OpenGL Specifics

### GLSL Syntax Requirements

**CRITICAL**: Use modern **GLSL 3.30 Core** syntax. The project is configured for an OpenGL 3.2 Core Profile (or higher). Do **NOT** use legacy keywords like `attribute`, `varying`, or `gl_FragColor`. Do not rely on JUCE's translation helpers for new shaders.

```cpp
// CORRECT: Modern GLSL 3.30 Core syntax
static const char* vertexShader = R"(
    #version 330 core
    in vec2 position;              // 'in' for attributes
    in float myAttribute;
    
    uniform mat4 projection;
    
    out float vMyVarying;          // 'out' for varyings to fragment shader

    void main() {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vMyVarying = myAttribute;
    }
)";

static const char* fragmentShader = R"(
    #version 330 core
    in float vMyVarying;           // 'in' for varyings from vertex shader
    
    uniform vec4 baseColor;
    
    out vec4 fragColor;            // Custom output variable required

    void main() {
        fragColor = baseColor;     // Assign to output variable
    }
)";
```

### OpenGL 3.2 Core Profile (macOS)

macOS requires OpenGL 3.2 Core Profile, which mandates VAOs. Always:

1. Create VAO with `glGenVertexArrays()`
2. Bind VAO before any vertex operations
3. Delete VAO on release

```cpp
// In compile():
context.extensions.glGenVertexArrays(1, &gl_->vao);
context.extensions.glGenBuffers(1, &gl_->vbo);

// In render():
ext.glBindVertexArray(gl_->vao);
ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);
// ... vertex attribute setup and draw calls ...
ext.glBindVertexArray(0);

// In release():
ext.glDeleteBuffers(1, &gl_->vbo);
ext.glDeleteVertexArrays(1, &gl_->vao);
```

### Using JUCE's OpenGL Extensions

Access OpenGL functions through `context.extensions`:

```cpp
auto& ext = context.extensions;
ext.glGenVertexArrays(1, &vao);
ext.glBindBuffer(GL_ARRAY_BUFFER, vbo);
ext.glUniformMatrix4fv(loc, 1, GL_FALSE, matrix);
// etc.
```

For basic functions, use `juce::gl` namespace directly:

```cpp
using namespace juce::gl;
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE);
glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
```

## How to Add a New Shader

### Step 1: Create Header

Create `include/rendering/shaders/MyShader.h`:

```cpp
/*
    Oscil - My Shader
    Brief description of the visual effect
*/

#pragma once

#include "rendering/WaveformShader.h"
#include <memory>

namespace oscil
{

class MyShader : public WaveformShader
{
public:
    MyShader();
    ~MyShader() override;

    // Identification (shown in UI shader selector)
    [[nodiscard]] juce::String getId() const override { return "my_shader"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "My Shader"; }
    [[nodiscard]] juce::String getDescription() const override
    {
        return "Brief description for tooltip";
    }

#if OSCIL_ENABLE_OPENGL
    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    void render(
        juce::OpenGLContext& context,
        const std::vector<float>& channel1,
        const std::vector<float>* channel2,
        const ShaderRenderParams& params
    ) override;
#endif

    void renderSoftware(
        juce::Graphics& g,
        const std::vector<float>& channel1,
        const std::vector<float>* channel2,
        const ShaderRenderParams& params
    ) override;

private:
#if OSCIL_ENABLE_OPENGL
    struct GLResources;
    std::unique_ptr<GLResources> gl_;
#endif

    // Shader-specific parameters
    static constexpr float MY_PARAM = 1.0f;
};

} // namespace oscil
```

### Step 2: Create Implementation

Create `src/rendering/shaders/MyShader.cpp`:

```cpp
/*
    Oscil - My Shader Implementation
*/

#include "rendering/shaders/MyShader.h"
#include <cmath>

namespace oscil
{

#if OSCIL_ENABLE_OPENGL
using namespace juce::gl;

// GLSL shaders - Modern GLSL 3.30 Core
static const char* vertexShaderSource = R"(
    #version 330 core
    in vec2 position;
    in float distFromCenter;

    uniform mat4 projection;

    out float vDistFromCenter;

    void main()
    {
        gl_Position = projection * vec4(position, 0.0, 1.0);
        vDistFromCenter = distFromCenter;
    }
)";

static const char* fragmentShaderSource = R"(
    #version 330 core
    in float vDistFromCenter;

    uniform vec4 baseColor;
    uniform float opacity;
    
    out vec4 fragColor;

    void main()
    {
        // Your shader effect here
        vec3 color = baseColor.rgb;
        float alpha = opacity * baseColor.a;

        fragColor = vec4(color, alpha);
    }
)";

struct MyShader::GLResources
{
    std::unique_ptr<juce::OpenGLShaderProgram> program;
    GLuint vao = 0;
    GLuint vbo = 0;
    bool compiled = false;

    // Uniform locations
    GLint projectionLoc = -1;
    GLint baseColorLoc = -1;
    GLint opacityLoc = -1;
};
#endif

MyShader::MyShader()
#if OSCIL_ENABLE_OPENGL
    : gl_(std::make_unique<GLResources>())
#endif
{
}

MyShader::~MyShader() = default;

#if OSCIL_ENABLE_OPENGL
bool MyShader::compile(juce::OpenGLContext& context)
{
    if (gl_->compiled)
        return true;

    // Create shader program
    gl_->program = std::make_unique<juce::OpenGLShaderProgram>(context);

    // Direct compilation - no translation needed for GLSL 3.30
    if (!gl_->program->addVertexShader(vertexShaderSource))
    {
        DBG("MyShader: Vertex shader failed: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    if (!gl_->program->addFragmentShader(fragmentShaderSource))
    {
        DBG("MyShader: Fragment shader failed: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    if (!gl_->program->link())
    {
        DBG("MyShader: Link failed: " << gl_->program->getLastError());
        gl_->program.reset();
        return false;
    }

    // Get uniform locations
    gl_->projectionLoc = gl_->program->getUniformIDFromName("projection");
    gl_->baseColorLoc = gl_->program->getUniformIDFromName("baseColor");
    gl_->opacityLoc = gl_->program->getUniformIDFromName("opacity");

    // Validate uniforms
    if (gl_->projectionLoc < 0 || gl_->baseColorLoc < 0 || gl_->opacityLoc < 0)
    {
        DBG("MyShader: Missing uniforms");
        gl_->program.reset();
        return false;
    }

    // Create VAO and VBO (required for OpenGL 3.2 Core Profile)
    context.extensions.glGenVertexArrays(1, &gl_->vao);
    context.extensions.glGenBuffers(1, &gl_->vbo);

    gl_->compiled = true;
    return true;
}

void MyShader::release(juce::OpenGLContext& context)
{
    if (!gl_->compiled)
        return;

    auto& ext = context.extensions;

    if (gl_->vbo != 0)
    {
        ext.glDeleteBuffers(1, &gl_->vbo);
        gl_->vbo = 0;
    }

    if (gl_->vao != 0)
    {
        ext.glDeleteVertexArrays(1, &gl_->vao);
        gl_->vao = 0;
    }

    gl_->program.reset();
    gl_->compiled = false;
}

bool MyShader::isCompiled() const
{
    return gl_->compiled;
}

void MyShader::render(
    juce::OpenGLContext& context,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    if (!gl_->compiled || channel1.size() < 2)
        return;

    auto& ext = context.extensions;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use shader program
    gl_->program->use();

    // Set up orthographic projection
    auto* targetComponent = context.getTargetComponent();
    if (!targetComponent)
        return;

    float viewportWidth = static_cast<float>(targetComponent->getWidth());
    float viewportHeight = static_cast<float>(targetComponent->getHeight());

    float left = 0.0f, right = viewportWidth;
    float top = 0.0f, bottom = viewportHeight;

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(right + left) / (right - left),
        -(top + bottom) / (top - bottom), 0.0f, 1.0f
    };

    ext.glUniformMatrix4fv(gl_->projectionLoc, 1, GL_FALSE, projection);

    // Set color uniforms
    ext.glUniform4f(gl_->baseColorLoc,
        params.colour.getFloatRed(),
        params.colour.getFloatGreen(),
        params.colour.getFloatBlue(),
        params.colour.getFloatAlpha());
    ext.glUniform1f(gl_->opacityLoc, params.opacity);

    // Calculate layout
    float height = params.bounds.getHeight();
    float centerY = params.bounds.getCentreY();
    float amplitude = height * 0.45f * params.verticalScale;

    // Get attribute locations
    GLint programID = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programID);
    GLint positionLoc = ext.glGetAttribLocation(
        static_cast<GLuint>(programID), "position");
    GLint distFromCenterLoc = ext.glGetAttribLocation(
        static_cast<GLuint>(programID), "distFromCenter");

    // Bind VAO and VBO
    ext.glBindVertexArray(gl_->vao);
    ext.glBindBuffer(GL_ARRAY_BUFFER, gl_->vbo);

    // Build geometry using base class helper
    std::vector<float> vertices;
    buildLineGeometry(vertices, channel1, centerY, amplitude,
        params.lineWidth, params.bounds.getX(), params.bounds.getWidth());

    ext.glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(), GL_DYNAMIC_DRAW);

    // Set up vertex attributes
    ext.glEnableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(positionLoc),
        2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    ext.glEnableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glVertexAttribPointer(static_cast<GLuint>(distFromCenterLoc),
        1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0,
        static_cast<GLsizei>(vertices.size() / 4));

    // Handle stereo channel if present
    if (params.isStereo && channel2 != nullptr && channel2->size() >= 2)
    {
        float centerY2 = params.bounds.getY() + height * 0.75f;
        vertices.clear();
        buildLineGeometry(vertices, *channel2, centerY2, amplitude,
            params.lineWidth, params.bounds.getX(), params.bounds.getWidth());

        ext.glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            vertices.data(), GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLE_STRIP, 0,
            static_cast<GLsizei>(vertices.size() / 4));
    }

    // Cleanup
    ext.glDisableVertexAttribArray(static_cast<GLuint>(positionLoc));
    ext.glDisableVertexAttribArray(static_cast<GLuint>(distFromCenterLoc));
    ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
    ext.glBindVertexArray(0);
    glDisable(GL_BLEND);
}
#endif

void MyShader::renderSoftware(
    juce::Graphics& g,
    const std::vector<float>& channel1,
    const std::vector<float>* channel2,
    const ShaderRenderParams& params)
{
    // REQUIRED: Fallback when OpenGL is disabled
    // Call base class implementation or provide custom CPU rendering
    WaveformShader::renderSoftware(g, channel1, channel2, params);
}

} // namespace oscil
```

### Step 3: Register the Shader

Edit `src/rendering/ShaderRegistry.cpp`:

```cpp
#include "rendering/shaders/BasicShader.h"
#include "rendering/shaders/MyShader.h"  // Add include

void ShaderRegistry::registerBuiltInShaders()
{
    registerShader(std::make_unique<BasicShader>());
    registerShader(std::make_unique<MyShader>());  // Add registration
}
```

### Step 4: Add to CMakeLists.txt

Add to `target_sources(Oscil PRIVATE ...)`:

```cmake
src/rendering/shaders/MyShader.cpp
```

Also add to `test_harness/CMakeLists.txt` if testing:

```cmake
${CMAKE_SOURCE_DIR}/src/rendering/shaders/MyShader.cpp
```

### Step 5: Build and Test

```bash
cd build && cmake --build . --target Oscil
```

## ShaderRenderParams Reference

Parameters passed to `render()` and `renderSoftware()`:

| Field | Type | Description |
|-------|------|-------------|
| `colour` | `juce::Colour` | Waveform color from oscillator settings |
| `opacity` | `float` | 0.0-1.0 transparency |
| `lineWidth` | `float` | Line thickness in pixels |
| `bounds` | `juce::Rectangle<float>` | Rendering area (screen coordinates) |
| `isStereo` | `bool` | True if stereo display mode |
| `verticalScale` | `float` | Vertical scale factor (includes auto-scale) |

## Helper Functions in WaveformShader Base Class

### buildLineGeometry()

Creates triangle strip geometry for anti-aliased line rendering:

```cpp
static void buildLineGeometry(
    std::vector<float>& vertices,   // Output: x, y, distFromCenter, reserved
    const std::vector<float>& samples,
    float centerY,
    float amplitude,
    float lineWidth,
    float boundsX,                  // Screen X offset
    float boundsWidth
);
```

Vertex format: 4 floats per vertex (x, y, distFromCenter, reserved)

### compileShaderProgram()

Helper for basic shader compilation (without JUCE translation):

```cpp
static bool compileShaderProgram(
    juce::OpenGLShaderProgram& program,
    const char* vertexSource,
    const char* fragmentSource
);
```

### checkGLError()

Debug helper for OpenGL errors:

```cpp
static bool checkGLError(const char* location);
```

## Blending Modes for Effects

| Effect | Blend Function |
|--------|----------------|
| Normal transparency | `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` |
| Additive (glow) | `glBlendFunc(GL_SRC_ALPHA, GL_ONE)` |
| Multiply | `glBlendFunc(GL_DST_COLOR, GL_ZERO)` |

## Decision Tree: Shader Effect Type

```
What visual effect do you want?
├── Solid color line
│   └── Use base renderSoftware() with custom color
├── Glowing/neon effect
│   └── Use additive blending + multiple passes
│       └── See BasicShader for reference
├── Gradient fill
│   └── Pass UV coordinates as varying
├── Textured effect
│   └── Add texture sampler uniform
└── Animated effect
    └── Pass time uniform, update each frame
```

## Common Mistakes

**GLSL Syntax**
- Use `in` not `attribute` for vertex inputs
- Use `out`/`in` not `varying` for shader communication
- Use custom output variable (e.g. `fragColor`) not `gl_FragColor`
- Use `texture()` not `texture2D` or `textureCube`

**OpenGL 3.2 Core (macOS)**
- Always create and bind VAO before vertex operations
- Use `context.extensions` for modern OpenGL functions
- Delete VAO and VBO in `release()`

**JUCE Integration**
- Do NOT use `translateVertexShaderToV3()` - write GLSL 3.30 directly
- Get component dimensions from `context.getTargetComponent()`
- Check `isCompiled()` before rendering

**Software Fallback**
- ALWAYS implement `renderSoftware()` - OpenGL may be disabled
- Use `juce::Path` and `juce::Graphics::strokePath()` for CPU rendering

**Coordinate System**
- Projection maps (0,0) to top-left, (width,height) to bottom-right
- Use `params.bounds` for waveform area, not viewport dimensions
- `buildLineGeometry()` already adds `boundsX` offset

## Debugging Tips

Add logging macro for release builds:
```cpp
#define MY_LOG(msg) std::cerr << "[MyShader] " << msg << std::endl
```

Check GL errors:
```cpp
GLenum err = glGetError();
if (err != GL_NO_ERROR)
    MY_LOG("GL error: " << err);
```

Log uniform locations after compilation:
```cpp
MY_LOG("projection=" << gl_->projectionLoc
       << ", baseColor=" << gl_->baseColorLoc);
```

---

## Example Shaders

### Reference: BasicShader

Location: `src/rendering/shaders/BasicShader.cpp`

Key features:
- Multi-pass rendering for glow buildup
- Additive blending (`GL_SRC_ALPHA, GL_ONE`)
- `distFromCenter` varying for edge fadeout
- Exponential glow falloff in fragment shader

### Ideas for New Shaders

| Name | Effect | Key Technique |
|------|--------|---------------|
| ClassicShader | Simple solid line | Basic fragment, no effects |
| GradientShader | Vertical color gradient | UV-based color interpolation |
| RainbowShader | Hue varies along X | Pass X coordinate to fragment |
| PulseShader | Brightness animation | Time uniform |
| VUMeterShader | Amplitude-based color | Sample value as color input |
