/*
    Oscil - Grid Renderer Implementation
*/

#include "rendering/GridRenderer.h"

#include <cmath>

namespace oscil
{

using namespace juce::gl;

// Simple color shader for grid rendering
static const char* colorVertexShader = R"(
    #version 330 core
    in vec2 position;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
    }
)";

static const char* colorFragmentShader = R"(
    #version 330 core
    uniform vec4 color;
    out vec4 fragColor;

    // Convert sRGB to linear color space for correct rendering pipeline
    vec3 sRGBToLinear(vec3 srgb) {
        return pow(srgb, vec3(2.2));
    }

    void main() {
        vec3 linearColor = sRGBToLinear(color.rgb);
        fragColor = vec4(linearColor, color.a);
    }
)";

GridRenderer::GridRenderer() {}

GridRenderer::~GridRenderer()
{
    // release() should be called explicitly with context
}

void GridRenderer::initialize(juce::OpenGLContext& context)
{
    if (initialized_)
        release(context);

    compileShaders(context);
    createBuffers(context);

    // Pre-reserve vector capacity
    // Estimating typical usage to avoid reallocations
    minorLines_.reserve(gridBufferCapacity_);
    majorLines_.reserve(gridBufferCapacity_);
    zeroLines_.reserve(gridBufferCapacity_);
    timeMajorLines_.reserve(gridBufferCapacity_);
    timeMinorLines_.reserve(gridBufferCapacity_);

    initialized_ = true;
}

void GridRenderer::release(juce::OpenGLContext& context)
{
    colorShader_.reset();

    if (gridVAO_ != 0)
    {
        context.extensions.glDeleteVertexArrays(1, &gridVAO_);
        gridVAO_ = 0;
    }

    if (gridVBO_ != 0)
    {
        context.extensions.glDeleteBuffers(1, &gridVBO_);
        gridVBO_ = 0;
    }

    initialized_ = false;
}

void GridRenderer::createBuffers(juce::OpenGLContext& context)
{
    // Create persistent grid buffers (avoid per-frame allocation)
    context.extensions.glGenBuffers(1, &gridVBO_);
    context.extensions.glGenVertexArrays(1, &gridVAO_);

    // Set up VAO once - attribute layout won't change
    context.extensions.glBindVertexArray(gridVAO_);
    context.extensions.glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
    // Pre-allocate buffer with GL_STREAM_DRAW for frequent updates
    context.extensions.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(gridBufferCapacity_ * 2 * sizeof(float)),
                                    nullptr, GL_STREAM_DRAW);
    context.extensions.glEnableVertexAttribArray(0);
    context.extensions.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    context.extensions.glBindVertexArray(0);
}

void GridRenderer::compileShaders(juce::OpenGLContext& context)
{
    // Compile color shader
    colorShader_ = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (!colorShader_->addVertexShader(colorVertexShader))
    {
        DBG("GridRenderer: Failed to compile vertex shader: " << colorShader_->getLastError());
        colorShader_.reset();
        return;
    }

    if (!colorShader_->addFragmentShader(colorFragmentShader))
    {
        DBG("GridRenderer: Failed to compile fragment shader: " << colorShader_->getLastError());
        colorShader_.reset();
        return;
    }

    GLuint colorProgramID = colorShader_->getProgramID();
    context.extensions.glBindAttribLocation(colorProgramID, 0, "position");

    if (!colorShader_->link())
    {
        DBG("GridRenderer: Failed to link shader program: " << colorShader_->getLastError());
        colorShader_.reset();
        return;
    }

    colorUniformLoc_ = colorShader_->getUniformIDFromName("color");
    if (colorUniformLoc_ < 0)
    {
        DBG("GridRenderer: Missing 'color' uniform");
        colorShader_.reset();
    }
}

void GridRenderer::drawLines(juce::OpenGLContext& context, const std::vector<float>& verts, juce::Colour col)
{
    if (verts.empty())
        return;

    size_t vertexCount = verts.size() / 2;
    if (vertexCount > gridBufferCapacity_)
        vertexCount = gridBufferCapacity_;

    context.extensions.glBindVertexArray(gridVAO_);
    context.extensions.glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
    context.extensions.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexCount * 2 * sizeof(float)),
                                    verts.data(), GL_STREAM_DRAW);

    context.extensions.glUniform4f(colorUniformLoc_, col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(),
                                   col.getFloatAlpha());

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));
    context.extensions.glBindVertexArray(0);
}

void GridRenderer::generateHorizontalGrid(float yTop, float yBottom)
{
    float height = yTop - yBottom;
    float yCenter = yBottom + height * 0.5f;

    // Minor (8 divisions)
    for (int i = 1; i < 8; ++i)
    {
        float y = yTop - (i / 8.0f) * height;
        if (std::abs(y - yCenter) > 0.01f)
            addLine(minorLines_, -1.0f, y, 1.0f, y);
    }

    // Major (quarters)
    addLine(majorLines_, -1.0f, yTop - 0.25f * height, 1.0f, yTop - 0.25f * height);
    addLine(majorLines_, -1.0f, yTop - 0.75f * height, 1.0f, yTop - 0.75f * height);

    // Zero line
    addLine(zeroLines_, -1.0f, yCenter, 1.0f, yCenter);
}

void GridRenderer::generateTimeGrid(const GridConfiguration& config, float yTop, float yBottom)
{
    float durationMs = config.visibleDurationMs;
    if (durationMs <= 0.0001f || !std::isfinite(durationMs))
        return;

    float targetStep = durationMs / 8.0f;
    if (targetStep <= 0.0f || !std::isfinite(targetStep))
        return;

    float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
    if (magnitude <= 0.0f || !std::isfinite(magnitude))
        magnitude = 1.0f;

    float normalizedStep = targetStep / magnitude;
    float stepSize;
    if (normalizedStep < 2.0f)
        stepSize = magnitude;
    else if (normalizedStep < 5.0f)
        stepSize = 2.0f * magnitude;
    else
        stepSize = 5.0f * magnitude;

    for (float t = stepSize; t < durationMs; t += stepSize)
    {
        float x = -1.0f + (t / durationMs) * 2.0f;
        addLine(timeMajorLines_, x, yTop, x, yBottom);
    }
}

void GridRenderer::generateMusicalGrid(const GridConfiguration& config, float yTop, float yBottom)
{
    int numDivisions = 4;
    bool isBarBased = false;
    int beatsPerBar = config.timeSigNumerator;

    switch (config.noteInterval)
    {
        case NoteInterval::WHOLE:
            numDivisions = beatsPerBar;
            isBarBased = true;
            break;
        case NoteInterval::TWO_BARS:
            numDivisions = 2;
            isBarBased = true;
            break;
        case NoteInterval::THREE_BARS:
            numDivisions = 3;
            isBarBased = true;
            break;
        case NoteInterval::FOUR_BARS:
            numDivisions = 4;
            isBarBased = true;
            break;
        case NoteInterval::EIGHT_BARS:
            numDivisions = 8;
            isBarBased = true;
            break;
        default:
            numDivisions = 4;
            break;
    }

    if (numDivisions <= 0)
        numDivisions = 1;
    float widthPerDiv = 2.0f / static_cast<float>(numDivisions);

    for (int i = 1; i < numDivisions; ++i)
    {
        float x = -1.0f + i * widthPerDiv;
        if (isBarBased)
            addLine(majorLines_, x, yTop, x, yBottom);
        else
            addLine(timeMajorLines_, x, yTop, x, yBottom);
    }

    if (isBarBased && config.noteInterval >= NoteInterval::TWO_BARS)
    {
        int subBeatsPerDiv = std::max(1, beatsPerBar);
        float subBeatWidth = widthPerDiv / static_cast<float>(subBeatsPerDiv);
        for (int i = 0; i < numDivisions; ++i)
        {
            float baseX = -1.0f + i * widthPerDiv;
            for (int j = 1; j < subBeatsPerDiv; ++j)
                addLine(timeMinorLines_, baseX + j * subBeatWidth, yTop, baseX + j * subBeatWidth, yBottom);
        }
    }
}

void GridRenderer::generateChannelGrid(const GridConfiguration& config, float yTop, float yBottom)
{
    generateHorizontalGrid(yTop, yBottom);

    if (config.timingMode == TimingMode::TIME)
        generateTimeGrid(config, yTop, yBottom);
    else
        generateMusicalGrid(config, yTop, yBottom);
}

void GridRenderer::render(juce::OpenGLContext& context, const WaveformRenderData& data)
{
    if (!initialized_ || !colorShader_ || gridVAO_ == 0 || gridVBO_ == 0)
        return;

    const auto& gridColors = data.gridColors;
    const auto& config = data.gridConfig;

    glDisable(GL_DEPTH_TEST);
    colorShader_->use();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    minorLines_.clear();
    majorLines_.clear();
    zeroLines_.clear();
    timeMajorLines_.clear();
    timeMinorLines_.clear();

    if (data.isStereo)
    {
        generateChannelGrid(config, 1.0f, 0.0f);
        generateChannelGrid(config, 0.0f, -1.0f);
        addLine(majorLines_, -1.0f, 0.0f, 1.0f, 0.0f);
    }
    else
    {
        generateChannelGrid(config, 1.0f, -1.0f);
    }

    drawLines(context, minorLines_, gridColors.gridMinor);
    drawLines(context, timeMinorLines_, gridColors.gridMinor.withAlpha(0.3f));
    drawLines(context, timeMajorLines_, gridColors.gridMajor.withAlpha(0.6f));
    drawLines(context, majorLines_, gridColors.gridMajor);
    drawLines(context, zeroLines_, gridColors.gridZeroLine);
}

} // namespace oscil
