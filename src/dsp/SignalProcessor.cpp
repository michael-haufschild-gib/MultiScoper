/*
    Oscil - Signal Processor Implementation
*/

#include "dsp/SignalProcessor.h"
#include <cmath>
#include <algorithm>

namespace oscil
{

void SignalProcessor::process(const float* leftChannel,
                               const float* rightChannel,
                               int numSamples,
                               ProcessingMode mode,
                               ProcessedSignal& output) const
{
    switch (mode)
    {
        case ProcessingMode::FullStereo:
            processFullStereo(leftChannel, rightChannel, numSamples, output);
            break;

        case ProcessingMode::Mono:
            processMono(leftChannel, rightChannel, numSamples, output);
            break;

        case ProcessingMode::Mid:
            processMid(leftChannel, rightChannel, numSamples, output);
            break;

        case ProcessingMode::Side:
            processSide(leftChannel, rightChannel, numSamples, output);
            break;

        case ProcessingMode::Left:
            processLeft(leftChannel, numSamples, output);
            break;

        case ProcessingMode::Right:
            processRight(rightChannel, numSamples, output);
            break;
    }
}

void SignalProcessor::process(const juce::AudioBuffer<float>& input,
                               ProcessingMode mode,
                               ProcessedSignal& output) const
{
    // Handle empty buffer case
    if (input.getNumChannels() == 0 || input.getNumSamples() == 0)
    {
        output.clear();
        return;
    }

    const float* left = input.getReadPointer(0);
    const float* right = input.getNumChannels() > 1 ? input.getReadPointer(1) : left;

    process(left, right, input.getNumSamples(), mode, output);
}

void SignalProcessor::processFullStereo(const float* left, const float* right,
                                         int numSamples, ProcessedSignal& output) const
{
    output.resize(numSamples, true);

    if (left != nullptr)
    {
        std::copy(left, left + numSamples, output.channel1.begin());
    }
    else
    {
        output.clear();
    }

    if (right != nullptr)
    {
        std::copy(right, right + numSamples, output.channel2.begin());
    }
    else if (left != nullptr)
    {
        // Mono source: duplicate left to right
        std::copy(left, left + numSamples, output.channel2.begin());
    }
}

void SignalProcessor::processMono(const float* left, const float* right,
                                   int numSamples, ProcessedSignal& output) const
{
    output.resize(numSamples, false);

    for (int i = 0; i < numSamples; ++i)
    {
        float l = (left != nullptr) ? left[i] : 0.0f;
        float r = (right != nullptr) ? right[i] : l;
        output.channel1[i] = (l + r) * 0.5f;
    }
}

void SignalProcessor::processMid(const float* left, const float* right,
                                  int numSamples, ProcessedSignal& output) const
{
    // Mid = (L+R)/2 - identical to mono
    processMono(left, right, numSamples, output);
}

void SignalProcessor::processSide(const float* left, const float* right,
                                   int numSamples, ProcessedSignal& output) const
{
    output.resize(numSamples, false);

    for (int i = 0; i < numSamples; ++i)
    {
        float l = (left != nullptr) ? left[i] : 0.0f;
        float r = (right != nullptr) ? right[i] : l;
        output.channel1[i] = (l - r) * 0.5f;
    }
}

void SignalProcessor::processLeft(const float* left, int numSamples, ProcessedSignal& output) const
{
    output.resize(numSamples, false);

    if (left != nullptr)
    {
        std::copy(left, left + numSamples, output.channel1.begin());
    }
    else
    {
        output.clear();
    }
}

void SignalProcessor::processRight(const float* right, int numSamples, ProcessedSignal& output) const
{
    output.resize(numSamples, false);

    if (right != nullptr)
    {
        std::copy(right, right + numSamples, output.channel1.begin());
    }
    else
    {
        output.clear();
    }
}

float SignalProcessor::calculateCorrelation(const float* leftChannel,
                                             const float* rightChannel,
                                             int numSamples)
{
    // Need at least 2 samples for meaningful correlation calculation
    // With 1 sample, variance is undefined (division by zero risk)
    if (leftChannel == nullptr || rightChannel == nullptr || numSamples < 2)
        return 0.0f;

    double sumL = 0.0, sumR = 0.0, sumLR = 0.0;
    double sumL2 = 0.0, sumR2 = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        double l = leftChannel[i];
        double r = rightChannel[i];

        sumL += l;
        sumR += r;
        sumLR += l * r;
        sumL2 += l * l;
        sumR2 += r * r;
    }

    double n = static_cast<double>(numSamples);
    double numerator = n * sumLR - sumL * sumR;

    // Calculate the argument for sqrt, checking for negative values due to floating point errors
    double sqrtArg = (n * sumL2 - sumL * sumL) * (n * sumR2 - sumR * sumR);
    if (sqrtArg <= 0.0)
        return 0.0f;

    double denominator = std::sqrt(sqrtArg);

    // Check for very small denominator or NaN
    if (!std::isfinite(denominator) || denominator < 1e-10)
        return 0.0f;

    return static_cast<float>(numerator / denominator);
}

float SignalProcessor::calculatePeak(const float* samples, int numSamples)
{
    if (samples == nullptr || numSamples <= 0)
        return 0.0f;

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float absVal = std::abs(samples[i]);
        if (absVal > peak)
            peak = absVal;
    }
    return peak;
}

float SignalProcessor::calculateRMS(const float* samples, int numSamples)
{
    if (samples == nullptr || numSamples <= 0)
        return 0.0f;

    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        sumSquares += samples[i] * samples[i];
    }
    return static_cast<float>(std::sqrt(sumSquares / numSamples));
}

void SignalProcessor::decimate(const float* input, int inputLength,
                                float* output, int outputLength, bool preservePeaks)
{
    if (input == nullptr || output == nullptr || inputLength <= 0 || outputLength <= 0)
        return;

    if (inputLength <= outputLength)
    {
        // No decimation needed, just copy
        std::copy(input, input + std::min(inputLength, outputLength), output);
        return;
    }

    float samplesPerOutput = static_cast<float>(inputLength) / static_cast<float>(outputLength);

    for (int i = 0; i < outputLength; ++i)
    {
        int startIdx = static_cast<int>(i * samplesPerOutput);
        int endIdx = static_cast<int>((i + 1) * samplesPerOutput);
        endIdx = std::min(endIdx, inputLength);

        if (preservePeaks)
        {
            // Find max absolute value in range
            float maxVal = 0.0f;
            float maxSample = 0.0f;
            for (int j = startIdx; j < endIdx; ++j)
            {
                float absVal = std::abs(input[j]);
                if (absVal > maxVal)
                {
                    maxVal = absVal;
                    maxSample = input[j];
                }
            }
            output[i] = maxSample;
        }
        else
        {
            // Average samples in range
            float sum = 0.0f;
            int count = endIdx - startIdx;
            for (int j = startIdx; j < endIdx; ++j)
            {
                sum += input[j];
            }
            output[i] = count > 0 ? sum / count : 0.0f;
        }
    }
}

// AdaptiveDecimator implementation

void AdaptiveDecimator::setDisplayWidth(int widthPixels)
{
    displayWidth_ = std::max(1, widthPixels);
    targetSamples_ = displayWidth_ * 2; // 2 samples per pixel for good visual quality
}

void AdaptiveDecimator::process(const float* input, int inputLength,
                                 std::vector<float>& output) const
{
    output.resize(targetSamples_);

    if (inputLength <= targetSamples_)
    {
        output.resize(inputLength);
        std::copy(input, input + inputLength, output.begin());
        return;
    }

    SignalProcessor::decimate(input, inputLength, output.data(), targetSamples_, true);
}

void AdaptiveDecimator::processWithEnvelope(const float* input, int inputLength,
                                             std::vector<float>& minEnvelope,
                                             std::vector<float>& maxEnvelope) const
{
    minEnvelope.resize(displayWidth_);
    maxEnvelope.resize(displayWidth_);

    if (inputLength <= 0)
    {
        std::fill(minEnvelope.begin(), minEnvelope.end(), 0.0f);
        std::fill(maxEnvelope.begin(), maxEnvelope.end(), 0.0f);
        return;
    }

    float samplesPerPixel = static_cast<float>(inputLength) / static_cast<float>(displayWidth_);

    for (int i = 0; i < displayWidth_; ++i)
    {
        int startIdx = static_cast<int>(i * samplesPerPixel);
        int endIdx = static_cast<int>((i + 1) * samplesPerPixel);
        endIdx = std::min(endIdx, inputLength);

        float minVal = input[startIdx];
        float maxVal = input[startIdx];

        for (int j = startIdx + 1; j < endIdx; ++j)
        {
            minVal = std::min(minVal, input[j]);
            maxVal = std::max(maxVal, input[j]);
        }

        minEnvelope[i] = minVal;
        maxEnvelope[i] = maxVal;
    }
}

} // namespace oscil
