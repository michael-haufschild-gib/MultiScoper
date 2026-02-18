/*
    Oscil - Signal Processor Implementation
*/

#include "core/dsp/SignalProcessor.h"
#include <cmath>
#include <algorithm>

namespace oscil
{

void SignalProcessor::process(std::span<const float> leftChannel,
                               std::span<const float> rightChannel,
                               ProcessingMode mode,
                               ProcessedSignal& output) const
{
    // Validate input
    if (leftChannel.empty())
    {
        output.clear();
        return;
    }

    // If right channel is provided, ensure size matches left
    // If mismatch, use minimum size (or treat as mono if right is empty)
    
    switch (mode)
    {
        case ProcessingMode::FullStereo:
            processFullStereo(leftChannel, rightChannel, output);
            break;

        case ProcessingMode::Mono:
            processMono(leftChannel, rightChannel, output);
            break;

        case ProcessingMode::Mid:
            processMid(leftChannel, rightChannel, output);
            break;

        case ProcessingMode::Side:
            processSide(leftChannel, rightChannel, output);
            break;

        case ProcessingMode::Left:
            processLeft(leftChannel, output);
            break;

        case ProcessingMode::Right:
            processRight(rightChannel, output);
            break;

        default:
            // Unknown processing mode - fallback to mono for safety
            jassertfalse; // Catch in debug builds
            processMono(leftChannel, rightChannel, output);
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

    // Construct spans from buffer
    std::span<const float> left(input.getReadPointer(0), static_cast<size_t>(input.getNumSamples()));
    std::span<const float> right;
    
    if (input.getNumChannels() > 1)
    {
        right = std::span<const float>(input.getReadPointer(1), static_cast<size_t>(input.getNumSamples()));
    }

    process(left, right, mode, output);
}

void SignalProcessor::processFullStereo(std::span<const float> left, std::span<const float> right,
                                         ProcessedSignal& output) const
{
    int numSamples = static_cast<int>(left.size());
    output.resize(numSamples, true);

    std::copy(left.begin(), left.end(), output.channel1.begin());

    if (!right.empty())
    {
        // Ensure right has enough samples, copy available
        size_t copyCount = std::min(right.size(), static_cast<size_t>(numSamples));
        std::copy(right.begin(), right.begin() + static_cast<std::ptrdiff_t>(copyCount), output.channel2.begin());
        
        // Fill remaining with silence if right is shorter (unlikely in normal usage)
        if (copyCount < static_cast<size_t>(numSamples))
        {
            std::fill(output.channel2.begin() + static_cast<std::ptrdiff_t>(copyCount), output.channel2.end(), 0.0f);
        }
    }
    else
    {
        // Mono source: duplicate left to right
        std::copy(left.begin(), left.end(), output.channel2.begin());
    }
}

void SignalProcessor::processMono(std::span<const float> left, std::span<const float> right,
                                   ProcessedSignal& output) const
{
    int numSamples = static_cast<int>(left.size());
    output.resize(numSamples, false);

    bool hasRight = !right.empty();
    size_t rightSize = right.size();

    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[static_cast<size_t>(i)];
        float r = (hasRight && static_cast<size_t>(i) < rightSize) ? right[static_cast<size_t>(i)] : l;
        output.channel1[static_cast<size_t>(i)] = (l + r) * 0.5f;
    }
}

void SignalProcessor::processMid(std::span<const float> left, std::span<const float> right,
                                  ProcessedSignal& output) const
{
    // Mid = (L+R)/2 - identical to mono
    processMono(left, right, output);
}

void SignalProcessor::processSide(std::span<const float> left, std::span<const float> right,
                                   ProcessedSignal& output) const
{
    int numSamples = static_cast<int>(left.size());
    output.resize(numSamples, false);

    bool hasRight = !right.empty();
    size_t rightSize = right.size();

    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[static_cast<size_t>(i)];
        float r = (hasRight && static_cast<size_t>(i) < rightSize) ? right[static_cast<size_t>(i)] : l;
        output.channel1[static_cast<size_t>(i)] = (l - r) * 0.5f;
    }
}

void SignalProcessor::processLeft(std::span<const float> left, ProcessedSignal& output) const
{
    int numSamples = static_cast<int>(left.size());
    output.resize(numSamples, false);
    std::copy(left.begin(), left.end(), output.channel1.begin());
}

void SignalProcessor::processRight(std::span<const float> right, ProcessedSignal& output) const
{
    if (right.empty())
    {
        output.clear();
        return;
    }

    int numSamples = static_cast<int>(right.size());
    output.resize(numSamples, false);
    std::copy(right.begin(), right.end(), output.channel1.begin());
}

float SignalProcessor::calculateCorrelation(std::span<const float> leftChannel,
                                             std::span<const float> rightChannel)
{
    if (leftChannel.empty() || rightChannel.empty())
        return 0.0f;

    size_t numSamples = std::min(leftChannel.size(), rightChannel.size());
    if (numSamples < 2)
        return 0.0f;

    double sumL = 0.0, sumR = 0.0, sumLR = 0.0;
    double sumL2 = 0.0, sumR2 = 0.0;

    for (size_t i = 0; i < numSamples; ++i)
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

float SignalProcessor::calculatePeak(std::span<const float> samples)
{
    if (samples.empty())
        return 0.0f;

    float peak = 0.0f;
    for (float sample : samples)
    {
        float absVal = std::abs(sample);
        if (absVal > peak)
            peak = absVal;
    }
    return peak;
}

float SignalProcessor::calculateRMS(std::span<const float> samples)
{
    if (samples.empty())
        return 0.0f;

    double sumSquares = 0.0;
    for (float sample : samples)
    {
        sumSquares += sample * sample;
    }
    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(samples.size())));
}

void SignalProcessor::decimate(std::span<const float> input,
                                std::span<float> output,
                                bool preservePeaks)
{
    if (input.empty() || output.empty())
        return;

    size_t inputLength = input.size();
    size_t outputLength = output.size();

    if (inputLength <= outputLength)
    {
        if (inputLength == 0) 
        {
            std::fill(output.begin(), output.end(), 0.0f);
            return;
        }

        // Upsampling with linear interpolation
        if (inputLength == 1)
        {
            std::fill(output.begin(), output.end(), input[0]);
            return;
        }

        // Guard against outputLength == 1 (would cause division by zero)
        if (outputLength == 1)
        {
            output[0] = input[inputLength / 2];  // Use middle sample
            return;
        }

        double scale = static_cast<double>(inputLength - 1) / static_cast<double>(outputLength - 1);
        
        for (size_t i = 0; i < outputLength; ++i)
        {
            double pos = static_cast<double>(i) * scale;
            size_t idx0 = static_cast<size_t>(pos);
            size_t idx1 = std::min(idx0 + 1, inputLength - 1);
            float frac = static_cast<float>(pos - static_cast<double>(idx0));
            
            output[i] = input[idx0] * (1.0f - frac) + input[idx1] * frac;
        }
        return;
    }

    float samplesPerOutput = static_cast<float>(inputLength) / static_cast<float>(outputLength);

    for (size_t i = 0; i < outputLength; ++i)
    {
        size_t startIdx = static_cast<size_t>(static_cast<float>(i) * samplesPerOutput);
        size_t endIdx = static_cast<size_t>(static_cast<float>(i + 1) * samplesPerOutput);
        endIdx = std::min(endIdx, inputLength);

        if (preservePeaks)
        {
            // Find max absolute value in range
            float maxVal = 0.0f;
            float maxSample = 0.0f;
            for (size_t j = startIdx; j < endIdx; ++j)
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
            double sum = 0.0;
            size_t count = endIdx - startIdx;
            for (size_t j = startIdx; j < endIdx; ++j)
            {
                sum += static_cast<double>(input[j]);
            }
            output[i] = count > 0 ? static_cast<float>(sum / static_cast<double>(count)) : 0.0f;
        }
    }
}

//==============================================================================
// AdaptiveDecimator Implementation
//==============================================================================

LODTier AdaptiveDecimator::calculateTierWithHysteresis(int width, LODTier currentTier) const
{
    // Apply hysteresis to prevent tier flickering at boundaries.
    // When width is near a threshold, prefer staying in the current tier
    // unless we've moved clearly past the hysteresis zone.
    
    // Helper to check if width is within hysteresis zone of a threshold
    auto isNearThreshold = [this](int w, int threshold) {
        return w >= (threshold - HYSTERESIS_PIXELS) && w <= (threshold + HYSTERESIS_PIXELS);
    };
    
    // Determine the strict tier based on thresholds (with >= to include boundary in higher tier)
    auto getStrictTier = [](int w) {
        if (w >= 800) return LODTier::Full;
        if (w >= 400) return LODTier::High;
        if (w >= 200) return LODTier::Medium;
        return LODTier::Preview;
    };
    
    LODTier strictTier = getStrictTier(width);
    
    // If already at the strict tier, no change needed
    if (strictTier == currentTier)
        return currentTier;
    
    // Check if we're in a hysteresis zone around any threshold
    bool inHysteresisZone = isNearThreshold(width, THRESHOLD_FULL_HIGH) ||
                            isNearThreshold(width, THRESHOLD_HIGH_MEDIUM) ||
                            isNearThreshold(width, THRESHOLD_MEDIUM_PREVIEW);
    
    if (inHysteresisZone)
    {
        // In hysteresis zone: only change tier if moving by more than one tier level
        int currentSamples = getLODSampleCount(currentTier);
        int strictSamples = getLODSampleCount(strictTier);
        
        // If the difference is one tier (512 samples or less), prefer current tier
        if (std::abs(currentSamples - strictSamples) <= 1024)
            return currentTier;
    }
    
    return strictTier;
}

void AdaptiveDecimator::setDisplayWidth(int widthPixels)
{
    int width = std::max(1, widthPixels);
    displayWidth_.store(width, std::memory_order_relaxed);

    // Calculate new tier with hysteresis to prevent flickering
    LODTier oldTier = currentTier_.load(std::memory_order_relaxed);
    LODTier newTier = calculateTierWithHysteresis(width, oldTier);

    // Update tier and target sample count
    currentTier_.store(newTier, std::memory_order_relaxed);
    targetSamples_.store(getLODSampleCount(newTier), std::memory_order_relaxed);
}

void AdaptiveDecimator::process(std::span<const float> input,
                                 std::vector<float>& output) const
{
    int targetSamples = targetSamples_.load(std::memory_order_relaxed);
    output.resize(static_cast<size_t>(targetSamples));

    if (output.empty())
        return;

    // Wrap output vector in span
    std::span<float> outSpan(output.data(), output.size());
    SignalProcessor::decimate(input, outSpan, true);
}

void AdaptiveDecimator::processWithEnvelope(std::span<const float> input,
                                             std::vector<float>& minEnvelope,
                                             std::vector<float>& maxEnvelope) const
{
    // Use target samples for envelope size (consistent with LOD tier)
    int envelopeSize = targetSamples_.load(std::memory_order_relaxed);
    
    minEnvelope.resize(static_cast<size_t>(envelopeSize));
    maxEnvelope.resize(static_cast<size_t>(envelopeSize));

    if (input.empty())
    {
        std::fill(minEnvelope.begin(), minEnvelope.end(), 0.0f);
        std::fill(maxEnvelope.begin(), maxEnvelope.end(), 0.0f);
        return;
    }

    size_t inputLength = input.size();
    float samplesPerOutput = static_cast<float>(inputLength) / static_cast<float>(envelopeSize);

    for (int i = 0; i < envelopeSize; ++i)
    {
        size_t startIdx = static_cast<size_t>(static_cast<float>(i) * samplesPerOutput);
        size_t endIdx = static_cast<size_t>(static_cast<float>(i + 1) * samplesPerOutput);
        endIdx = std::min(endIdx, inputLength);

        if (startIdx >= endIdx)
        {
             minEnvelope[static_cast<size_t>(i)] = 0.0f;
             maxEnvelope[static_cast<size_t>(i)] = 0.0f;
             continue;
        }

        float minVal = input[startIdx];
        float maxVal = input[startIdx];

        for (size_t j = startIdx + 1; j < endIdx; ++j)
        {
            minVal = std::min(minVal, input[j]);
            maxVal = std::max(maxVal, input[j]);
        }

        minEnvelope[static_cast<size_t>(i)] = minVal;
        maxEnvelope[static_cast<size_t>(i)] = maxVal;
    }
}

void AdaptiveDecimator::processWithPeaks(std::span<const float> input,
                                          DecimatedWaveform& output) const
{
    output.tier = currentTier_.load(std::memory_order_relaxed);
    output.resize(static_cast<size_t>(targetSamples_.load(std::memory_order_relaxed)));

    if (input.empty())
    {
        std::fill(output.samples.begin(), output.samples.end(), 0.0f);
        std::fill(output.minEnvelope.begin(), output.minEnvelope.end(), 0.0f);
        std::fill(output.maxEnvelope.begin(), output.maxEnvelope.end(), 0.0f);
        return;
    }

    size_t inputLength = input.size();
    size_t outputLength = output.size();
    
    // Handle case where input is smaller than output (upsampling)
    if (inputLength <= outputLength)
    {
        if (inputLength == 1)
        {
            std::fill(output.samples.begin(), output.samples.end(), input[0]);
            std::fill(output.minEnvelope.begin(), output.minEnvelope.end(), input[0]);
            std::fill(output.maxEnvelope.begin(), output.maxEnvelope.end(), input[0]);
            return;
        }

        // Linear interpolation for upsampling
        double scale = static_cast<double>(inputLength - 1) / static_cast<double>(outputLength - 1);
        
        for (size_t i = 0; i < outputLength; ++i)
        {
            double pos = static_cast<double>(i) * scale;
            size_t idx0 = static_cast<size_t>(pos);
            size_t idx1 = std::min(idx0 + 1, inputLength - 1);
            float frac = static_cast<float>(pos - static_cast<double>(idx0));
            
            float interpolated = input[idx0] * (1.0f - frac) + input[idx1] * frac;
            output.samples[i] = interpolated;
            output.minEnvelope[i] = interpolated;
            output.maxEnvelope[i] = interpolated;
        }
        return;
    }

    // Downsampling with min/max envelope preservation
    float samplesPerOutput = static_cast<float>(inputLength) / static_cast<float>(outputLength);

    for (size_t i = 0; i < outputLength; ++i)
    {
        size_t startIdx = static_cast<size_t>(static_cast<float>(i) * samplesPerOutput);
        size_t endIdx = static_cast<size_t>(static_cast<float>(i + 1) * samplesPerOutput);
        endIdx = std::min(endIdx, inputLength);

        if (startIdx >= endIdx)
        {
            output.samples[i] = 0.0f;
            output.minEnvelope[i] = 0.0f;
            output.maxEnvelope[i] = 0.0f;
            continue;
        }

        float minVal = input[startIdx];
        float maxVal = input[startIdx];
        float maxAbsVal = 0.0f;
        float peakSample = 0.0f;

        for (size_t j = startIdx; j < endIdx; ++j)
        {
            float sample = input[j];
            minVal = std::min(minVal, sample);
            maxVal = std::max(maxVal, sample);
            
            float absVal = std::abs(sample);
            if (absVal > maxAbsVal)
            {
                maxAbsVal = absVal;
                peakSample = sample;
            }
        }

        // Store the peak sample (preserves visual transients)
        output.samples[i] = peakSample;
        // Store min/max for accurate envelope rendering
        output.minEnvelope[i] = minVal;
        output.maxEnvelope[i] = maxVal;
    }
}

} // namespace oscil
