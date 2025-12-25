Refactoring completed to fix the timing controls issue.

The issue was a disconnect between the UI (`TimingSidebarSection`) and the backend (`TimingEngine`). 
The UI was emitting events via `SidebarComponent::Listener`, but the `OscilPluginEditor` (which owns the sidebar) was not listening to these timing-specific events.

Changes made:
1.  **Created `TimingSidebarListenerAdapter`**: A new adapter class in `include/tools/PluginEditor_Adapters.h` that implements `SidebarComponent::Listener` and forwards timing events (Mode, Interval, BPM, etc.) to the `TimingEngine`.
2.  **Integrated Adapter**: Updated `OscilPluginEditor` to instantiate this adapter and register it with the `SidebarComponent`.
3.  **Added Tests**: Created `tests/test_timing_controls_integration.cpp` to verify that the adapter correctly updates the `TimingEngine` configuration.

Files modified:
- `include/tools/PluginEditor_Adapters.h`
- `include/plugin/PluginEditor.h`
- `src/plugin/PluginEditor.cpp`
- `CMakeLists.txt` (Added new test)
- `tests/test_timing_controls_integration.cpp` (New test file)

Verification:
Ran `OscilTests` filtering for `TimingControlsIntegrationTest.*`. All 4 tests passed, confirming that:
- Timing Mode changes are propagated.
- Note Interval changes are propagated (with correct type conversion).
- Time Interval (ms) changes are propagated.
- BPM changes are propagated.
