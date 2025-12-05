# Feature Requirements: History System (Core)

**Status**: Draft
**Priority**: High
**Related To**: Timeline & Navigation

## 1. Overview
The History System provides the backend infrastructure to store a significant duration of past audio samples for every active oscillator. This enables users to pause, scroll back, and analyze transient events that have already occurred.

## 2. Requirements

### 2.1 Functional Requirements
- **Circular Storage**: Audio data must be stored in a fixed-size ring buffer. Oldest data is overwritten by new data.
- **Configurable Duration**: The buffer collection size (time duration) should be configurable.
    - Options: 10s, 30s, 60s, 120s.
    - Default: 30s.
- **Multi-Channel Support**: Independent history buffers for every active oscillator/track.
- **Data Integrity**: Audio samples must be stored with sufficient precision for visual analysis (float 32-bit).

### 2.2 Technical Constraints
- **Memory Efficiency**:
    - 120s @ 48kHz * 4 bytes = ~23MB per mono track.
    - System must handle ~16 tracks without exceeding reasonable RAM limits (approx 500MB total for history).
    - Allocation must happen at initialization or configuration change, never during real-time processing.
- **Thread Safety**:
    - **Write**: Audio Thread (Lock-free required).
    - **Read**: UI/Render Thread.
    - Must use thread-safe read/write pointers or atomic indices.

### 2.3 Performance Requirements
- **O(1) Access**: Random access to any point in history must be constant time.
- **Zero Allocations**: No memory allocation during the `processBlock` audio callback.

### 2.4 Integration Points
- **Oscillator Class**: Should own or reference its History Buffer.
- **SignalProcessor**: Should feed data into the History Buffer after DSP processing.
