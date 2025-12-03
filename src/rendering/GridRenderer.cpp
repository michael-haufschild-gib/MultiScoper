/*
    Oscil - Grid Renderer Implementation
*/

#include "rendering/GridRenderer.h"
#include "ui/theme/ThemeManager.h" // For grid colors if needed
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

GridRenderer::GridRenderer()
{
}

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
    context.extensions.glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(gridBufferCapacity_ * 2 * sizeof(float)),
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
}

void GridRenderer::render(juce::OpenGLContext& context, const WaveformRenderData& data)
{
    if (!initialized_ || !colorShader_ || gridVAO_ == 0 || gridVBO_ == 0) return;

    // Grid colors are passed via WaveformRenderData
    const auto& gridColors = data.gridColors;
    const auto& config = data.gridConfig;

    colorShader_->use();

    // Helper to draw a set of lines with a color using persistent buffers
    auto drawLines = [&](const std::vector<float>& verts, juce::Colour col) {
        if (verts.empty()) return;

        // Check buffer capacity (2 floats per vertex)
        size_t vertexCount = verts.size() / 2;
        if (vertexCount > gridBufferCapacity_)
        {
            DBG("GridRenderer: Vertex count (" << vertexCount << ") exceeds buffer capacity ("
                << gridBufferCapacity_ << "), truncating");
            vertexCount = gridBufferCapacity_;
        }

        // Use persistent VAO/VBO - just update the buffer data
        context.extensions.glBindVertexArray(gridVAO_);
        context.extensions.glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);

        // Update buffer data (orphan + upload pattern for efficient streaming)
        context.extensions.glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertexCount * 2 * sizeof(float)),
            verts.data(), GL_STREAM_DRAW);

        // Set color
        context.extensions.glUniform4f(colorUniformLoc_,
            col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), col.getFloatAlpha());

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));

        context.extensions.glBindVertexArray(0);
    };

    // Clear reusable buffers
    minorLines_.clear();
    majorLines_.clear();
    zeroLines_.clear();
    timeMajorLines_.clear();
    timeMinorLines_.clear();

    // Helper to generate grid for a specific vertical range (NDC yTop to yBottom)
    // yTop > yBottom (e.g. 1.0 to 0.0)
    auto generateChannelGrid = [&](float yTop, float yBottom) {
        float height = yTop - yBottom;
        float yCenter = yBottom + height * 0.5f;
        
        // Helper to add line (x1, y1, x2, y2) in NDC
        auto addLineTo = [&](std::vector<float>& dest, float x1, float y1, float x2, float y2) {
            dest.push_back(x1); dest.push_back(y1);
            dest.push_back(x2); dest.push_back(y2);
        };

        // --- Horizontal Lines ---
        // Normalized 0..1 within the channel height
        
        // Minor (8 divisions)
        for (int i=1; i<8; ++i) {
            float ratio = i / 8.0f;
            float y = yTop - ratio * height; // Top-down
            if (std::abs(y - yCenter) > 0.01f) // Skip zero line
                addLineTo(minorLines_, -1.0f, y, 1.0f, y);
        }
        
        // Major (quarters)
        float yQ1 = yTop - 0.25f * height;
        float yQ3 = yTop - 0.75f * height;
        addLineTo(majorLines_, -1.0f, yQ1, 1.0f, yQ1);
        addLineTo(majorLines_, -1.0f, yQ3, 1.0f, yQ3);
        
        // Zero line
        addLineTo(zeroLines_, -1.0f, yCenter, 1.0f, yCenter);
        
        // --- Vertical Lines ---
        
        if (config.timingMode == TimingMode::TIME) {
            float durationMs = config.visibleDurationMs;
            if (durationMs <= 0.0001f || !std::isfinite(durationMs))
                return; // Don't render time grid with invalid duration

            float targetStep = durationMs / 8.0f;
            if (targetStep <= 0.0f || !std::isfinite(targetStep))
                return;

            float magnitude = std::pow(10.0f, std::floor(std::log10(targetStep)));
            if (magnitude <= 0.0f || !std::isfinite(magnitude))
                magnitude = 1.0f;

            float normalizedStep = targetStep / magnitude;
            float stepSize;
            
            if (normalizedStep < 2.0f) stepSize = 1.0f * magnitude;
            else if (normalizedStep < 5.0f) stepSize = 2.0f * magnitude;
            else stepSize = 5.0f * magnitude;
            
            // Time starts at 0 (left edge = -1.0)
            // x = -1.0 + (t / duration) * 2.0
            for (float t = stepSize; t < durationMs; t += stepSize) {
                float xRatio = t / durationMs;
                float x = -1.0f + xRatio * 2.0f;
                addLineTo(timeMajorLines_, x, yTop, x, yBottom);
            }
        } else {
            // Musical Mode - grid divisions aligned with labels
            int numDivisions = 4;  // Number of major grid divisions
            bool isBarBased = false;
            int beatsPerBar = config.timeSigNumerator;

            switch (config.noteInterval) {
                // Single note intervals - show 4 subdivisions within the note
                case NoteInterval::THIRTY_SECOND:
                case NoteInterval::SIXTEENTH:
                case NoteInterval::TWELFTH:
                case NoteInterval::EIGHTH:
                case NoteInterval::TRIPLET_EIGHTH:
                case NoteInterval::DOTTED_EIGHTH:
                case NoteInterval::QUARTER:
                case NoteInterval::TRIPLET_QUARTER:
                case NoteInterval::DOTTED_QUARTER:
                case NoteInterval::HALF:
                case NoteInterval::TRIPLET_HALF:
                case NoteInterval::DOTTED_HALF:
                    numDivisions = 4;
                    break;
                // Bar-based intervals - show beats or bars
                case NoteInterval::WHOLE:
                    numDivisions = beatsPerBar; // Show beat divisions within the bar
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

            float widthPerDiv = 2.0f / static_cast<float>(numDivisions);

            // Draw major division lines (between divisions, not at edges)
            for (int i = 1; i < numDivisions; ++i) {
                float x = -1.0f + i * widthPerDiv;

                if (isBarBased) {
                    // Bar/beat boundaries are major lines
                    addLineTo(majorLines_, x, yTop, x, yBottom);
                } else {
                    // Subdivision lines are minor
                    addLineTo(timeMajorLines_, x, yTop, x, yBottom);
                }
            }

            // For bar-based modes, add sub-beat lines within each bar
            if (isBarBased && config.noteInterval >= NoteInterval::TWO_BARS) {
                int subBeatsPerDiv = beatsPerBar;
                float subBeatWidth = widthPerDiv / static_cast<float>(subBeatsPerDiv);

                for (int i = 0; i < numDivisions; ++i) {
                    float baseX = -1.0f + i * widthPerDiv;
                    for (int j = 1; j < subBeatsPerDiv; ++j) {
                        float x = baseX + j * subBeatWidth;
                        addLineTo(timeMinorLines_, x, yTop, x, yBottom);
                    }
                }
            }
        }
    };

    if (data.isStereo) {
        // Top Half (Left): +1.0 to 0.0
        generateChannelGrid(1.0f, 0.0f);
        
        // Bottom Half (Right): 0.0 to -1.0
        generateChannelGrid(0.0f, -1.0f);
        
        // Separator
        majorLines_.push_back(-1.0f); majorLines_.push_back(0.0f);
        majorLines_.push_back(1.0f); majorLines_.push_back(0.0f);
        
    } else {
        // Full: +1.0 to -1.0
        generateChannelGrid(1.0f, -1.0f);
    }
    
    // Render passes
    // Order: Minor, TimeMinor, TimeMajor, Major, Zero

    drawLines(minorLines_, gridColors.gridMinor);
    drawLines(timeMinorLines_, gridColors.gridMinor.withAlpha(0.3f));

    // Time Major (Beats/Time Steps) - usually a bit fainter than structural major lines
    drawLines(timeMajorLines_, gridColors.gridMajor.withAlpha(0.6f));

    drawLines(majorLines_, gridColors.gridMajor);
    drawLines(zeroLines_, gridColors.gridZeroLine);
}

} // namespace oscil
