# Feature Requirements: Statistical Analysis Engine

**Status**: Draft
**Priority**: Medium
**Related To**: Statistics Visualization

## 1. Overview
The Statistical Analysis Engine calculates real-time audio metrics from the input signal. This component provides the raw data required for the visual overlays. It must operate efficiently on the audio thread or a decoupled worker thread.

## 2. Mathematical Requirements

### 2.1 Basic Metrics (Per Block)
For a block of samples $x$ of size $N$:

- **RMS (Root Mean Square)**:
  $$ RMS = \sqrt{\frac{1}{N} \sum_{i=1}^{N} x[i]^2} $$
  _Implementation Note_: Can use efficient running sums or SIMD vectorization.

- **Peak Amplitude**:
  $$ Peak = \max(|x|) $$
  _Implementation Note_: Standard vector max operation.

- **Crest Factor**:
  $$ Crest = \frac{Peak_{linear}}{RMS_{linear}} $$
  $$ Crest_{dB} = 20 \log_{10}(Crest) $$
  _Condition_: If $RMS \approx 0$, Crest Factor is 0 (undefined).

- **DC Offset**:
  $$ DC = \frac{1}{N} \sum_{i=1}^{N} x[i] $$
  _Threshold_: Report only if $|DC| > 0.001$ (0.1%).

### 2.2 Attack/Decay Estimation
Automated timing detection based on signal envelopes.

- **Algorithm**:
  1.  **State Machine**: IDLE -> CROSSING_LOW -> MEASURING_ATTACK -> ATTACK_COMPLETE/DECAY_START -> MEASURING_DECAY -> IDLE.
  2.  **Thresholds**:
      - $Low = 10\%$ of recent max peak.
      - $High = 90\%$ of recent max peak.
  3.  **Attack Time**:
      $$ T_{attack} = T_{90\%} - T_{10\%} $$
      _Trigger_: Signal rises from below $10\%$ to above $90\%$.
  4.  **Decay Time**:
      $$ T_{decay} = T_{10\%} - T_{90\%} $$
      _Trigger_: Signal falls from above $90\%$ to below $10\%$.

### 2.3 Transient Envelopes (For Visualization)
To visualize RMS/Peak *over time* along the waveform:
- **Windowed Analysis**:
  - Instead of one value per 120s buffer, calculate separate RMS/Peak values for smaller time windows (e.g., 100ms chunks or per-pixel averaging).
- **Parallel Buffers**:
  - Maintain parallel circular buffers for `RMS_History` and `Peak_History` alongside raw audio.

## 3. Performance & Architecture
- **Thread Safety**: Calculations usually happen on Audio Thread. Results must be published atomically to the UI/Render thread.
- **Latency**: Analysis should introduce 0 latency to the audio path.
- **Optimization**: Use `vDSP` (macOS Accelerate) or `juce::FloatVectorOperations` for SIMD speedups.
