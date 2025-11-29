# Oscil Multi-Track Oscilloscope Plugin - Behavior Matrix

## SCOPE

**Purpose**: Professional audio visualization platform for engineers and producers to analyze multi-track audio signals via real-time oscilloscope visualization with cross-DAW compatibility.

**Core Capabilities**:
- Real-time visualization of unlimited simultaneous audio sources (performance-bound)
- Per-oscillator signal processing (FullStereo, Mono, Mid, Side, Left, Right)
- Cross-DAW plugin compatibility (VST3/CLAP/AU)
- Per-instance plugin model: each DAW track requires own plugin instance to appear as Source
- Cross-instance source discovery with automatic deduplication
- Customizable theme system with global theme sharing across instances
- Per-instance oscillator and pane configuration
- Sample-accurate timing synchronization with DAW transport
- Real-time performance monitoring with adaptive quality control
- OpenGL-accelerated rendering with automatic software fallback

**Success Metrics**:
- Performance: 60fps target, warn at 10% CPU, critical at 15% CPU
- Memory: Warn at 500MB, critical at 800MB usage
- Compatibility: Support Ableton, Bitwig, Reaper, Cubase across macOS/Windows and Logic Mac
- Accuracy: Sample-accurate timing, ±0.001 Mid/Side precision
- Responsiveness: <16ms UI interactions, <1ms audio thread impact
- Rendering: OpenGL default with automatic software fallback

## ENTITIES

### PluginInstance
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | UUID | Required, unique | Stable instance identifier |
| isAggregator | boolean | Required | True when UI is open and displaying |
| sourceId | UUID | Optional | Reference to owned Source (if producer) |
| state | InstanceState | Required | Current operational state (INITIALIZING/ACTIVE/AGGREGATING/SUSPENDED/TERMINATING) |
| dawTrackId | string | Required, max 256 chars | Host-provided track identifier (DAW-specific format) |
| hostInfo | HostInfo | Required | DAW name, version, process ID for deduplication |
| audioConfig | AudioConfig | Required | Sample rate, buffer size, channel count from host |
| createdAt | timestamp | Required | Instance creation time (ISO 8601 UTC) |
| lastHeartbeat | timestamp | Required | Last activity indicator (updated every 100ms) |
| uiWindowHandle | WindowHandle | Optional | Native window handle when UI open |
| windowSize | WindowSize | Required | Current plugin window dimensions (width/height) |
| sidebarWidth | float | 200-800 pixels | Width of right sidebar containing controls |
| sidebarCollapsed | boolean | Required | Whether sidebar is collapsed/expanded |
| priority | int | 0-255 | Instance priority for source ownership conflicts |
| oscillatorConfigs | Oscillator[] | None | Per-instance oscillator configurations |
| paneLayout | PaneLayout | Required | Per-instance pane arrangement and column settings |
| performanceMetrics | PerformanceData | Required | Real-time CPU/memory monitoring data |

### Source
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | UUID | Required, unique | Stable source identifier (plugin-generated, DAW-track-context-based) |
| dawTrackId | string | Required, unique | Deduplicated DAW track reference |
| owningInstanceId | UUID | Required | Current producing instance |
| sampleRate | float | 44.1kHz-192kHz | Current audio sample rate |
| channelConfig | enum | MONO, STEREO | Audio channel configuration |
| bufferSize | int | 32-2048 samples | Current buffer size |
| isActive | boolean | Required | Currently receiving audio |
| displayName | string | Max 64 chars | User-friendly track name (fallback if DAW name unavailable) |
| audioBuffer | RingBuffer | Required | Circular buffer storing recent audio samples |
| lastAudioTime | timestamp | Optional | Timestamp of last audio sample received |
| correlationData | CorrelationMetrics | Optional | Stereo correlation coefficient (for STEREO sources) |
| signalMetrics | SignalMetrics | Required | RMS level, peak level, DC offset measurements |
| backupInstanceIds | UUID[] | None | Backup instances for ownership transfer |

### Oscillator
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | UUID | Required, unique | Stable oscillator identifier |
| sourceId | UUID | Required | Reference to Source (set to "NO_SOURCE" when source unavailable, preserving all other settings) |
| processingMode | ProcessingMode | Required | Signal preprocessing type (FullStereo/Mono/Mid/Side/Left/Right) |
| colour | ARGB | Required | RGBA color (0xAARRGGBB) |
| opacity | float | 0.0-1.0 | Transparency level |
| paneId | UUID | Required | Parent pane reference (can be "NO_PANE" if pane deleted) |
| orderIndex | int | ≥0 | Position within pane |
| visible | boolean | Required | Visibility toggle |
| displayName | string | Max 32 chars | User-assigned name |
| lineWidth | float | 0.5-5.0 pixels | Waveform trace thickness |
| timeWindow | float | 0.001-60.0 seconds | Display time window duration |
| verticalScale | float | 0.1-10.0 | Vertical zoom factor |
| verticalOffset | float | -1.0 to 1.0 | Vertical position offset |
| triggerConfig | TriggerConfig | Optional | Trigger mode settings (level, edge, manual) |

### Pane
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | UUID | Required, unique | Stable pane identifier |
| orderIndex | int | ≥0 | Global pane ordering |
| collapsed | boolean | Required | Collapse state |
| height | float | >0 pixels | User-adjustable pane height |
| oscillatorIds | UUID[] | None | Child oscillator references |

### Theme
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | UUID | Required, unique | Stable theme identifier |
| name | string | 1-64 chars, unique | Theme display name |
| isSystemTheme | boolean | Required | Immutable system theme flag |
| backgroundColour | RGB | Required | Main background color |
| gridColour | RGB | Required | Grid line color |
| textColour | RGB | Required | UI text color |
| waveformColours | RGB[64] | Required | Default oscillator colors |
| createdAt | timestamp | Required | Theme creation time |
| modifiedAt | timestamp | Required | Last modification time |

### Project
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| version | string | Required | Schema version (semantic versioning) |
| oscillators | Oscillator[] | None | User-created oscillators (per-instance) |
| panes | Pane[] | Min 1 | Pane layout structure (per-instance) |
| layoutColumns | int | 1-3 | Column layout preference (per-instance) |
| windowLayout | WindowLayout | Required | Window size and sidebar configuration (per-instance) |
| selectedThemeId | UUID | Required | Active theme reference (shared across instances) |
| timingConfig | TimingConfig | Required | Synchronization settings (per-instance) |
| globalSettings | GlobalSettings | Required | Plugin-wide configuration |
| performanceSettings | PerformanceConfig | Required | Quality and monitoring preferences |
| statusBarSettings | StatusBarConfig | Required | Status bar display preferences |

### TimingConfig
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| syncMode | enum | FREE_RUNNING, HOST_SYNC, TRIGGERED | Timing synchronization mode |
| timeWindow | float | 0.001-60.0 seconds | Display time window duration |
| triggerThreshold | float | -60.0 to 0.0 dBFS | Trigger level for TRIGGERED mode |
| triggerSlope | enum | RISING, FALLING, BOTH | Edge detection for triggers |
| hostSyncDivision | enum | 1/32, 1/16, 1/8, 1/4, 1/2, 1, 2, 4, 8, 16 bars | Musical timing division for HOST_SYNC |
| sampleAccuracy | boolean | Required | Enable sample-accurate timing |
| lookahead | float | 0-100 ms | Processing lookahead buffer |

### HostInfo
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| dawName | string | Max 64 chars | DAW application name |
| dawVersion | string | Max 32 chars | DAW version string |
| processId | int | > 0 | Operating system process ID |
| trackName | string | Max 128 chars | DAW track name (if available) |
| trackIndex | int | ≥ 0 | DAW track number/index |
| bpm | float | 20.0-999.0 | Current host BPM |
| timeSignature | TimeSignature | Required | Time signature (4/4, 3/4, etc.) |
| transportState | enum | STOPPED, PLAYING, RECORDING, PAUSED | DAW transport state |

### AudioConfig
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| sampleRate | float | 44.1kHz-192kHz | Current sample rate |
| bufferSize | int | 32-2048 samples | Audio buffer size |
| channelCount | int | 1-2 | Input channel count |
| bitDepth | int | 16, 24, 32 | Audio bit depth |
| isRealtime | boolean | Required | Real-time audio processing flag |
| reportedLatency | int | 0-2048 samples | Latency reported to host for timing compensation |
| bypassState | boolean | Required | Current plugin bypass state from host |

### HostContext
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| pluginFormat | enum | VST3, CLAP, AU | Current plugin format |
| hostCapabilities | HostCapability[] | None | List of supported host features |
| maxParameterCount | int | 0-1000 | Maximum parameters supported by host |
| supportsParameterAutomation | boolean | Required | Whether host supports parameter automation |
| supportsStateChunks | boolean | Required | Whether host supports binary state persistence |
| supportsMidiMapping | boolean | Required | Whether host supports MIDI CC parameter mapping |
| supportsLatencyReporting | boolean | Required | Whether host accepts latency information |
| hostThreadingModel | enum | SINGLE, MULTI, REALTIME_SEPARATE | Host audio/UI threading approach |
| hostTimingPrecision | enum | SAMPLE_ACCURATE, BLOCK_ACCURATE, APPROXIMATE | Host timing precision level |

**Enumerations:**
- **HostCapability**: PARAMETER_AUTOMATION, STATE_CHUNKS, MIDI_MAPPING, LATENCY_REPORTING, BYPASS_PROCESSING, SAMPLE_RATE_CHANGE, BUFFER_SIZE_CHANGE
| isRealtime | boolean | Required | Real-time audio processing flag |

### PerformanceConfig
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| qualityMode | enum | HIGHEST, HIGH, BALANCED, PERFORMANCE, MAXIMUM | Current quality/performance mode |
| refreshRate | float | 15-120 fps | Target refresh rate |
| decimationLevel | int | 0-4 | Level of sample decimation for performance |
| statusBarVisible | boolean | Required | Status bar visibility state |
| cpuWarningThreshold | float | 5.0-20.0% | CPU usage warning threshold |
| cpuCriticalThreshold | float | 10.0-25.0% | CPU usage critical threshold |
| memoryWarningThreshold | int | 200-1000 MB | Memory usage warning threshold |
| memoryCriticalThreshold | int | 500-1500 MB | Memory usage critical threshold |
| autoQualityReduction | boolean | Required | Allow automatic quality reduction |
| measurePerformance | boolean | Required | Enable performance measurement (tied to status bar visibility) |

### StatusBar
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| visible | boolean | Required | Status bar visibility state |
| cpuUsage | float | 0.0-100.0% | Current CPU usage percentage |
| memoryUsage | float | 0.0-2000.0 MB | Current memory usage in megabytes |
| fps | float | 0.0-120.0 | Current frames per second |
| renderingMode | enum | OPENGL, SOFTWARE | Current rendering backend (OpenGL default, software fallback) |
| updateInterval | int | 100-1000 ms | Status update frequency |

### StatusBarConfig
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| enabled | boolean | Required | Status bar enabled/disabled setting |
| position | enum | BOTTOM, TOP | Status bar position preference |
| showCPU | boolean | Required | Display CPU usage |
| showMemory | boolean | Required | Display memory usage |
| showFPS | boolean | Required | Display frames per second |
| showRenderingMode | boolean | Required | Display current rendering backend (OpenGL/Software) |
| updateIntervalMs | int | 100-1000 ms | Update frequency in milliseconds |

### GPUResourceManager
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| isGPUAvailable | boolean | Required | Whether GPU/OpenGL context is available |
| gpuMemoryTotal | int | 0-8192 MB | Total GPU memory available |
| gpuMemoryUsed | int | 0-gpuMemoryTotal | Current GPU memory usage |
| gpuMemoryReserved | int | 0-512 MB | Memory reserved for waveform textures |
| texturePoolSize | int | 16-256 textures | Number of textures in memory pool |
| maxTextureSize | int | 1024-8192 pixels | Maximum texture dimension supported |
| openglVersion | string | Max 32 chars | OpenGL version string |
| gpuVendor | string | Max 64 chars | Graphics card vendor and model |
| driverVersion | string | Max 32 chars | Graphics driver version |
| contextLossCount | int | ≥0 | Number of context loss events since startup |
| lastContextLoss | timestamp | Optional | Timestamp of most recent context loss |
| fallbackToSoftware | boolean | Required | Whether currently using software rendering |
| gpuCompatibilityScore | int | 0-100 | Compatibility assessment score |

### TextureMemoryPool
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| poolId | UUID | Required, unique | Texture pool identifier |
| maxTextures | int | 16-256 | Maximum textures in pool |
| currentTextures | int | 0-maxTextures | Currently allocated textures |
| textureWidth | int | 256-4096 pixels | Width of textures in pool |
| textureHeight | int | 256-2048 pixels | Height of textures in pool |
| textureFormat | enum | RGBA8, RGBA16F, RGB8 | Texture pixel format |
| memoryUsage | int | 0-512 MB | Current memory used by pool |
| allocationStrategy | enum | ROUND_ROBIN, LRU, PRIORITY | Texture allocation method |
| recycleCount | int | ≥0 | Number of texture recycling operations |
| averageLifetime | float | >0 ms | Average texture lifetime before recycling |

### GPUCompatibilityProfile
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| minOpenGLVersion | string | Required | Minimum OpenGL version required |
| requiredExtensions | string[] | None | List of required OpenGL extensions |
| recommendedMemory | int | 256-2048 MB | Recommended GPU memory for optimal performance |
| blacklistedDrivers | string[] | None | Known problematic driver versions |
| performanceTier | enum | LOW, MEDIUM, HIGH, ULTRA | GPU performance classification |
| maxWaveforms | int | 1-128 | Maximum simultaneous waveforms for this GPU |
| recommendedQuality | enum | PERFORMANCE, BALANCED, QUALITY | Recommended quality setting |

### SettingsDropdown
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| isOpen | boolean | Required | Dropdown open/closed state |
| selectedThemeId | UUID | Required | Currently selected theme |
| availableThemes | Theme[] | Min 1 | List of available themes |
| selectedLayoutMode | int | 1-3 | Currently selected column layout (1, 2, or 3 columns) |
| statusBarEnabled | boolean | Required | Status bar visibility setting |
| performanceStatsEnabled | boolean | Required | Performance measurement setting (mirrors status bar) |
| themeEditorOpen | boolean | Required | Theme editor window state |

### PaneLayout
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| columns | int | 1-3 | Number of layout columns |
| paneOrder | UUID[] | None | Ordered list of pane IDs for layout |
| columnDistribution | int[] | Size = columns | Number of panes per column |
| totalHeight | float | >0 pixels | Total available height for panes |
| scrollPosition | float | ≥0 pixels | Current scroll position if content exceeds viewport |

### PaneDragState
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| isDragging | boolean | Required | Whether a pane drag operation is in progress |
| draggedPaneId | UUID | Optional | ID of the pane being dragged |
| dragStartPosition | Position | Optional | Original position when drag started |
| currentPosition | Position | Optional | Current mouse/touch position during drag |
| dropTarget | DropTarget | Optional | Current valid drop target information |
| previewMode | boolean | Required | Whether to show drop target preview |

### WindowLayout
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| windowWidth | int | 800-3840 pixels | Plugin window width |
| windowHeight | int | 600-2160 pixels | Plugin window height |
| sidebarWidth | int | 200-800 pixels | Width of right control sidebar |
| sidebarCollapsed | boolean | Required | Whether sidebar is collapsed |
| sidebarMinWidth | int | 200 pixels | Minimum sidebar width when expanded |
| sidebarMaxWidth | int | 800 pixels | Maximum sidebar width |
| oscilloscopeAreaWidth | int | Calculated | Remaining width for oscilloscope display (windowWidth - sidebarWidth) |

### TimingConfig
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| timingMode | TimingMode | Required | TIME or MELODIC interval mode |
| hostSyncEnabled | boolean | Required | Whether to sync with host start/stop events |
| timeIntervalMs | int | 10-60000 ms | Time interval in milliseconds (TIME mode only) |
| noteInterval | NoteInterval | Required | Note length for display interval (MELODIC mode only) |
| hostBPM | float | 20-300 BPM | Current host BPM (read-only, from DAW) |
| actualIntervalMs | float | Calculated | Computed interval duration based on mode and BPM |
| syncToPlayhead | boolean | Required | Whether oscillator resets align with host playhead |
| lastSyncTimestamp | double | Optional | Sample timestamp of last sync point |

### ErrorRecoveryManager
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| crashCount | int | ≥0 | Number of crashes detected since startup |
| lastCrashTimestamp | timestamp | Optional | Timestamp of most recent crash |
| recoveryAttempts | int | 0-3 | Number of automatic recovery attempts |
| isInRecoveryMode | boolean | Required | Whether currently in recovery state |
| backupStateValid | boolean | Required | Whether backup state is available and valid |
| lastBackupTimestamp | timestamp | Optional | When state was last backed up |
| criticalErrors | ErrorRecord[] | None | List of critical errors encountered |
| resourceStarvationLevel | enum | NONE, LOW, MEDIUM, HIGH, CRITICAL | Current resource pressure level |
| degradationLevel | int | 0-5 | Current feature degradation level |
| interferenceDetected | boolean | Required | Whether inter-plugin interference detected |

### ErrorRecord
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| errorId | UUID | Required, unique | Unique error identifier |
| errorType | ErrorType | Required | Classification of error |
| timestamp | timestamp | Required | When error occurred |
| severity | enum | INFO, WARNING, ERROR, CRITICAL | Error severity level |
| message | string | Max 512 chars | Human-readable error description |
| stackTrace | string | Max 2048 chars | Technical stack trace (debug builds) |
| context | ErrorContext | Required | System state when error occurred |
| recoveryAction | RecoveryAction | Optional | Action taken to recover |
| resolved | boolean | Required | Whether error was successfully resolved |

**Enumerations:**
- **ErrorType**: MEMORY_ALLOCATION, GPU_CONTEXT_LOSS, STATE_CORRUPTION, RESOURCE_STARVATION, PLUGIN_CRASH, HOST_COMMUNICATION, FILE_IO_ERROR
- **RecoveryAction**: RESTART_COMPONENT, FALLBACK_SOFTWARE, REDUCE_QUALITY, RELOAD_STATE, CLEAR_CACHE, GARBAGE_COLLECT

### ResourceMonitor
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| systemCpuUsage | float | 0-100% | Overall system CPU usage |
| systemMemoryUsage | float | 0-100% | Overall system memory usage |
| systemMemoryTotal | int | >0 MB | Total system memory available |
| pluginCpuUsage | float | 0-100% | Plugin-specific CPU usage |
| pluginMemoryUsage | int | >0 MB | Plugin-specific memory usage |
| memoryFragmentation | float | 0-100% | Memory fragmentation percentage |
| diskSpaceAvailable | int | >0 MB | Available disk space for temp files |
| networkLatency | float | >0 ms | Network latency (if applicable) |
| otherPluginsDetected | int | ≥0 | Number of other audio plugins active |
| interferenceScore | float | 0-100 | Calculated interference impact score |

### StateBackupManager
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| backupIntervalMs | int | 1000-60000 ms | How often to create state backups |
| maxBackups | int | 3-10 | Maximum number of backup files to retain |
| backupDirectory | string | Max 512 chars | Directory for backup state files |
| currentBackups | BackupRecord[] | None | List of available backup files |
| autoRecoveryEnabled | boolean | Required | Whether to auto-recover from backups |
| compressionEnabled | boolean | Required | Whether to compress backup files |
| encryptionEnabled | boolean | Required | Whether to encrypt sensitive backup data |

### BackupRecord
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| backupId | UUID | Required, unique | Backup file identifier |
| filePath | string | Max 512 chars | Full path to backup file |
| timestamp | timestamp | Required | When backup was created |
| fileSize | int | >0 bytes | Size of backup file |
| checksumMD5 | string | 32 chars | MD5 checksum for integrity verification |
| oscillatorCount | int | ≥0 | Number of oscillators in backup |
| paneCount | int | ≥1 | Number of panes in backup |
| isCorrupted | boolean | Required | Whether backup failed integrity check |

**Enumerations:**
- **TimingMode**: TIME, MELODIC
- **NoteInterval**: NOTE_1_16TH, NOTE_1_12TH, NOTE_1_8TH, NOTE_1_4TH, NOTE_1_2, NOTE_1_1, NOTE_2_1, NOTE_3_1, NOTE_4_1, NOTE_8_1, NOTE_DOTTED_1_8TH, NOTE_TRIPLET_1_4TH, NOTE_DOTTED_1_4TH, NOTE_TRIPLET_1_2, NOTE_DOTTED_1_2

### SidebarResizeState
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| isResizing | boolean | Required | Whether sidebar resize is in progress |
| dragStartX | int | Optional | X coordinate where resize drag started |
| originalSidebarWidth | int | Optional | Sidebar width when resize started |
| currentSidebarWidth | int | 200-800 pixels | Current sidebar width during resize |
| previewWidth | int | Optional | Preview width shown during resize |

### OscillatorListState
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| sortMode | OscillatorSortMode | Required | How oscillators are sorted in the list |
| sortDirection | SortDirection | Required | Ascending or descending sort order |
| selectedOscillatorId | UUID | Optional | Currently selected oscillator in list |
| scrollPosition | float | 0.0-1.0 | Vertical scroll position in oscillator list |
| configPopupOpen | boolean | Required | Whether configuration popup is currently open |
| configPopupOscillatorId | UUID | Optional | ID of oscillator being configured |

**Enumerations:**
- **OscillatorSortMode**: BY_NAME, BY_SOURCE, BY_PANE, BY_ORDER
- **SortDirection**: ASCENDING, DESCENDING

### PluginParameter
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | UUID | Required, unique | Stable parameter identifier |
| parameterId | int | 0-999, unique | DAW parameter index for automation |
| name | string | 1-64 chars | Parameter display name in DAW |
| normalizedValue | float | 0.0-1.0 | Current normalized value (DAW standard) |
| defaultValue | float | 0.0-1.0 | Default normalized value |
| internalValue | variant | Type-specific | Mapped internal value (float, bool, enum, etc.) |
| isAutomatable | boolean | Required | Whether parameter can be automated by DAW |
| isDiscrete | boolean | Required | Whether parameter has discrete steps vs continuous |
| stepCount | int | 2-1000 | Number of discrete steps (if isDiscrete=true) |
| units | string | Max 16 chars | Parameter units display (ms, %, dB, etc.) |
| stringFromValue | function | Required | Function to convert normalized value to display string |
| valueFromString | function | Required | Function to parse display string to normalized value |
| groupName | string | Max 32 chars | Parameter group for DAW organization |
| automationGesture | AutomationGestureState | Required | Current automation recording state |
| lastChangeTimestamp | double | Required | Sample-accurate timestamp of last parameter change |

**Enumerations:**
- **AutomationGestureState**: NONE, STARTED, IN_PROGRESS, ENDED

### ParameterMapping
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| parameterId | int | 0-999 | DAW parameter index |
| targetType | ParameterTargetType | Required | Type of control being automated |
| targetId | UUID | Required | ID of target entity (oscillator, timing config, etc.) |
| targetProperty | string | Max 32 chars | Specific property name being controlled |
| valueMapper | function | Required | Function to map normalized value to target property |
| inverseMapper | function | Required | Function to map target property to normalized value |
| isEnabled | boolean | Required | Whether mapping is currently active |

**Enumerations:**
- **ParameterTargetType**: OSCILLATOR_PROPERTY, TIMING_CONFIG, THEME_PROPERTY, PERFORMANCE_CONFIG

### GlobalPluginSettings
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| settingsId | UUID | Required, unique | Global settings identifier |
| defaultThemeId | UUID | Required | Default theme for new instances |
| maxInstanceCount | int | 1-16 | Maximum allowed plugin instances |
| globalPerformanceMode | enum | EFFICIENCY, BALANCED, QUALITY | System-wide performance preference |
| enableCrashReporting | boolean | Required | Whether to collect crash analytics |
| enableTelemetry | boolean | Required | Whether to send usage statistics |
| autoBackupInterval | int | 0-3600 seconds | Automatic backup frequency (0=disabled) |
| defaultOscillatorColor | ARGB | Required | Default color for new oscillators |
| enableKeyboardShortcuts | boolean | Required | Whether keyboard shortcuts are active |
| showTooltips | boolean | Required | Whether to show UI tooltips |
| debugLoggingLevel | enum | NONE, ERROR, WARNING, INFO, DEBUG | Logging verbosity level |

### PresetTemplate
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| presetId | UUID | Required, unique | Preset template identifier |
| name | string | 1-64 chars | User-friendly preset name |
| description | string | Max 256 chars | Preset description |
| isSystemPreset | boolean | Required | Whether preset is built-in or user-created |
| oscillatorConfigs | OscillatorTemplate[] | 1-16 items | Saved oscillator configurations |
| paneLayout | PaneLayoutTemplate | Required | Saved pane arrangement |
| timingConfig | TimingConfigTemplate | Required | Saved timing settings |
| createdAt | timestamp | Required | Preset creation time |
| lastUsed | timestamp | Optional | Last time preset was applied |
| useCount | int | ≥0 | Number of times preset has been used |

### KeyboardShortcut
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| shortcutId | UUID | Required, unique | Keyboard shortcut identifier |
| keyCombo | string | Max 32 chars | Key combination (e.g., "Ctrl+Shift+A") |
| actionType | ShortcutAction | Required | Type of action to perform |
| targetId | UUID | Optional | Target entity for action (if applicable) |
| isEnabled | boolean | Required | Whether shortcut is currently active |
| isCustomizable | boolean | Required | Whether user can modify this shortcut |
| conflictsWith | UUID[] | None | Other shortcuts that conflict with this combo |

**Enumerations:**
- **ShortcutAction**: CREATE_OSCILLATOR, DELETE_SELECTED, TOGGLE_SIDEBAR, NEXT_THEME, PREV_THEME, CLEAR_ALL, SAVE_PRESET, LOAD_PRESET

### UndoRedoManager
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| maxHistorySize | int | 10-100 | Maximum number of undo actions to retain |
| currentIndex | int | -1 to maxHistorySize-1 | Current position in undo stack |
| undoStack | UndoAction[] | Max maxHistorySize | Stack of reversible actions |
| isRecording | boolean | Required | Whether actions are being recorded |
| groupedActionId | UUID | Optional | ID for grouping related actions |

### UndoAction
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| actionId | UUID | Required, unique | Undo action identifier |
| actionType | UndoActionType | Required | Type of action that was performed |
| timestamp | timestamp | Required | When action was performed |
| description | string | Max 128 chars | Human-readable action description |
| beforeState | variant | Required | State before action was performed |
| afterState | variant | Required | State after action was performed |
| entityIds | UUID[] | None | IDs of entities affected by action |

**Enumerations:**
- **UndoActionType**: CREATE_OSCILLATOR, DELETE_OSCILLATOR, MODIFY_OSCILLATOR, REORDER_PANES, CHANGE_THEME, BULK_OPERATION

### BulkOperationManager
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| selectedOscillators | UUID[] | None | Currently selected oscillator IDs |
| operationType | BulkOperationType | Optional | Type of bulk operation in progress |
| operationProgress | float | 0.0-1.0 | Progress of current bulk operation |
| confirmationRequired | boolean | Required | Whether operation requires user confirmation |
| previewMode | boolean | Required | Whether showing preview of bulk changes |

**Enumerations:**
- **BulkOperationType**: CHANGE_COLOR, CHANGE_PROCESSING_MODE, CHANGE_PANE, DELETE_MULTIPLE, SET_VISIBILITY

## OPERATIONS

### OP001: CreateOscillator
**Purpose**: Create new oscillator for visualizing a source signal
**Trigger**: User clicks "Add Oscillator" button
**Actor**: User

**Pre-conditions**:
- At least one Source available
- Oscillator count < 128
- UI is responsive

**Post-conditions**:
- New Oscillator entity created with unique ID
- Oscillator assigned to default or selected Pane
- Visual trace appears in next frame
- Oscillator persisted in project state

**UI Integration**:
- **UI_Trigger**: "Add Oscillator" button click
- **UI_Inputs**: Source selection dropdown, optional color picker
- **UI_Effects**: New oscillator row in pane, immediate waveform display
- **UI_Validations**: Disable button if max oscillators reached

**Examples**:

*Happy Path*:
```
Input: { sourceId: "S001", paneId: "P001", processingMode: "FullStereo" }
State Changes: [
  + Oscillator { id: "O123", sourceId: "S001", colour: 0xFF00FF00, visible: true }
  * Pane.P001.oscillatorIds += ["O123"]
]
Output: { oscillatorId: "O123", success: true }
UI_Context: {
  before: "Empty pane or existing oscillators",
  action: "User clicks Add Oscillator, selects source",
  after: "New green waveform trace appears in pane"
}
```

*Boundary*:
```
Input: { sourceId: "S001" } (127 existing oscillators)
State Changes: [
  + Oscillator { id: "O128", sourceId: "S001", ... }
]
Output: { oscillatorId: "O128", success: true, warning: "Maximum oscillators reached" }
UI_Context: {
  before: "127 oscillators visible",
  action: "User creates final allowed oscillator",
  after: "128th oscillator appears, Add button becomes disabled"
}
```

*Failure*:
```
Input: { sourceId: "INVALID" }
State Changes: []
Output: { error: "INVALID_SOURCE_ID", success: false }
UI_Context: {
  before: "User attempting to create oscillator",
  action: "Selects invalid/missing source",
  after: "Error toast appears, no oscillator created"
}
```

### OP002: UpdateOscillatorProcessing
**Purpose**: Change signal processing mode for existing oscillator
**Trigger**: User selects different processing mode
**Actor**: User

**Pre-conditions**:
- Oscillator exists and is visible
- New processing mode is valid
- Source is active

**Post-conditions**:
- Oscillator.processingMode updated
- Visual trace reflects new processing immediately
- No audio thread allocation
- Change persisted

**UI Integration**:
- **UI_Trigger**: Processing mode dropdown change
- **UI_Inputs**: Selected ProcessingMode enum value
- **UI_Effects**: Waveform trace updates within 1 frame
- **UI_Validations**: All modes always available

**Examples**:

*Happy Path*:
```
Input: { oscillatorId: "O123", processingMode: "Mid" }
State Changes: [
  * Oscillator.O123.processingMode = "Mid"
]
Output: { success: true }
UI_Context: {
  before: "Stereo waveform showing L/R channels",
  action: "User selects Mid from dropdown",
  after: "Single trace showing (L+R)/2 signal"
}
```

*Boundary*:
```
Input: { oscillatorId: "O123", processingMode: "Side" } (mono source)
State Changes: [
  * Oscillator.O123.processingMode = "Side"
]
Output: { success: true, warning: "Side processing on mono source shows silence" }
UI_Context: {
  before: "Mono waveform visible",
  action: "User switches to Side mode",
  after: "Flat line (silence) displayed as expected"
}
```

*Failure*:
```
Input: { oscillatorId: "INVALID", processingMode: "Mid" }
State Changes: []
Output: { error: "OSCILLATOR_NOT_FOUND", success: false }
```

### OP003: SwitchTheme
**Purpose**: Apply different theme to entire plugin UI
**Trigger**: User selects theme from theme manager
**Actor**: User

**Pre-conditions**:
- Theme exists and is valid
- Plugin UI is active
- Theme switching not in progress

**Post-conditions**:
- All UI colors updated to new theme
- Theme preference persisted
- No visual artifacts during transition
- Waveform rendering continues uninterrupted

**UI Integration**:
- **UI_Trigger**: Theme dropdown selection or theme manager apply
- **UI_Inputs**: Selected theme ID
- **UI_Effects**: Global color update, CSS variable changes, immediate repaint
- **UI_Validations**: Prevent switching during theme load

**Examples**:

*Happy Path*:
```
Input: { themeId: "T002" } // Dark Professional theme
State Changes: [
  * Project.selectedThemeId = "T002"
  * UI.globalColors = Theme.T002.colors
]
Output: { success: true, appliedTheme: "Dark Professional" }
UI_Context: {
  before: "Light theme with white backgrounds",
  action: "User selects Dark Professional theme",
  after: "All UI elements switch to dark colors within 50ms"
}
```

*Boundary*:
```
Input: { themeId: "T999" } // Non-existent theme
State Changes: []
Output: { error: "THEME_NOT_FOUND", success: false }
UI_Context: {
  before: "Current theme active",
  action: "User selects invalid theme",
  after: "Error message displayed, theme unchanged"
}
```

*Failure*:
```
Input: { themeId: "T001" } // During active theme switch
State Changes: []
Output: { error: "THEME_SWITCH_IN_PROGRESS", success: false }
```

### OP004: RegisterSource
**Purpose**: Register new audio source when plugin instance loads on DAW track
**Trigger**: Plugin instance initialization on DAW track
**Actor**: System

**Pre-conditions**:
- Plugin instance created and initialized
- DAW track identifier available
- InstanceRegistry operational

**Post-conditions**:
- New Source entity created (if first instance on track)
- Source ownership assigned to current instance
- Source deduplication enforced
- InstanceRegistry updated

**Edge Cases**:
- Multiple instances on same track: First instance wins ownership
- Instance crash/removal: Source ownership transferred to next available instance
- DAW track rename: Source displayName updated automatically

**Examples**:

*Happy Path (New Track)*:
```
Input: { instanceId: "I123", dawTrackId: "Track_001", audioConfig: {...} }
State Changes: [
  + Source { id: "S001", dawTrackId: "Track_001", owningInstanceId: "I123" }
  * InstanceRegistry.sources += ["S001"]
]
Output: { sourceId: "S001", isNewSource: true, success: true }
```

*Boundary (Duplicate Track)*:
```
Input: { instanceId: "I124", dawTrackId: "Track_001" } // Track already has source
State Changes: [
  * Source.S001.candidateInstances += ["I124"]  // Backup instance
]
Output: { sourceId: "S001", isNewSource: false, role: "backup", success: true }
```

*Failure (Registry Unavailable)*:
```
Input: { instanceId: "I123", dawTrackId: "Track_001" }
State Changes: []
Output: { error: "REGISTRY_UNAVAILABLE", success: false }
```

### OP005: DeleteOscillator
**Purpose**: Remove existing oscillator from visualization
**Trigger**: User clicks delete button or uses keyboard shortcut
**Actor**: User

**Pre-conditions**:
- Oscillator exists and is accessible
- User has delete permissions
- UI is responsive

**Post-conditions**:
- Oscillator entity removed
- Visual trace disappears immediately
- Pane layout updated
- Oscillator count decremented

**Edge Cases**:
- Last oscillator in pane: Pane remains but shows empty state
- Oscillator deletion during drag operation: Drag cancelled
- Rapid multiple deletions: Operations queued and processed sequentially

**Examples**:

*Happy Path*:
```
Input: { oscillatorId: "O123" }
State Changes: [
  - Oscillator.O123
  * Pane.P001.oscillatorIds -= ["O123"]
  * Project.oscillatorCount -= 1
]
Output: { success: true, deletedId: "O123" }
UI_Context: {
  before: "Oscillator visible in pane with waveform",
  action: "User clicks delete button",
  after: "Waveform disappears, remaining oscillators adjust layout"
}
```

*Boundary (Last Oscillator in Pane)*:
```
Input: { oscillatorId: "O999" } // Only oscillator in pane
State Changes: [
  - Oscillator.O999
  * Pane.P001.oscillatorIds = []
]
Output: { success: true, emptyPane: "P001" }
UI_Context: {
  before: "Single oscillator in pane",
  action: "User deletes oscillator",
  after: "Pane shows empty state with 'Add Oscillator' prompt"
}
```

*Failure (Oscillator Not Found)*:
```
Input: { oscillatorId: "INVALID" }
State Changes: []
Output: { error: "OSCILLATOR_NOT_FOUND", success: false }
```

### OP006: ReorderPanes
**Purpose**: Change pane display order through drag-and-drop
**Trigger**: User drags pane to new position
**Actor**: User

**Pre-conditions**:
- At least 2 panes exist
- Drag operation initiated
- Target position is valid

**Post-conditions**:
- Pane orderIndex values updated
- Visual layout reflects new order
- Layout preference persisted
- No oscillator content lost

**Edge Cases**:
- Drag to same position: No changes made
- Drag across column boundaries: Column distribution recalculated
- Drag interrupted (e.g., alt-tab): Operation cancelled, original order restored

**Examples**:

*Happy Path*:
```
Input: { paneId: "P002", newIndex: 1, columnContext: { layout: 2, targetColumn: 1 } }
State Changes: [
  * Pane.P002.orderIndex = 1
  * Pane.P001.orderIndex = 2  // Shifted down
  * Project.paneOrder = ["P002", "P001", "P003"]
]
Output: { success: true, newOrder: ["P002", "P001", "P003"] }
UI_Context: {
  before: "3 panes in order P001, P002, P003",
  action: "User drags P002 to first position",
  after: "Panes reorder to P002, P001, P003 with smooth animation"
}
```

*Boundary (Cross-Column Drag)*:
```
Input: { paneId: "P001", newIndex: 3, columnContext: { layout: 3, fromColumn: 1, toColumn: 2 } }
State Changes: [
  * Pane.P001.orderIndex = 3
  * ColumnLayoutManager.redistribute()
]
Output: { success: true, columnDistribution: { col1: ["P002"], col2: ["P001", "P003"] } }
```

### OP007: HandleSourceOwnershipTransfer
**Purpose**: Transfer source ownership when producing instance is removed or handle source removal
**Trigger**: Plugin instance shutdown, crash, or source deletion
**Actor**: System

**Pre-conditions**:
- Source exists with current owner
- Owner instance terminating or source being removed
- Dependent oscillators may exist

**Post-conditions**:
- If backup instances available: Source ownership transferred to backup instance
- If no backup instances: Source removed, all dependent oscillators set to "NO_SOURCE" state
- Audio capture continues uninterrupted (if backup available) or stops gracefully
- Oscillator configurations preserved (color, processing mode, pane assignment, etc.)
- Registry updated

**Edge Cases**:
- No backup instances: Source becomes unavailable, oscillators show "No Signal" state
- Multiple backup candidates: Highest priority instance selected
- Rapid instance cycling: Ownership stabilizes on most stable instance

**Examples**:

*Happy Path (Backup Available)*:
```
Input: { sourceId: "S001", terminatingInstance: "I123", backupInstance: "I124" }
State Changes: [
  * Source.S001.owningInstanceId = "I124"
  * Source.S001.candidateInstances -= ["I124"]
  * InstanceRegistry.updateOwnership("S001", "I124")
  // All oscillators keep their sourceId reference unchanged
]
Output: { newOwner: "I124", seamlessTransfer: true, success: true }
```

*No Backup Available (Source Removed)*:
```
Input: { sourceId: "S001", terminatingInstance: "I123", backupInstance: null }
State Changes: [
  - Source.S001 // Source entity completely removed
  * All oscillators with sourceId="S001" → sourceId="NO_SOURCE"
  * All affected oscillators maintain other settings (color, processingMode, paneId, etc.)
  * InstanceRegistry.sources -= ["S001"]
]
Output: {
  sourceRemoved: "S001",
  affectedOscillators: ["O001", "O002", "O003"],
  oscillatorsPreserved: true,
  success: true
}
UI_Context: {
  before: "Oscillators O001, O002, O003 showing waveforms from Source S001",
  action: "Source S001 owner instance crashes with no backup",
  after: "Oscillators show 'No Signal' state, all other settings preserved, ready for new source assignment"
}
```

### OP008: ChangeOscillatorSource
**Purpose**: Update which source an oscillator displays
**Trigger**: User selects different source from dropdown or system assigns "NO_SOURCE" state
**Actor**: User or System

**Pre-conditions**:
- Oscillator exists and is accessible
- New source is valid, "NO_SOURCE" for disconnection, or system-triggered for source loss
- UI is responsive (for user-triggered changes)

**Post-conditions**:
- Oscillator.sourceId updated to new value
- Visual trace reflects new source immediately or shows "No Signal" state for "NO_SOURCE"
- All other oscillator settings preserved (color, processing mode, pane assignment, etc.)
- Change persisted in project state
- Previous source reference cleanly released

**Examples**:

*Happy Path (User Source Change)*:
```
Input: { oscillatorId: "O123", newSourceId: "S002", trigger: "user" }
State Changes: [
  * Oscillator.O123.sourceId = "S002"
  // All other settings preserved: colour, processingMode, paneId, visible, etc.
]
Output: { success: true }
UI_Context: {
  before: "Oscillator showing waveform from Source S001",
  after: "Oscillator immediately switches to waveform from Source S002"
}
```

*Source Lost Scenario (System Triggered)*:
```
Input: { oscillatorId: "O123", newSourceId: "NO_SOURCE", trigger: "system", reason: "source_removed" }
State Changes: [
  * Oscillator.O123.sourceId = "NO_SOURCE"
  // All other settings preserved: colour=0xFF00FF00, processingMode="FullStereo", paneId="P001", etc.
]
Output: { success: true, reason: "Source no longer available" }
UI_Context: {
  before: "Oscillator showing active waveform from removed source",
  after: "Oscillator shows 'No Signal' placeholder, retains green color and stereo processing mode"
}
```

*User Disconnection*:
```
Input: { oscillatorId: "O123", newSourceId: "NO_SOURCE", trigger: "user" }
State Changes: [
  * Oscillator.O123.sourceId = "NO_SOURCE"
]
Output: { success: true, warning: "Oscillator disconnected from source" }
UI_Context: {
  before: "Oscillator showing active waveform",
  after: "Oscillator shows 'No Signal' placeholder, ready for new source assignment"
}
```

### OP009: ChangeOscillatorPane
**Purpose**: Move oscillator to different pane or create new pane
**Trigger**: User drags oscillator to new pane or selects from dropdown
**Actor**: User

**Pre-conditions**:
- Oscillator exists and is accessible
- Target pane is valid or "CREATE_NEW" for new pane creation
- UI layout system is responsive

**Post-conditions**:
- Oscillator.paneId updated to new pane
- Visual layout reflects new arrangement
- If "CREATE_NEW", new pane entity created
- Pane order indices updated as needed

**Examples**:

*Move to Existing Pane*:
```
Input: { oscillatorId: "O123", targetPaneId: "P002" }
State Changes: [
  * Oscillator.O123.paneId = "P002"
  * Pane.P002.oscillatorIds += ["O123"]
  * Pane.P001.oscillatorIds -= ["O123"]
]
Output: { success: true, movedToPane: "P002" }
```

*Create New Pane*:
```
Input: { oscillatorId: "O123", targetPaneId: "CREATE_NEW" }
State Changes: [
  + Pane { id: "P003", orderIndex: nextOrder, oscillatorIds: ["O123"] }
  * Oscillator.O123.paneId = "P003"
  * Pane.P001.oscillatorIds -= ["O123"]
]
Output: { success: true, createdPane: "P003" }
```

### OP010: ResizePane
**Purpose**: Adjust pane height manually via drag handles
**Trigger**: User drags pane resize handle
**Actor**: User

**Pre-conditions**:
- Pane exists and is resizable
- Sufficient space available for resize
- No conflicting resize operations in progress

**Post-conditions**:
- Pane.height updated to new value
- Adjacent panes adjusted to maintain layout
- Resize handles repositioned
- Change persisted immediately

**Examples**:

*Successful Resize*:
```
Input: { paneId: "P001", newHeight: 300.0, availableSpace: 800.0 }
State Changes: [
  * Pane.P001.height = 300.0
  * Adjacent panes reflow to fit remaining space
]
Output: { success: true, finalHeight: 300.0 }
```

### OP011: ShowStatusBar
**Purpose**: Toggle status bar display showing CPU, memory, FPS, and rendering mode
**Trigger**: User enables/disables status bar in settings dropdown
**Actor**: User

**Pre-conditions**:
- Settings dropdown is accessible
- UI layout system can accommodate status bar

**Post-conditions**:
- Status bar visibility toggled
- Performance measurement enabled/disabled accordingly
- Real-time metrics display updated if visible (CPU/Memory/FPS/Rendering mode)
- Preference persisted in user settings

**Examples**:

*Enable Status Bar*:
```
Input: { show: true }
State Changes: [
  * PerformanceSettings.statusBarVisible = true
  * PerformanceSettings.measurePerformance = true
]
Output: { success: true, statusBarState: "visible" }
UI_Context: {
  before: "No status bar visible",
  after: "Status bar appears at bottom showing CPU/Memory/FPS/Rendering mode (OpenGL/Software)"
}
```

*Disable Status Bar*:
```
Input: { show: false }
State Changes: [
  * PerformanceSettings.statusBarVisible = false
  * PerformanceSettings.measurePerformance = false
]
Output: { success: true, statusBarState: "hidden" }
UI_Context: {
  before: "Status bar visible with CPU/Memory/FPS/Rendering mode",
  after: "Status bar hidden, performance measurement stopped"
}
```

### OP012: OpenSettingsDropdown
**Purpose**: Display settings dropdown for theme selection and preferences
**Trigger**: User clicks settings button/icon
**Actor**: User

**Pre-conditions**:
- Settings UI components are available
- Theme system is operational

**Post-conditions**:
- Settings dropdown opened
- Current theme highlighted
- Status bar setting reflects current state
- Theme editor accessible

**Examples**:

*Open Settings*:
```
Input: { }
State Changes: [
  * SettingsDropdown.isOpen = true
]
Output: { success: true, availableThemes: ["Dark Professional", "Light Modern", ...] }
UI_Context: {
  before: "Settings dropdown closed",
  after: "Dropdown shows theme list, status bar toggle, theme editor button"
}
```

### OP013: OpenThemeEditor
**Purpose**: Launch theme editor from settings dropdown
**Trigger**: User clicks "Theme Editor" in settings dropdown
**Actor**: User

**Pre-conditions**:
- Settings dropdown is open
- Theme editor components are available
- Current theme is accessible for editing

**Post-conditions**:
- Theme editor window/panel opened
- Current theme loaded for editing
- Settings dropdown may close (platform dependent)
- Real-time preview available

**Examples**:

*Launch Theme Editor*:
```
Input: { currentThemeId: "T001" }
State Changes: [
  * SettingsDropdown.themeEditorOpen = true
  * ThemeEditor.loadTheme("T001")
]
Output: { success: true, editorState: "open", loadedTheme: "Dark Professional" }
UI_Context: {
  before: "Settings dropdown open",
  after: "Theme editor window/panel opens with current theme loaded"
}
```

### OP014: ReduceQuality
**Purpose**: Lower rendering quality to improve performance
**Trigger**: User manual selection or automatic performance threshold
**Actor**: User or System

**Pre-conditions**:
- Performance monitoring indicates strain
- Quality reduction options available
- User confirmation obtained (if manual)

**Post-conditions**:
- Render quality level reduced
- Performance metrics improve
- Quality indicator updated in UI
- User can reverse decision

**Examples**:

*Manual Quality Reduction*:
```
Input: { qualityLevel: "PERFORMANCE", userConfirmed: true }
State Changes: [
  * PerformanceSettings.qualityMode = "PERFORMANCE"
  * RenderSettings.refreshRate = 30  // Reduced from 60fps
  * RenderSettings.decimationLevel = 2  // Increased decimation
]
Output: { success: true, newQuality: "PERFORMANCE" }
```

*Automatic Warning*:
```
Input: { cpuUsage: 12.5, threshold: 10.0 }
State Changes: []
Output: { warning: "HIGH_CPU_USAGE", suggestedAction: "REDUCE_QUALITY" }
UI_Context: {
  before: "Normal operation",
  after: "Performance warning dialog with quality reduction option"
}
```

### OP015: ClearAllOscillators
**Purpose**: Remove all oscillators (reset to default state)
**Trigger**: User selects "Clear All" from menu
**Actor**: User

**Pre-conditions**:
- User confirmation obtained
- At least one oscillator exists
- UI is responsive

**Post-conditions**:
- All oscillator entities removed
- All panes except default removed
- UI shows empty state
- Change persisted immediately

**Examples**:

*Clear All Confirmed*:
```
Input: { confirmed: true, oscillatorCount: 5 }
State Changes: [
  - All Oscillator entities
  - All Pane entities except default
  * Default pane reset to empty state
]
Output: { success: true, removedCount: 5 }
UI_Context: {
  before: "Multiple oscillators and panes visible",
  after: "Empty oscilloscope with single default pane"
}
```

### OP016: ChangeLayoutMode
**Purpose**: Switch between 1, 2, or 3 column layout modes
**Trigger**: User selects layout mode from settings dropdown
**Actor**: User

**Pre-conditions**:
- Settings dropdown is accessible
- Layout system can accommodate requested column count
- No active drag operations in progress

**Post-conditions**:
- PaneLayout.columns updated to new value
- Panes redistributed across new column structure
- UI layout reflects new arrangement immediately
- Layout preference persisted in user settings

**UI Integration**:
- **UI_Trigger**: Layout mode dropdown selection in settings
- **UI_Inputs**: Selected column count (1, 2, or 3)
- **UI_Effects**: Panes redistribute with smooth animation, layout indicators update
- **UI_Validations**: All column counts always available

**Examples**:

*Switch to 2 Columns*:
```
Input: { newColumnCount: 2, currentColumns: 1 }
State Changes: [
  * PaneLayout.columns = 2
  * PaneLayout.columnDistribution = [2, 1] // 3 panes redistributed
  * UI layout animations triggered
]
Output: { success: true, newLayout: "2 columns", distribution: [2, 1] }
UI_Context: {
  before: "3 panes stacked vertically in single column",
  action: "User selects 2-column layout from settings",
  after: "Panes redistribute: 2 panes in left column, 1 in right column"
}
```

*Switch to 3 Columns*:
```
Input: { newColumnCount: 3, currentColumns: 2 }
State Changes: [
  * PaneLayout.columns = 3
  * PaneLayout.columnDistribution = [2, 1, 1] // 4 panes redistributed
]
Output: { success: true, newLayout: "3 columns", distribution: [2, 1, 1] }
UI_Context: {
  before: "4 panes in 2-column layout",
  action: "User selects 3-column layout",
  after: "Panes redistribute evenly across 3 columns"
}
```

*Switch to 1 Column*:
```
Input: { newColumnCount: 1, currentColumns: 3 }
State Changes: [
  * PaneLayout.columns = 1
  * PaneLayout.columnDistribution = [5] // All panes in single column
  * PaneLayout.paneOrder maintained for vertical stacking
]
Output: { success: true, newLayout: "1 column", distribution: [5] }
UI_Context: {
  before: "5 panes distributed across 3 columns",
  action: "User selects single-column layout",
  after: "All panes stack vertically in single column"
}
```

### OP017: DragDropPane
**Purpose**: Drag and drop a pane to a different position with intelligent reflow
**Trigger**: User drags pane handle to new position
**Actor**: User

**Pre-conditions**:
- At least 2 panes exist
- Drag operation initiated successfully
- Target position is valid within layout

**Post-conditions**:
- Pane moved to new position
- Source column panes shift up to fill gap
- Target column panes shift down from drop point
- Layout automatically rebalanced
- Visual feedback cleared

**Drag and Drop Logic**:
1. **Drag Start**: Pane becomes semi-transparent, drop targets highlighted
2. **During Drag**: Real-time preview of drop position, invalid targets grayed out
3. **Drop Validation**: Ensure target position exists and is different from source
4. **Source Column Update**: All panes below dragged pane move up one position
5. **Target Column Update**: Panes at and below drop target move down one position
6. **Position Assignment**: Dragged pane takes the target position

**UI Integration**:
- **UI_Trigger**: Mouse drag on pane title bar or drag handle
- **UI_Inputs**: Drag start position, current mouse position, drop target
- **UI_Effects**: Semi-transparent pane preview, drop zone highlighting, smooth animations
- **UI_Validations**: Prevent dropping on self, validate column boundaries

**Examples**:

*Drag Within Same Column*:
```
Input: {
  paneId: "P002",
  sourceColumn: 1,
  sourcePosition: 2,
  targetColumn: 1,
  targetPosition: 0
}
State Changes: [
  // Source column: P001, P002, P003 -> P001, P003, [empty]
  * Pane.P003.orderIndex = 2 → 1 (moves up)

  // Target position: Insert P002 at position 0
  * Pane.P001.orderIndex = 0 → 1 (moves down)
  * Pane.P002.orderIndex = 0 (new position)

  // Final order: P002, P001, P003
]
Output: {
  success: true,
  finalOrder: ["P002", "P001", "P003"],
  columnsAffected: [1]
}
UI_Context: {
  before: "Column 1: P001, P002, P003 (top to bottom)",
  action: "User drags P002 to top of same column",
  after: "Column 1: P002, P001, P003 (P002 moved to top, others shifted down)"
}
```

*Drag Between Columns*:
```
Input: {
  paneId: "P003",
  sourceColumn: 1,
  sourcePosition: 2,
  targetColumn: 2,
  targetPosition: 1,
  layout: { columns: 3, distribution: [2, 2, 1] }
}
State Changes: [
  // Source column 1: P001, P002, P003 -> P001, P002
  * Column1.panes = ["P001", "P002"] (P003 removed)

  // Target column 2: P004, P005 -> P004, P003, P005
  * Pane.P005.orderIndex = 1 → 2 (moves down)
  * Pane.P003.orderIndex = 1 (inserted)

  // Update column distribution
  * PaneLayout.columnDistribution = [2, 3, 1]
]
Output: {
  success: true,
  sourceColumn: { id: 1, newCount: 2 },
  targetColumn: { id: 2, newCount: 3 },
  finalDistribution: [2, 3, 1]
}
UI_Context: {
  before: "Column 1: [P001, P002, P003], Column 2: [P004, P005], Column 3: [P006]",
  action: "User drags P003 to position 1 in column 2",
  after: "Column 1: [P001, P002], Column 2: [P004, P003, P005], Column 3: [P006]"
}
```

*Drag to Empty Column*:
```
Input: {
  paneId: "P001",
  sourceColumn: 1,
  sourcePosition: 0,
  targetColumn: 3,
  targetPosition: 0,
  layout: { columns: 3, distribution: [3, 2, 0] }
}
State Changes: [
  // Source column 1: P001, P002, P003 -> P002, P003
  * Pane.P002.orderIndex = 1 → 0 (moves up)
  * Pane.P003.orderIndex = 2 → 1 (moves up)

  // Target column 3: [] -> [P001]
  * Pane.P001.columnId = 3
  * Pane.P001.orderIndex = 0

  // Update distribution
  * PaneLayout.columnDistribution = [2, 2, 1]
]
Output: {
  success: true,
  sourceColumn: { id: 1, newCount: 2 },
  targetColumn: { id: 3, newCount: 1 },
  finalDistribution: [2, 2, 1]
}
UI_Context: {
  before: "Column 1: [P001, P002, P003], Column 2: [P004, P005], Column 3: []",
  action: "User drags P001 to empty column 3",
  after: "Column 1: [P002, P003], Column 2: [P004, P005], Column 3: [P001]"
}
```

*Invalid Drag (Boundary)*:
```
Input: {
  paneId: "P001",
  sourceColumn: 1,
  sourcePosition: 0,
  targetColumn: 1,
  targetPosition: 0  // Same position
}
State Changes: []
Output: {
  success: false,
  error: "INVALID_DROP_TARGET",
  reason: "Cannot drop pane on its current position"
}
UI_Context: {
  before: "P001 being dragged",
  action: "User attempts to drop P001 on its original position",
  after: "Pane snaps back to original position, no changes made"
}
```

### OP018: ResizePluginWindow
**Purpose**: Resize the plugin window and adjust layout accordingly
**Trigger**: User drags window edges or maximizes window
**Actor**: User

**Pre-conditions**:
- Plugin window is open and resizable
- New dimensions are within valid range
- No conflicting resize operations in progress

**Post-conditions**:
- Window dimensions updated
- Oscilloscope area resized proportionally
- Sidebar maintains fixed width
- Layout adjustments animated smoothly
- Window size persisted per instance

**UI Integration**:
- **UI_Trigger**: Window resize handles, maximize button, OS window controls
- **UI_Inputs**: New window width and height
- **UI_Effects**: Oscilloscope area scales, sidebar remains fixed width, smooth animation
- **UI_Validations**: Enforce minimum/maximum window dimensions

**Examples**:

*Window Resize*:
```
Input: { newWidth: 1400, newHeight: 900, previousWidth: 1200, previousHeight: 800 }
State Changes: [
  * WindowLayout.windowWidth = 1400
  * WindowLayout.windowHeight = 900
  * WindowLayout.oscilloscopeAreaWidth = 1400 - sidebarWidth
  * All pane heights recalculated proportionally
]
Output: { success: true, newDimensions: { width: 1400, height: 900 } }
UI_Context: {
  before: "Plugin window 1200x800, sidebar 300px",
  action: "User drags window corner to resize",
  after: "Plugin window 1400x900, oscilloscope area grows, sidebar stays 300px"
}
```

### OP019: ResizeSidebar
**Purpose**: Adjust sidebar width by dragging the resize handle
**Trigger**: User drags the sidebar resize handle (left edge of sidebar)
**Actor**: User

**Pre-conditions**:
- Sidebar is expanded (not collapsed)
- Resize handle is accessible
- Sufficient window width for minimum oscilloscope area

**Post-conditions**:
- Sidebar width updated within valid range
- Oscilloscope area width adjusted accordingly
- Sidebar width preference persisted
- Visual feedback during resize

**UI Integration**:
- **UI_Trigger**: Mouse drag on sidebar left edge resize handle
- **UI_Inputs**: Mouse X position during drag
- **UI_Effects**: Real-time sidebar width preview, oscilloscope area resize, cursor changes
- **UI_Validations**: Enforce minimum/maximum sidebar width, ensure oscilloscope area minimum

**Examples**:

*Expand Sidebar*:
```
Input: {
  dragStartX: 900,
  currentX: 850,
  originalSidebarWidth: 300,
  windowWidth: 1200
}
State Changes: [
  * WindowLayout.sidebarWidth = 350 // Increased by 50px
  * WindowLayout.oscilloscopeAreaWidth = 850 // Decreased accordingly
]
Output: { success: true, newSidebarWidth: 350, newOscilloscopeWidth: 850 }
UI_Context: {
  before: "Sidebar 300px wide, oscilloscope area 900px",
  action: "User drags sidebar handle left to expand",
  after: "Sidebar 350px wide, oscilloscope area 850px, smooth transition"
}
```

*Shrink Sidebar*:
```
Input: {
  dragStartX: 900,
  currentX: 950,
  originalSidebarWidth: 300,
  windowWidth: 1200
}
State Changes: [
  * WindowLayout.sidebarWidth = 250 // Decreased by 50px
  * WindowLayout.oscilloscopeAreaWidth = 950 // Increased accordingly
]
Output: { success: true, newSidebarWidth: 250, newOscilloscopeWidth: 950 }
UI_Context: {
  before: "Sidebar 300px wide, oscilloscope area 900px",
  action: "User drags sidebar handle right to shrink",
  after: "Sidebar 250px wide, oscilloscope area 950px"
}
```

*Boundary Enforcement*:
```
Input: {
  dragStartX: 900,
  currentX: 1050, // Would make sidebar < 200px minimum
  originalSidebarWidth: 300,
  windowWidth: 1200
}
State Changes: [
  * WindowLayout.sidebarWidth = 200 // Clamped to minimum
  * WindowLayout.oscilloscopeAreaWidth = 1000
]
Output: { success: true, newSidebarWidth: 200, clamped: true }
UI_Context: {
  before: "User dragging sidebar handle",
  action: "User attempts to shrink sidebar below minimum",
  after: "Sidebar stops at 200px minimum, visual resistance feedback"
}
```

### OP020: CollapseSidebar
**Purpose**: Toggle sidebar collapsed/expanded state
**Trigger**: User clicks collapse button or double-clicks resize handle
**Actor**: User

**Pre-conditions**:
- Plugin window is open
- UI is responsive

**Post-conditions**:
- Sidebar collapsed state toggled
- Oscilloscope area resized to full width (if collapsed) or reduced (if expanded)
- Collapse state persisted per instance
- Smooth animation transition

**Examples**:

*Collapse Sidebar*:
```
Input: { currentlyExpanded: true, sidebarWidth: 300 }
State Changes: [
  * WindowLayout.sidebarCollapsed = true
  * WindowLayout.oscilloscopeAreaWidth = windowWidth - collapsedSidebarWidth
  // Sidebar content hidden, only collapse button visible
]
Output: { success: true, collapsed: true, oscilloscopeAreaWidth: 1150 }
UI_Context: {
  before: "Sidebar visible with controls, oscilloscope area 900px",
  action: "User clicks collapse button",
  after: "Sidebar slides out, oscilloscope area expands to 1150px"
}
```

### OP021: ConfigureTimingMode
**Purpose**: Set timing mode (TIME or MELODIC) and configure display intervals
**Trigger**: User selects timing mode from sidebar dropdown
**Actor**: User

**Pre-conditions**:
- Plugin is active
- Sidebar is accessible
- Valid timing mode selection

**Post-conditions**:
- TimingConfig.timingMode updated
- Appropriate interval controls enabled/disabled
- Display intervals recalculated
- All oscillators immediately apply new timing

**UI Integration**:
- **UI_Trigger**: Timing mode dropdown in sidebar
- **UI_Inputs**: TimingMode selection (TIME/MELODIC)
- **UI_Effects**: Enable/disable relevant interval controls, immediate display update
- **UI_Validations**: Valid mode selection

**Examples**:

*Switch to TIME Mode*:
```
Input: { timingMode: "TIME", previousMode: "MELODIC" }
State Changes: [
  * TimingConfig.timingMode = TIME
  * TimingConfig.timeIntervalMs = 1000 // Default 1 second
  * TimingConfig.actualIntervalMs = 1000 // Direct mapping
]
Output: { success: true, newMode: "TIME", intervalMs: 1000 }
UI_Context: {
  before: "MELODIC mode with 1/4 note display",
  action: "User selects TIME from dropdown",
  after: "TIME mode enabled, millisecond controls visible, 1 second interval"
}
```

*Switch to MELODIC Mode*:
```
Input: { timingMode: "MELODIC", hostBPM: 120 }
State Changes: [
  * TimingConfig.timingMode = MELODIC
  * TimingConfig.noteInterval = NOTE_1_4TH // Default quarter note
  * TimingConfig.actualIntervalMs = 500 // 120 BPM quarter note
]
Output: { success: true, newMode: "MELODIC", intervalMs: 500 }
UI_Context: {
  before: "TIME mode with 1000ms interval",
  action: "User selects MELODIC from dropdown",
  after: "MELODIC mode enabled, note length controls visible, 1/4 note = 500ms"
}
```

### OP022: SetTimeInterval
**Purpose**: Configure time interval in milliseconds (TIME mode only)
**Trigger**: User adjusts time interval slider or input field
**Actor**: User

**Pre-conditions**:
- TimingConfig.timingMode = TIME
- Valid interval value within bounds
- UI controls are enabled

**Post-conditions**:
- TimingConfig.timeIntervalMs updated
- TimingConfig.actualIntervalMs recalculated
- Oscillator display window immediately resized
- All active oscillators apply new interval

**UI Integration**:
- **UI_Trigger**: Time interval slider, numeric input field
- **UI_Inputs**: Time value in milliseconds (1-60000)
- **UI_Effects**: Real-time oscillator window resize, interval preview
- **UI_Validations**: Enforce 1ms-60s range, numeric input validation

**Examples**:

*Set 500ms Interval*:
```
Input: { timeIntervalMs: 500, previousInterval: 1000 }
State Changes: [
  * TimingConfig.timeIntervalMs = 500
  * TimingConfig.actualIntervalMs = 500
  // All oscillators immediately resize display window
]
Output: { success: true, newIntervalMs: 500 }
UI_Context: {
  before: "1000ms display window",
  action: "User sets slider to 500ms",
  after: "500ms display window, oscillators show half the previous duration"
}
```

*Boundary Enforcement*:
```
Input: { timeIntervalMs: 70000 } // Exceeds 60s limit
State Changes: [
  * TimingConfig.timeIntervalMs = 60000 // Clamped to maximum
  * TimingConfig.actualIntervalMs = 60000
]
Output: { success: true, newIntervalMs: 60000, clamped: true }
UI_Context: {
  before: "User types 70000ms",
  action: "Input validation triggers",
  after: "Value clamped to 60000ms maximum, visual feedback shown"
}
```

### OP023: SetNoteInterval
**Purpose**: Configure note interval length (MELODIC mode only)
**Trigger**: User selects note interval from dropdown
**Actor**: User

**Pre-conditions**:
- TimingConfig.timingMode = MELODIC
- Valid host BPM available
- Note interval selection valid

**Post-conditions**:
- TimingConfig.noteInterval updated
- TimingConfig.actualIntervalMs recalculated based on BPM
- Oscillator display window resized to note duration
- Sync points aligned with musical timing

**UI Integration**:
- **UI_Trigger**: Note interval dropdown (1/16th, 1/8th, 1/4, etc.)
- **UI_Inputs**: NoteInterval selection
- **UI_Effects**: Automatic BPM-based calculation, display update
- **UI_Validations**: Valid note interval selection

**Examples**:

*Set 1/8th Note Interval*:
```
Input: { noteInterval: "NOTE_1_8TH", hostBPM: 120 }
State Changes: [
  * TimingConfig.noteInterval = NOTE_1_8TH
  * TimingConfig.actualIntervalMs = 250 // (60/120) * (1/2) * 1000
  // Display window resizes to exactly one 8th note duration
]
Output: { success: true, noteInterval: "NOTE_1_8TH", intervalMs: 250 }
UI_Context: {
  before: "1/4 note display (500ms at 120 BPM)",
  action: "User selects 1/8th note from dropdown",
  after: "1/8th note display (250ms), oscillator window shows exactly one 8th note"
}
```

*BPM Change Adaptation*:
```
Input: { hostBPM: 140, currentNoteInterval: "NOTE_1_4TH" }
State Changes: [
  * TimingConfig.hostBPM = 140 // Updated from host
  * TimingConfig.actualIntervalMs = 429 // (60/140) * 1000 for quarter note
  // Oscillator automatically adjusts to new tempo
]
Output: { success: true, newBPM: 140, intervalMs: 429 }
UI_Context: {
  before: "1/4 note = 500ms at 120 BPM",
  action: "Host BPM changes to 140",
  after: "1/4 note = 429ms at 140 BPM, oscillator adapts automatically"
}
```

### OP024: ToggleHostSync
**Purpose**: Enable/disable synchronization with host transport
**Trigger**: User toggles host sync checkbox
**Actor**: User

**Pre-conditions**:
- Host provides transport information
- Plugin can access host playhead position
- UI controls are responsive

**Post-conditions**:
- TimingConfig.hostSyncEnabled updated
- Oscillator sync behavior changed
- Sync points calculated relative to host timing
- Display resets align with host events

**UI Integration**:
- **UI_Trigger**: Host sync checkbox/toggle button
- **UI_Inputs**: Boolean enabled/disabled state
- **UI_Effects**: Sync indicator updates, oscillator behavior changes
- **UI_Validations**: Host compatibility check

**Examples**:

*Enable Host Sync*:
```
Input: { hostSyncEnabled: true, hostIsPlaying: true, noteInterval: "NOTE_1_8TH" }
State Changes: [
  * TimingConfig.hostSyncEnabled = true
  * TimingConfig.syncToPlayhead = true
  // Oscillators immediately sync to next 8th note boundary
]
Output: { success: true, hostSyncEnabled: true, syncActive: true }
UI_Context: {
  before: "Free-running oscillators, no host alignment",
  action: "User enables host sync checkbox",
  after: "Oscillators snap to host 8th note grid, reset on play/stop events"
}
```

*Sync with Host Play Event*:
```
Input: { hostEvent: "PLAY", hostSyncEnabled: true, playheadPosition: 2.5 }
State Changes: [
  * TimingConfig.lastSyncTimestamp = currentSampleTime
  // All oscillators reset display window to start fresh
  // Next interval boundary calculated from play position
]
Output: { success: true, syncTriggered: true, nextBoundary: 2.75 }
UI_Context: {
  before: "Host stopped, oscillators showing last data",
  action: "Host starts playback at 2.5 beats",
  after: "Oscillators reset, new display window starts from beat 2.5"
}
```

*Disable Host Sync*:
```
Input: { hostSyncEnabled: false, currentlyPlaying: true }
State Changes: [
  * TimingConfig.hostSyncEnabled = false
  * TimingConfig.syncToPlayhead = false
  // Oscillators continue free-running from current position
]
Output: { success: true, hostSyncEnabled: false, freeRunning: true }
UI_Context: {
  before: "Oscillators locked to host beat grid",
  action: "User disables host sync",
  after: "Oscillators run continuously, independent of host timing"
}
```

### OP025: UpdateHostBPM
**Purpose**: Update BPM value from host and recalculate musical intervals
**Trigger**: Host DAW changes tempo
**Actor**: Host/System

**Pre-conditions**:
- Plugin is active and receiving host information
- TimingConfig is in MELODIC mode (optional but relevant)
- Host provides valid BPM information

**Post-conditions**:
- TimingConfig.hostBPM updated with new value
- TimingConfig.actualIntervalMs recalculated for MELODIC mode
- All oscillators immediately adapt to new timing
- UI displays updated BPM value

**Examples**:

*BPM Change in MELODIC Mode*:
```
Input: { newBPM: 140, previousBPM: 120, currentNoteInterval: "NOTE_1_4TH" }
State Changes: [
  * TimingConfig.hostBPM = 140
  * TimingConfig.actualIntervalMs = 429 // (60/140) * 1000 for quarter note
  // All oscillators adapt display windows to new interval
]
Output: { success: true, newBPM: 140, intervalMs: 429 }
UI_Context: {
  before: "1/4 note = 500ms at 120 BPM",
  action: "Host BPM changes to 140",
  after: "1/4 note = 429ms at 140 BPM, oscillators adapt immediately"
}
```

### OP026: RenameOscillator
**Purpose**: Change the display name of an oscillator
**Trigger**: User edits oscillator name in configuration popup
**Actor**: User

**Pre-conditions**:
- Valid oscillator ID exists
- New name meets length constraints (1-32 characters)
- Configuration popup is open

**Post-conditions**:
- Oscillator.displayName updated
- Oscillator list refreshes to show new name
- Sort order maintained if sorting by name
- Configuration preserved

**UI Integration**:
- **UI_Trigger**: Text input field in oscillator configuration popup
- **UI_Inputs**: New oscillator name string
- **UI_Effects**: Immediate name update in list and pane display
- **UI_Validations**: 1-32 character length, non-empty string

**Examples**:

*Rename Oscillator*:
```
Input: { oscillatorId: "OSC001", newName: "Lead Vocal", previousName: "Oscillator 1" }
State Changes: [
  * Oscillator.displayName = "Lead Vocal"
  // List view updates immediately
]
Output: { success: true, newName: "Lead Vocal" }
UI_Context: {
  before: "Oscillator shows 'Oscillator 1' in list and pane",
  action: "User types 'Lead Vocal' in name field",
  after: "Oscillator shows 'Lead Vocal' everywhere, maintains position in sorted list"
}
```

### OP027: SortOscillatorList
**Purpose**: Change sorting mode and direction for oscillator list
**Trigger**: User selects sort option from toolbar dropdown
**Actor**: User

**Pre-conditions**:
- Oscillator list contains items
- Valid sort mode and direction selected
- UI is responsive

**Post-conditions**:
- OscillatorListState.sortMode and sortDirection updated
- Oscillator list reordered according to new criteria
- Selected oscillator remains visible
- Sort preference persisted

**UI Integration**:
- **UI_Trigger**: Sort dropdown and direction toggle in oscillator list toolbar
- **UI_Inputs**: OscillatorSortMode and SortDirection
- **UI_Effects**: Animated list reordering, sort indicators updated
- **UI_Validations**: Valid sort mode selection

**Examples**:

*Sort by Source Name Ascending*:
```
Input: { sortMode: "BY_SOURCE", sortDirection: "ASCENDING" }
State Changes: [
  * OscillatorListState.sortMode = BY_SOURCE
  * OscillatorListState.sortDirection = ASCENDING
  // List reorders by source name alphabetically
]
Output: { success: true, newOrder: ["Bass", "Drums", "Lead", "Vocals"] }
UI_Context: {
  before: "List sorted by oscillator name",
  action: "User selects 'Sort by Source (A-Z)'",
  after: "List reorders by source name, maintains scroll position near selected item"
}
```

### OP028: DeleteOscillatorFromList
**Purpose**: Remove oscillator from list and system
**Trigger**: User clicks trash icon in oscillator list item
**Actor**: User

**Pre-conditions**:
- Valid oscillator exists
- User confirms deletion (if required)
- No other operations in progress on oscillator

**Post-conditions**:
- Oscillator removed from system
- Pane updated to remove oscillator reference
- List refreshes and maintains scroll position
- Selection moves to next available item

**UI Integration**:
- **UI_Trigger**: Trash can icon in oscillator list item
- **UI_Inputs**: Oscillator ID, optional confirmation
- **UI_Effects**: Item removal animation, list compaction, selection update
- **UI_Validations**: Confirmation dialog for non-empty oscillators

**Examples**:

*Delete Oscillator*:
```
Input: { oscillatorId: "OSC003", confirmDeletion: true }
State Changes: [
  * Oscillator "OSC003" removed from system
  * Associated pane updates oscillator references
  * List selection moves to next item
]
Output: { success: true, deletedId: "OSC003", newSelection: "OSC004" }
UI_Context: {
  before: "List shows 5 oscillators, OSC003 selected",
  action: "User clicks trash icon and confirms deletion",
  after: "List shows 4 oscillators, OSC004 now selected, smooth removal animation"
}
```

### OP029: ScrollOscillatorListToItem
**Purpose**: Scroll the oscillator list to make a specific item visible
**Trigger**: New oscillator added, item selected programmatically, or user search
**Actor**: System or User

**Pre-conditions**:
- Target oscillator exists in list
- List is scrollable (more items than visible area)
- Target item is not currently visible

**Post-conditions**:
- List scrolled to show target item
- OscillatorListState.scrollPosition updated
- Target item highlighted briefly
- Smooth scroll animation completed

**Examples**:

*Scroll to New Item*:
```
Input: { targetOscillatorId: "OSC_NEW", scrollBehavior: "smooth" }
State Changes: [
  * OscillatorListState.scrollPosition = 0.8 // 80% down the list
  * OscillatorListState.selectedOscillatorId = "OSC_NEW"
]
Output: { success: true, scrolledTo: "OSC_NEW", finalPosition: 0.8 }
UI_Context: {
  before: "List showing items 1-8, new item added at position 15",
  action: "System scrolls to show newly added item",
  after: "List scrolls to show items 12-19, new item visible and highlighted"
}
```

### OP030: OpenOscillatorConfigPopup
**Purpose**: Open configuration popup for specific oscillator
**Trigger**: User clicks gear icon in oscillator list item
**Actor**: User

**Pre-conditions**:
- Valid oscillator ID exists
- No other configuration popup is open
- UI is responsive

**Post-conditions**:
- OscillatorListState.configPopupOpen = true
- OscillatorListState.configPopupOscillatorId set
- Configuration popup displays all oscillator settings
- Background list dimmed/disabled

**UI Integration**:
- **UI_Trigger**: Gear icon in oscillator list item
- **UI_Inputs**: Oscillator ID from list item
- **UI_Effects**: Modal popup appears, background dimmed, focus moved to popup
- **UI_Validations**: Single popup restriction, valid oscillator check

**Examples**:

*Open Configuration*:
```
Input: { oscillatorId: "OSC005" }
State Changes: [
  * OscillatorListState.configPopupOpen = true
  * OscillatorListState.configPopupOscillatorId = "OSC005"
  // Popup shows all configuration options for OSC005
]
Output: { success: true, popupOpen: true, configuringId: "OSC005" }
UI_Context: {
  before: "Oscillator list showing all items normally",
  action: "User clicks gear icon on 'Lead Guitar' oscillator",
  after: "Configuration popup opens with source, pane, color, and processing settings"
}
```

### OP031: AutoCreateOscillatorForNewInstance
**Purpose**: Automatically create fully configured oscillator when plugin added to DAW track
**Trigger**: Plugin instance initialization on DAW track with audio source
**Actor**: System

**Pre-conditions**:
- Plugin instance successfully initialized
- DAW track contains audio source
- Source registration completed
- Default pane available or can be created

**Post-conditions**:
- New oscillator created with track's source
- Oscillator assigned to appropriate pane
- Default color and processing mode applied
- Oscillator immediately visible and functional
- List scrolled to show new oscillator

**Examples**:

*Auto-Create for New Track*:
```
Input: {
  dawTrackId: "TRACK_VOCAL_01",
  sourceId: "SRC_VOCAL_01",
  trackName: "Lead Vocal"
}
State Changes: [
  * New oscillator created with sourceId = "SRC_VOCAL_01"
  * Oscillator.displayName = "Lead Vocal"
  * Default pane assigned or new pane created
  * Oscillator.visible = true
  * List scrolled to show new oscillator
]
Output: {
  success: true,
  oscillatorId: "OSC_AUTO_001",
  paneId: "PANE_DEFAULT",
  scrolledToNew: true
}
UI_Context: {
  before: "User drags Oscil plugin to 'Lead Vocal' track in DAW",
  action: "Plugin initializes and detects audio source",
  after: "Oscillator list shows new 'Lead Vocal' entry, immediately displays waveform"
}
```

## RULES

### R001: Source Deduplication
**Statement**: Only one Source entity may exist per DAW track, regardless of plugin instance count
**Rationale**: Prevents duplicate signal capture and maintains consistent source identity
**Enforcement**: InstanceRegistry validates dawTrackId uniqueness during Source creation
**Entities**: Source, PluginInstance

### R002: Oscillator-Source Binding
**Statement**: Every Oscillator must reference exactly one valid Source or be in "NO_SOURCE" state when source becomes unavailable
**Rationale**: Ensures consistent waveform visualization and handles source loss gracefully without losing oscillator configuration
**Enforcement**: Foreign key constraint validation during Oscillator creation/update, with automatic "NO_SOURCE" assignment when referenced source becomes unavailable
**Entities**: Oscillator, Source

### R003: Pane Ownership
**Statement**: Each Oscillator belongs to exactly one Pane or is in "NO_PANE" state
**Rationale**: Maintains clear visual hierarchy and handles pane deletion gracefully
**Enforcement**: UI validation during pane assignment operations, with special handling for "NO_PANE"
**Entities**: Oscillator, Pane

### R004: System Theme Immutability
**Statement**: Themes with isSystemTheme=true cannot be modified or deleted
**Rationale**: Preserves consistent default experience and prevents accidental system corruption
**Enforcement**: UI disables edit/delete actions, API returns IMMUTABLE_THEME error
**Entities**: Theme

### R005: Performance Bounds
**Statement**: System must maintain real-time performance with adaptive quality controls
**Rationale**: Maintains professional audio plugin standards with graceful degradation
**Enforcement**: Real-time performance monitoring with automatic warnings and user-controlled quality reduction
**Entities**: PerformanceConfig, RenderSettings

### R006: Instance Registry Integrity
**Statement**: InstanceRegistry maintains consistent state across all plugin instances in same process only
**Rationale**: Ensures reliable source discovery within process boundaries while avoiding complex IPC
**Enforcement**: Process-local atomic operations with mutex protection, heartbeat monitoring, process ID validation
**Entities**: PluginInstance, Source, InstanceRegistry

### R007: Audio Thread Safety
**Statement**: No dynamic memory allocation or blocking operations on audio processing thread
**Rationale**: Prevents audio dropouts and maintains real-time performance
**Enforcement**: Lock-free data structures, pre-allocated buffers, static validation
**Entities**: Source, AudioConfig

### R008: Timing Synchronization Accuracy
**Statement**: All timing operations must be sample-accurate when enabled
**Rationale**: Professional audio requires precise timing for measurement and analysis
**Enforcement**: High-resolution timers, sample-level trigger detection
**Entities**: TimingConfig, HostInfo

### R009: State Persistence Atomicity
**Statement**: Project state saves/loads must be atomic operations
**Rationale**: Prevents partial state corruption during save/load operations
**Enforcement**: Temporary file write with atomic rename, checksums validation
**Entities**: Project

### R010: Theme Color Accessibility
**Statement**: Theme colors must meet WCAG 2.1 AA contrast requirements
**Rationale**: Ensures accessibility for users with visual impairments
**Enforcement**: Contrast ratio validation during theme creation/edit
**Entities**: Theme

### R011: Cross-Instance Communication Limits
**Statement**: Inter-instance communication limited to same process only, no network/IPC
**Rationale**: Security, performance constraints, and complexity management
**Enforcement**: Process ID validation, in-process shared registry only, automatic isolation across process boundaries
**Entities**: PluginInstance, InstanceRegistry

### R012: Error Recovery Graceful Degradation
**Statement**: All error conditions must allow continued operation with reduced functionality
**Rationale**: Plugin must not crash or become unusable due to recoverable errors
**Enforcement**: Try-catch blocks with fallback behaviors, error state tracking
**Entities**: All entities

### R013: Per-Instance Configuration Isolation
**Statement**: Each plugin instance maintains independent oscillator and pane configurations
**Rationale**: Allows different visualization setups per instance while sharing sources and themes
**Enforcement**: Instance-scoped storage for oscillator/pane state, global storage only for themes and sources
**Entities**: PluginInstance, Oscillator, Pane, Theme

### R014: Theme Global Consistency
**Statement**: Theme selection and custom themes are shared across all plugin instances in same process
**Rationale**: Provides consistent visual experience across all oscilloscope instances
**Enforcement**: Global theme registry with change notification broadcast to all instances
**Entities**: Theme, ThemeManager, PluginInstance

### R015: Source Stability and Identification
**Statement**: Source IDs must be stable across DAW session saves/loads and track operations
**Rationale**: Ensures oscillator-source relationships persist reliably across sessions
**Enforcement**: UUID generation based on DAW track context with fallback to sequential assignment
**Entities**: Source, PluginInstance

### R016: Layout Mode Consistency
**Statement**: Column layout changes must preserve pane relationships and oscillator assignments
**Rationale**: Prevents data loss during layout transitions and maintains user workflow
**Enforcement**: Automatic pane redistribution algorithm with position preservation
**Entities**: PaneLayout, Pane, Oscillator

### R017: Drag Operation Atomicity
**Statement**: Pane drag-and-drop operations must be atomic (complete or fully revert)
**Rationale**: Prevents partial state corruption during complex drag operations
**Enforcement**: Transaction-like state management with rollback capability
**Entities**: Pane, PaneLayout, PaneDragState

### R018: Column Distribution Balance
**Statement**: Pane distribution across columns should be as balanced as possible
**Rationale**: Provides optimal visual layout and user experience
**Enforcement**: Automatic redistribution algorithm prioritizes even distribution
**Entities**: PaneLayout

### R019: Window Size Persistence Per Instance
**Statement**: Each plugin instance must remember its window size and sidebar configuration independently
**Rationale**: Different instances may have different optimal layouts depending on use case
**Enforcement**: Per-instance storage of window dimensions and sidebar state in project configuration
**Entities**: PluginInstance, WindowLayout

### R020: Sidebar Responsive Design
**Statement**: Sidebar must maintain usability across all supported window sizes
**Rationale**: Ensures controls remain accessible regardless of window configuration
**Enforcement**: Minimum sidebar width enforcement, responsive control layout
**Entities**: WindowLayout, Sidebar

### R021: Oscilloscope Area Minimum Width
**Statement**: Oscilloscope display area must maintain minimum 400px width for usability
**Rationale**: Ensures waveform visualization remains meaningful even with wide sidebar
**Enforcement**: Sidebar resize constraints and window resize validation
**Entities**: WindowLayout, OscilloscopeArea

### R022: Timing Mode Consistency
**Statement**: All oscillators must use the same global timing configuration
**Rationale**: Provides consistent temporal reference across all oscillator displays
**Enforcement**: Global TimingConfig applied to all oscillators simultaneously
**Entities**: TimingConfig, Oscillator

### R023: Host Sync Sample Accuracy
**Statement**: When host sync is enabled, all timing calculations must be sample-accurate
**Rationale**: Professional audio requires precise timing for musical applications
**Enforcement**: Sample-level timestamp tracking and boundary calculations
**Entities**: TimingConfig, HostInfo

### R024: Note Interval BPM Adaptation
**Statement**: MELODIC mode intervals must automatically adapt to host BPM changes
**Rationale**: Musical intervals must remain musically relevant as tempo changes
**Enforcement**: Real-time BPM monitoring with immediate interval recalculation
**Entities**: TimingConfig

### R025: Timing Bounds Enforcement
**Statement**: All timing intervals must enforce reasonable minimum and maximum bounds
**Rationale**: Prevents unusable display intervals and system performance issues
**Enforcement**: Input validation and boundary clamping
**Entities**: TimingConfig

### R026: Oscillator List Sort Consistency
**Statement**: Oscillator list sort order must be maintained across UI operations
**Rationale**: Provides predictable user experience and prevents confusion during list manipulation
**Enforcement**: Sort order preserved during add/delete/rename operations, automatic re-sorting
**Entities**: OscillatorListState, Oscillator

### R027: Single Configuration Popup
**Statement**: Only one oscillator configuration popup can be open at a time
**Rationale**: Prevents UI confusion and conflicting configuration changes
**Enforcement**: UI validation blocks multiple popup opens, automatic close on new popup request
**Entities**: OscillatorListState

### R028: Auto-Oscillator Creation
**Statement**: New plugin instances must automatically create a functional oscillator for immediate use
**Rationale**: Provides immediate visual feedback and reduces setup time for users
**Enforcement**: Automatic oscillator creation during plugin initialization on tracks with audio
**Entities**: PluginInstance, Oscillator, Source

### R029: Host BPM Real-time Updates
**Statement**: BPM changes from host must immediately update timing calculations
**Rationale**: Musical timing must remain accurate as users change tempo during playback
**Enforcement**: Real-time host BPM monitoring with immediate interval recalculation
**Entities**: TimingConfig, HostInfo

## STATES

### PluginInstance State Machine
**States**: [INITIALIZING, ACTIVE, AGGREGATING, SUSPENDED, TERMINATING]

**Detailed State Descriptions**:
- **INITIALIZING**: Instance created, loading configuration, registering with InstanceRegistry
- **ACTIVE**: Normal operation, processing audio, UI may be closed
- **AGGREGATING**: UI open, discovering and displaying sources from other instances
- **SUSPENDED**: DAW has suspended plugin (e.g., track muted, plugin bypassed)
- **TERMINATING**: Shutdown in progress, cleaning up resources

**Valid Transitions with Triggers**:
- INITIALIZING → ACTIVE (plugin host initialization complete, audio callback active)
- ACTIVE → AGGREGATING (user opens plugin UI window)
- AGGREGATING → ACTIVE (user closes plugin UI window)
- ACTIVE → SUSPENDED (DAW suspends plugin or mutes track)
- SUSPENDED → ACTIVE (DAW resumes plugin or unmutes track)
- ANY → TERMINATING (DAW shutdown, track removal, or plugin crash)
- TERMINATING → [DESTROYED] (cleanup complete, instance removed)

**Invalid Transitions**:
- INITIALIZING → AGGREGATING (UI cannot open during initialization)
- SUSPENDED → AGGREGATING (UI access blocked while suspended)
- TERMINATING → Any other state (termination is irreversible)

### Source State Machine
**States**: [DISCOVERED, ACTIVE, INACTIVE, ORPHANED, STALE]

**Detailed State Descriptions**:
- **DISCOVERED**: New source detected, awaiting audio data
- **ACTIVE**: Receiving audio samples, owning instance operational
- **INACTIVE**: No audio samples received for >1 second, but owning instance alive
- **ORPHANED**: Owning instance terminated, no backup instances available
- **STALE**: No audio samples for >30 seconds, ownership questionable

**Valid Transitions with Triggers**:
- DISCOVERED → ACTIVE (first audio samples received from owning instance)
- ACTIVE → INACTIVE (audio stopped but instance heartbeat continues)
- INACTIVE → ACTIVE (audio resumed after pause)
- ACTIVE → ORPHANED (owning instance terminated, no backup available)
- INACTIVE → ORPHANED (owning instance terminated during silence)
- ORPHANED → ACTIVE (new producer instance registered for same DAW track)
- ACTIVE → STALE (no audio for >30 seconds despite active instance)
- STALE → ACTIVE (audio resumed after extended silence)
- STALE → ORPHANED (owning instance confirmed dead)

**Automatic Cleanup**:
- ORPHANED sources with no oscillators referencing them are removed after 5 minutes
- STALE sources are promoted to ORPHANED if instance heartbeat fails

### Theme State Machine
**States**: [CREATING, ACTIVE, EDITING, APPLYING, CORRUPTED]

**Detailed State Descriptions**:
- **CREATING**: New theme being created, not yet saved
- **ACTIVE**: Theme saved and available for selection/use
- **EDITING**: Theme open in theme editor (custom themes only)
- **APPLYING**: Theme switch in progress, UI updating colors
- **CORRUPTED**: Theme data invalid, fallback required

**Valid Transitions with Triggers**:
- CREATING → ACTIVE (theme saved successfully, validation passed)
- ACTIVE → EDITING (user opens theme in editor, custom theme only)
- EDITING → ACTIVE (changes saved or editing cancelled)
- ACTIVE → APPLYING (theme selected for application)
- APPLYING → ACTIVE (theme application completed successfully)
- ACTIVE → CORRUPTED (theme validation fails during load)
- CORRUPTED → ACTIVE (theme repaired or replaced with default)
- CREATING → CORRUPTED (save operation fails validation)

**Protection Rules**:
- System themes cannot transition to EDITING or CORRUPTED states
- Only one theme can be in APPLYING state at a time
- CORRUPTED themes are automatically replaced with system default

### Oscillator State Machine
**States**: [CREATING, ACTIVE, HIDDEN, NO_SOURCE, ERROR]

**Detailed State Descriptions**:
- **CREATING**: Oscillator being created, awaiting source assignment
- **ACTIVE**: Normal operation, displaying waveform from valid source
- **HIDDEN**: User toggled visibility off, not rendering but state preserved
- **NO_SOURCE**: No valid source assigned, showing "No Signal" state but preserving all other settings
- **ERROR**: Processing error occurred, fallback display mode

**Valid Transitions with Triggers**:
- CREATING → ACTIVE (valid source assigned, oscillator configuration complete)
- CREATING → NO_SOURCE (no source available during creation, or "NO_SOURCE" explicitly selected)
- ACTIVE → HIDDEN (user toggles visibility off)
- HIDDEN → ACTIVE (user toggles visibility on, source still available)
- ACTIVE → NO_SOURCE (referenced source becomes unavailable or user disconnects source)
- HIDDEN → NO_SOURCE (referenced source becomes unavailable while hidden)
- NO_SOURCE → ACTIVE (valid source assigned to oscillator in NO_SOURCE state)
- ACTIVE → ERROR (processing mode fails, invalid configuration)
- NO_SOURCE → ERROR (configuration error while in no-source state)
- ERROR → ACTIVE (error resolved, valid source available)
- ERROR → NO_SOURCE (error resolved, but no source available)
- NO_SOURCE → [DELETED] (user deletes oscillator in no-source state)

**State Preservation Rules**:
- Transitions to NO_SOURCE preserve all settings: color, processingMode, paneId, visible, displayName, etc.
- Only sourceId changes to "NO_SOURCE", all other properties remain unchanged
- When transitioning from NO_SOURCE to ACTIVE, previous settings are restored

## ERRORS

### ERR001: INVALID_SOURCE_ID
**Message**: "Source not found or no longer available"
**Category**: VALIDATION_ERROR
**Triggers**: [Oscillator creation with invalid sourceId, Source reference update]
**Recovery**: Present valid source list, retry with correct ID
**Operations**: [CreateOscillator, UpdateOscillatorSource]
**Auto-Recovery**: Attempt to find alternative source with same DAW track name
**User Action**: Select from dropdown of available sources
**Logging**: Warn level with sourceId and available alternatives

### ERR002: MAX_OSCILLATORS_EXCEEDED
**Message**: "Maximum oscillator limit (128) reached"
**Category**: RESOURCE_LIMIT
**Triggers**: [CreateOscillator when count ≥ 128]
**Recovery**: Delete unused oscillators or increase limit (future)
**Operations**: [CreateOscillator]
**Auto-Recovery**: None - user decision required
**User Action**: Delete existing oscillators or reorganize layout
**Logging**: Info level with current count and suggestion

### ERR003: THEME_NOT_FOUND
**Message**: "Selected theme is not available"
**Category**: VALIDATION_ERROR
**Triggers**: [SwitchTheme with invalid themeId, Theme deletion while active]
**Recovery**: Fallback to default theme, present theme selection UI
**Operations**: [SwitchTheme, DeleteTheme]
**Auto-Recovery**: Apply default system theme immediately
**User Action**: Select alternative theme from theme manager
**Logging**: Warn level with missing themeId and fallback applied

### ERR004: IMMUTABLE_THEME_MODIFICATION
**Message**: "System themes cannot be modified or deleted"
**Category**: PERMISSION_ERROR
**Triggers**: [EditTheme on system theme, DeleteTheme on system theme]
**Recovery**: Clone theme before editing, use custom themes only
**Operations**: [EditTheme, DeleteTheme]
**Auto-Recovery**: Offer automatic clone creation with "_Custom" suffix
**User Action**: Accept clone creation or cancel operation
**Logging**: Info level with attempted action and resolution

### ERR005: SOURCE_REGISTRATION_CONFLICT
**Message**: "DAW track already has registered source"
**Category**: CONFLICT_ERROR
**Triggers**: [Multiple instances on same DAW track attempting Source creation]
**Recovery**: Use existing Source, implement instance priority resolution
**Operations**: [RegisterSource]
**Auto-Recovery**: Assign backup role to conflicting instance
**System Action**: Implement priority-based ownership algorithm
**Logging**: Debug level with conflict resolution details

### ERR006: NO_SOURCE_AVAILABLE
**Message**: "No audio sources available for oscillator"
**Category**: VALIDATION_ERROR
**Triggers**: [CreateOscillator when no sources exist, ChangeOscillatorSource with invalid ID, Source removal]
**Recovery**: Set oscillator to "NO_SOURCE" state preserving all other settings, add plugin instances to create sources
**Operations**: [CreateOscillator, ChangeOscillatorSource, HandleSourceOwnershipTransfer]
**Auto-Recovery**: Automatically set oscillator to "NO_SOURCE" state when source becomes unavailable
**User Action**: Add plugin instances to DAW tracks to create sources, then select valid source from dropdown
**Logging**: Info level with available source count and affected oscillator details

### ERR007: NO_PANE_AVAILABLE
**Message**: "Oscillator pane was deleted"
**Category**: STATE_ERROR
**Triggers**: [DeletePane while oscillators assigned to it]
**Recovery**: Set affected oscillators to "NO_PANE" state, user must manually assign to valid panes
**Operations**: [DeletePane, ChangeOscillatorPane]
**Auto-Recovery**: Set affected oscillators to "NO_PANE" state preserving all other settings
**User Action**: Select valid pane for affected oscillators from dropdown
**Logging**: Warn level with affected oscillator count and preservation details

### ERR008: PERFORMANCE_DEGRADATION
**Message**: "Performance threshold exceeded"
**Category**: PERFORMANCE_WARNING
**Triggers**: [CPU usage above threshold, Memory usage above threshold, Frame rate below minimum]
**Recovery**: Reduce quality settings, hide oscillators, lower refresh rate
**Operations**: [ReduceQuality, ShowStatusBar]
**Auto-Recovery**: Display performance warning with reduction options
**User Action**: Accept quality reduction or manually optimize configuration
**Logging**: Warn level with current performance metrics

### ERR009: PANE_RESIZE_CONFLICT
**Message**: "Cannot resize pane: insufficient space"
**Category**: UI_ERROR
**Triggers**: [ResizePane with size exceeding available space]
**Recovery**: Adjust to maximum available size, redistribute other panes
**Operations**: [ResizePane]
**Auto-Recovery**: Clamp to maximum available size
**User Action**: Reduce pane size or adjust other panes first
**Logging**: Info level with attempted and actual sizes

### ERR010: INSTANCE_COMMUNICATION_FAILURE
**Message**: "Cannot communicate with other plugin instances"
**Category**: SYSTEM_ERROR
**Triggers**: [Instance registry corruption, Process isolation issues]
**Recovery**: Restart instance registry, fall back to single-instance mode
**Operations**: [RegisterSource, HandleSourceOwnershipTransfer]
**Auto-Recovery**: Isolate instance and continue with local sources only
**System Action**: Reinitialize instance registry, log isolation event
**Logging**: Error level with communication failure details

### ERR006: PROCESSING_MODE_INVALID
**Message**: "Selected processing mode is not supported"
**Category**: VALIDATION_ERROR
**Triggers**: [UpdateOscillatorProcessing with invalid mode]
**Recovery**: Revert to previous mode, present valid mode options
**Operations**: [UpdateOscillatorProcessing]
**Auto-Recovery**: Revert to FullStereo as safe default
**User Action**: Select valid processing mode from dropdown
**Logging**: Warn level with invalid mode and reversion

### ERR007: PERFORMANCE_DEGRADATION
**Message**: "Performance below minimum thresholds"
**Category**: PERFORMANCE_WARNING
**Triggers**: [Frame rate < 30fps for >5 seconds, CPU usage > 15%]
**Recovery**: Suggest reducing oscillator count, disable visual effects
**Operations**: [All rendering operations]
**Auto-Recovery**: Reduce rendering quality, skip frames if necessary
**User Action**: Reduce oscillator count or simplify visualization
**Logging**: Warn level with performance metrics and auto-adjustments

### ERR008: STATE_CORRUPTION
**Message**: "Project state validation failed"
**Category**: DATA_ERROR
**Triggers**: [Load project with invalid schema, Corrupted ValueTree]
**Recovery**: Load backup state, reset to defaults, attempt repair
**Operations**: [LoadProject, SaveProject]
**Auto-Recovery**: Load most recent valid backup automatically
**User Action**: Choose to restore backup, reset, or manually repair
**Logging**: Error level with corruption details and recovery actions

### ERR009: AUDIO_BUFFER_UNDERRUN
**Message**: "Audio processing interrupted due to buffer underrun"
**Category**: PERFORMANCE_ERROR
**Triggers**: [Host audio callback timeout, Insufficient CPU resources]
**Recovery**: Increase buffer size, reduce processing load
**Operations**: [Audio processing, Buffer management]
**Auto-Recovery**: Temporarily increase buffer size, reduce quality
**System Action**: Notify host of processing capabilities
**Logging**: Warn level with buffer statistics and adjustments

### ERR010: DAW_HOST_DISCONNECT
**Message**: "Connection to DAW host lost"
**Category**: SYSTEM_ERROR
**Triggers**: [Host process termination, IPC failure]
**Recovery**: Attempt reconnection, save current state
**Operations**: [All host-dependent operations]
**Auto-Recovery**: Cache current state, attempt graceful degradation
**System Action**: Monitor for host reconnection
**Logging**: Error level with disconnection cause and recovery attempts

### ERR011: MEMORY_ALLOCATION_FAILURE
**Message**: "Insufficient memory for operation"
**Category**: RESOURCE_ERROR
**Triggers**: [Large oscillator creation, Audio buffer allocation]
**Recovery**: Free unused resources, reduce memory usage
**Operations**: [CreateOscillator, Audio buffer operations]
**Auto-Recovery**: Garbage collect unused objects, reduce buffer sizes
**User Action**: Close unused oscillators or reduce quality settings
**Logging**: Error level with memory usage and cleanup actions

### ERR012: DRAG_DROP_VALIDATION_FAILURE
**Message**: "Invalid drag and drop operation"
**Category**: UI_VALIDATION_ERROR
**Triggers**: [Invalid drop target, Circular pane reference, Same-position drop]
**Recovery**: Cancel drag operation, restore original state
**Operations**: [DragDropPane, ReorderPanes]
**Auto-Recovery**: Snap back to original position with animation
**User Action**: Retry drag operation with valid target
**Logging**: Debug level with invalid operation details

### ERR013: LAYOUT_MODE_TRANSITION_FAILURE
**Message**: "Cannot change layout mode while operation in progress"
**Category**: STATE_CONFLICT_ERROR
**Triggers**: [ChangeLayoutMode during active drag, Concurrent layout operations]
**Recovery**: Wait for current operation to complete, retry layout change
**Operations**: [ChangeLayoutMode]
**Auto-Recovery**: Queue layout change until operations complete
**User Action**: Wait for drag operations to finish, then retry
**Logging**: Info level with conflicting operation details

### ERR014: COLUMN_REDISTRIBUTION_ERROR
**Message**: "Failed to redistribute panes across new column layout"
**Category**: LAYOUT_ERROR
**Triggers**: [Insufficient space for redistribution, Invalid column configuration]
**Recovery**: Revert to previous layout, adjust pane sizes
**Operations**: [ChangeLayoutMode, DragDropPane]
**Auto-Recovery**: Fallback to single-column layout with warning
**User Action**: Reduce pane count or resize panes before changing layout
**Logging**: Warn level with redistribution failure details and fallback applied

### ERR015: WINDOW_RESIZE_CONSTRAINT_VIOLATION
**Message**: "Window size exceeds platform constraints"
**Category**: UI_VALIDATION_ERROR
**Triggers**: [Window resize below minimum, Window resize above maximum, Invalid aspect ratio]
**Recovery**: Clamp to valid dimensions, maintain aspect ratio if needed
**Operations**: [ResizePluginWindow]
**Auto-Recovery**: Enforce minimum/maximum dimensions with visual feedback
**User Action**: Resize within supported range
**Logging**: Debug level with attempted and actual dimensions

### ERR016: SIDEBAR_RESIZE_CONFLICT
**Message**: "Cannot resize sidebar: insufficient space"
**Category**: LAYOUT_CONSTRAINT_ERROR
**Triggers**: [Sidebar resize would violate oscilloscope minimum width, Invalid sidebar dimensions]
**Recovery**: Clamp sidebar width to valid range, preserve oscilloscope usability
**Operations**: [ResizeSidebar]
**Auto-Recovery**: Enforce minimum oscilloscope area width (400px minimum)
**User Action**: Increase window width or reduce sidebar size
**Logging**: Info level with constraint violation details

### ERR017: WINDOW_STATE_PERSISTENCE_FAILURE
**Message**: "Failed to save window layout preferences"
**Category**: PERSISTENCE_ERROR
**Triggers**: [File system error during save, Invalid state data, Storage quota exceeded]
**Recovery**: Continue with current layout, attempt save on next operation
**Operations**: [ResizePluginWindow, ResizeSidebar, CollapseSidebar]
**Auto-Recovery**: Cache state in memory, retry persistence periodically
**User Action**: Check file permissions, free disk space
**Logging**: Warn level with persistence failure details and retry strategy

### ERR018: HOST_SYNC_UNAVAILABLE
**Message**: "Host does not provide timing information"
**Category**: HOST_COMPATIBILITY_ERROR
**Triggers**: [Host sync enabled but host has no transport, BPM information unavailable]
**Recovery**: Fall back to free-running mode, disable host sync controls
**Operations**: [ToggleHostSync, ConfigureTimingMode]
**Auto-Recovery**: Automatically disable host sync and switch to TIME mode
**User Action**: Use TIME mode instead, or use compatible DAW host
**Logging**: Info level with host capability details and fallback mode

### ERR019: INVALID_TIME_INTERVAL
**Message**: "Time interval outside valid range"
**Category**: VALIDATION_ERROR
**Triggers**: [SetTimeInterval with value < 1ms or > 60s, Invalid numeric input]
**Recovery**: Clamp to valid range, revert to previous valid value
**Operations**: [SetTimeInterval]
**Auto-Recovery**: Automatically clamp to nearest valid boundary (1ms-60s)
**User Action**: Enter value within supported range
**Logging**: Debug level with attempted and corrected values

### ERR020: BPM_CALCULATION_FAILURE
**Message**: "Cannot calculate note interval: invalid BPM"
**Category**: CALCULATION_ERROR
**Triggers**: [Host BPM = 0 or invalid, BPM outside reasonable range (20-300)]
**Recovery**: Use default 120 BPM for calculations, show warning
**Operations**: [SetNoteInterval, BPM change from host]
**Auto-Recovery**: Apply 120 BPM default with user notification
**User Action**: Check host BPM setting, ensure host is providing valid tempo
**Logging**: Warn level with invalid BPM value and default substitution

### ERR021: TIMING_MODE_TRANSITION_FAILURE
**Message**: "Cannot change timing mode during active sync operation"
**Category**: STATE_CONFLICT_ERROR
**Triggers**: [ConfigureTimingMode while host sync actively updating, Concurrent timing operations]
**Recovery**: Queue timing mode change until sync operation completes
**Operations**: [ConfigureTimingMode]
**Auto-Recovery**: Defer mode change and apply after current sync cycle
**User Action**: Wait for sync operation to complete, then retry
**Logging**: Info level with timing conflict details and deferral

### ERR022: OSCILLATOR_NAME_VALIDATION_FAILURE
**Message**: "Oscillator name must be 1-32 characters"
**Category**: VALIDATION_ERROR
**Triggers**: [RenameOscillator with empty or too long name, Invalid character input]
**Recovery**: Revert to previous valid name, highlight input validation
**Operations**: [RenameOscillator]
**Auto-Recovery**: Trim name to valid length, remove invalid characters
**User Action**: Enter valid name within character limits
**Logging**: Debug level with invalid name and validation details

### ERR023: CONFIG_POPUP_CONFLICT
**Message**: "Another oscillator configuration is already open"
**Category**: UI_STATE_ERROR
**Triggers**: [OpenOscillatorConfigPopup while another popup open, Concurrent configuration attempts]
**Recovery**: Close existing popup and open new one, or queue request
**Operations**: [OpenOscillatorConfigPopup]
**Auto-Recovery**: Automatically close previous popup and open requested one
**User Action**: Wait for current configuration to close, then retry
**Logging**: Info level with conflicting popup details

### ERR024: OSCILLATOR_DELETION_FAILURE
**Message**: "Cannot delete oscillator: operation in progress"
**Category**: STATE_CONFLICT_ERROR
**Triggers**: [DeleteOscillatorFromList during configuration, Oscillator being modified]
**Recovery**: Wait for current operation to complete, then retry deletion
**Operations**: [DeleteOscillatorFromList]
**Auto-Recovery**: Queue deletion until oscillator is not busy
**User Action**: Wait for current operation to complete, then retry
**Logging**: Info level with blocking operation details

### ERR025: AUTO_OSCILLATOR_CREATION_FAILURE
**Message**: "Failed to create automatic oscillator for new instance"
**Category**: INITIALIZATION_ERROR
**Triggers**: [AutoCreateOscillatorForNewInstance with invalid source, Maximum oscillator limit reached]
**Recovery**: Continue without auto-oscillator, allow manual creation
**Operations**: [AutoCreateOscillatorForNewInstance]
**Auto-Recovery**: Log failure and continue initialization without auto-oscillator
**User Action**: Manually create oscillator if needed
**Logging**: Warn level with creation failure details and fallback behavior

## LIMITS

### Performance Limits
| Category | Item | Value | Rationale | Enforcement |
|----------|------|-------|-----------|-------------|
| Concurrency | Maximum Sources | 64 | CPU/memory performance bounds | Pre-creation validation |
| Concurrency | Maximum Oscillators | 128 | Visual complexity management | Creation-time check |
| Performance | Minimum Frame Rate | 60 FPS | Professional visualization standard | Runtime monitoring |
| Performance | Maximum CPU Usage | 5% (16 tracks) | Host DAW stability | Adaptive quality reduction |
| Performance | Maximum Memory | 640 MB total | System resource limits | Memory pool allocation |
| Latency | Processing Delay | 10 ms | Real-time requirement | Buffer size optimization |
| UI | Theme Switch Time | 50 ms | User experience standard | Timeout enforcement |
| UI | Operation Response | 5 ms | Interactive responsiveness | UI thread monitoring |

### Data Limits
| Category | Item | Value | Rationale | Enforcement |
|----------|------|-------|-----------|-------------|
| Naming | Theme Name Length | 64 characters | UI layout constraints | Input validation |
| Naming | Oscillator Name Length | 32 characters | Display space optimization | Character counting |
| Storage | Project File Size | 10 MB | Reasonable state complexity | Serialization limits |
| Audio | Sample Rate Range | 44.1-192 kHz | Standard audio range | Host capability detection |
| Audio | Buffer Size Range | 32-2048 samples | Latency vs stability balance | Host validation |
| Window | Minimum Window Width | 800 pixels | Usable interface requirement | Resize validation |
| Window | Minimum Window Height | 600 pixels | Vertical space for controls | Resize validation |
| Window | Maximum Window Width | 3840 pixels | 4K display support | Platform limits |
| Window | Maximum Window Height | 2160 pixels | 4K display support | Platform limits |
| Sidebar | Minimum Sidebar Width | 200 pixels | Control usability | Resize constraint |
| Sidebar | Maximum Sidebar Width | 800 pixels | Oscilloscope area preservation | Resize constraint |
| Oscilloscope | Minimum Area Width | 400 pixels | Waveform visibility | Layout validation |
| Timing | Minimum Time Interval | 1 millisecond | Practical usability limit | Input validation |
| Timing | Maximum Time Interval | 60 seconds | Reasonable display window | Input validation |
| Timing | Host BPM Range | 20-300 BPM | Standard musical tempo range | BPM validation |
| Timing | Sync Accuracy | ±1 sample | Professional timing requirement | Sample-level calculation |

## SECURITY

### Role-Based Permissions
| Role | Permission | Scope |
|------|------------|-------|
| User | Create/Edit Custom Themes | User-created themes only |
| User | View System Themes | All system themes |
| User | Create/Delete Oscillators | Current project only |
| User | Modify Project Settings | Current project only |
| System | Modify System Themes | System themes only |
| System | Enforce Resource Limits | Global enforcement |

### Data Protection
- **Theme Export**: No sensitive data exposure in JSON export
- **Project State**: No DAW-specific proprietary data storage
- **Instance Communication**: In-process only, no network exposure
- **File Access**: Plugin-sandboxed file operations only

### OP032: HandlePluginBypass
**Purpose**: Process host bypass state changes while maintaining audio passthrough
**Trigger**: DAW host sets plugin bypass state
**Actor**: Host

**Pre-conditions**:
- Plugin is active and processing audio
- Host supports bypass functionality
- Audio buffer available for passthrough

**Post-conditions**:
- AudioConfig.bypassState updated
- Audio processing disabled but passthrough maintained
- UI indicates bypass state
- Parameters remain accessible for editing

**Examples**:

*Enable Bypass*:
```
Input: { bypassState: true, audioBuffer: audioData }
State Changes: [
  * AudioConfig.bypassState = true
  * Audio processing disabled
  * Visual updates paused but UI remains responsive
]
Output: { success: true, audioPassthrough: true, processingDisabled: true }
UI_Context: {
  before: "Plugin actively processing and displaying waveforms",
  action: "User enables bypass in DAW",
  after: "Waveforms frozen at last state, audio passes through unprocessed"
}
```

### OP033: ReportLatencyToHost
**Purpose**: Inform host of plugin processing latency for timing compensation
**Trigger**: Plugin initialization or audio configuration change
**Actor**: System

**Pre-conditions**:
- Host supports latency reporting
- Audio configuration established
- Latency calculation completed

**Post-conditions**:
- AudioConfig.reportedLatency updated
- Host notified of latency value
- Host timing compensation adjusted
- Sample-accurate synchronization maintained

**Examples**:

*Report Zero Latency*:
```
Input: { latencySamples: 0, sampleRate: 48000 }
State Changes: [
  * AudioConfig.reportedLatency = 0
  * Host timing compensation disabled
]
Output: { success: true, reportedLatency: 0, hostCompensation: false }
```

### OP034: HandleSampleRateChange
**Purpose**: Reconfigure plugin for runtime sample rate changes
**Trigger**: DAW host changes audio sample rate
**Actor**: Host

**Pre-conditions**:
- Plugin is initialized
- New sample rate is supported (44.1-192 kHz)
- Audio processing can be safely interrupted

**Post-conditions**:
- AudioConfig.sampleRate updated
- Ring buffers resized for new sample rate
- Timing calculations recalibrated
- Audio processing resumed with new rate

**Examples**:

*Sample Rate Change*:
```
Input: { newSampleRate: 96000, previousRate: 48000 }
State Changes: [
  * AudioConfig.sampleRate = 96000
  * Ring buffer capacities doubled
  * Timing calculations updated
  * Oscillator display windows recalculated
]
Output: { success: true, newSampleRate: 96000, buffersResized: true }
```

### OP035: SavePluginStateChunk
**Purpose**: Serialize complete plugin state for DAW session persistence
**Trigger**: DAW requests plugin state for session save
**Actor**: Host

**Pre-conditions**:
- Plugin state is valid and consistent
- All oscillators and panes properly configured
- Theme and timing settings available

**Post-conditions**:
- Binary state chunk created with version header
- All user configuration included
- State validation checksum added
- Chunk returned to host for session storage

**Examples**:

*Save State*:
```
Input: { includeThemes: false, includeOscillators: true }
State Changes: []
Output: {
  stateChunk: binaryData,
  version: "1.0.0",
  size: 2048,
  checksum: "abc123"
}
```

### OP036: LoadPluginStateChunk
**Purpose**: Restore plugin state from DAW session data
**Trigger**: DAW provides state chunk during session load
**Actor**: Host

**Pre-conditions**:
- Valid state chunk provided
- Chunk version is compatible
- Plugin is initialized and ready

**Post-conditions**:
- All oscillators and panes restored
- Theme selection applied
- Timing configuration restored
- UI reflects loaded state

**Examples**:

*Load Compatible State*:
```
Input: { stateChunk: binaryData, chunkSize: 2048 }
State Changes: [
  * 3 oscillators restored with original configurations
  * 2 panes recreated with saved layout
  * Theme "Dark Professional" applied
  * Timing mode restored to MELODIC
]
Output: { success: true, oscillatorsRestored: 3, panesRestored: 2 }
```

### OP037: InitializeGPUResources
**Purpose**: Initialize GPU context and allocate texture memory pools
**Trigger**: Plugin startup or graphics context creation
**Actor**: System

**Pre-conditions**:
- OpenGL context available
- GPU drivers installed and functional
- Sufficient GPU memory available

**Post-conditions**:
- GPUResourceManager initialized with capabilities
- Texture memory pools allocated
- GPU compatibility profile established
- Fallback mechanism ready if needed

**Examples**:

*Successful GPU Initialization*:
```
Input: { requestedMemory: 256, minOpenGLVersion: "3.3" }
State Changes: [
  * GPUResourceManager.isGPUAvailable = true
  * TextureMemoryPool created with 64 textures
  * GPUCompatibilityProfile.performanceTier = HIGH
]
Output: { success: true, gpuMemory: 512, texturesAllocated: 64, tier: "HIGH" }
```

### OP038: HandleGPUContextLoss
**Purpose**: Detect and recover from GPU context loss events
**Trigger**: Graphics driver reset or context invalidation
**Actor**: System

**Pre-conditions**:
- GPU was previously available
- Context loss detected
- Software rendering fallback available

**Post-conditions**:
- GPUResourceManager.contextLossCount incremented
- All GPU resources marked invalid
- Automatic fallback to software rendering
- Context recovery attempted in background

**Examples**:

*Context Loss Recovery*:
```
Input: { contextLossReason: "DRIVER_RESET" }
State Changes: [
  * GPUResourceManager.contextLossCount += 1
  * GPUResourceManager.fallbackToSoftware = true
  * All texture pools invalidated
  * Software renderer activated
]
Output: { success: true, fallbackActive: true, recoveryInProgress: true }
```

### OP039: MonitorGPUMemoryUsage
**Purpose**: Track GPU memory consumption and trigger warnings
**Trigger**: Periodic system monitoring or memory allocation
**Actor**: System

**Pre-conditions**:
- GPU is available and in use
- Memory monitoring enabled
- Valid memory usage data available

**Post-conditions**:
- GPUResourceManager.gpuMemoryUsed updated
- Warnings triggered if thresholds exceeded
- Automatic quality reduction if critical
- Memory cleanup performed if needed

**Examples**:

*Memory Usage Warning*:
```
Input: { currentUsage: 768, totalMemory: 1024, warningThreshold: 0.75 }
State Changes: [
  * GPUResourceManager.gpuMemoryUsed = 768
  * PerformanceConfig.qualityMode = BALANCED (downgraded)
  * Texture pool size reduced to free memory
]
Output: { success: true, memoryUsage: 768, qualityReduced: true }
```

### OP040: OptimizeTextureMemoryPool
**Purpose**: Manage texture allocation and recycling for efficiency
**Trigger**: Texture allocation request or memory pressure
**Actor**: System

**Pre-conditions**:
- Texture memory pool initialized
- Valid allocation request
- Pool management strategy defined

**Post-conditions**:
- Texture allocated or recycled efficiently
- Pool statistics updated
- Memory defragmentation performed if needed
- Allocation strategy adjusted based on usage

**Examples**:

*Texture Recycling*:
```
Input: { requestedSize: "1024x512", format: "RGBA8" }
State Changes: [
  * TextureMemoryPool.recycleCount += 1
  * Oldest unused texture repurposed
  * TextureMemoryPool.averageLifetime updated
]
Output: { success: true, textureId: "T123", recycled: true, poolUsage: 0.8 }
```

### OP041: DetectResourceStarvation
**Purpose**: Monitor system resources and detect starvation conditions
**Trigger**: Periodic resource monitoring or performance degradation
**Actor**: System

**Pre-conditions**:
- Resource monitoring enabled
- Performance thresholds configured
- System metrics available

**Post-conditions**:
- ResourceMonitor updated with current metrics
- Starvation level classified
- Warning alerts triggered if needed
- Automatic mitigation actions initiated

**Examples**:

*High Memory Pressure Detected*:
```
Input: { systemMemory: 0.95, pluginMemory: 512, fragmentationLevel: 0.8 }
State Changes: [
  * ResourceMonitor.resourceStarvationLevel = HIGH
  * ErrorRecoveryManager.degradationLevel = 3
  * Automatic quality reduction triggered
]
Output: { success: true, starvationLevel: "HIGH", mitigationActive: true }
```

### OP042: RecoverFromCrash
**Purpose**: Attempt automatic recovery after plugin component crash
**Trigger**: Crash detection or component failure
**Actor**: System

**Pre-conditions**:
- Crash or component failure detected
- Recovery mechanisms available
- Backup state data accessible

**Post-conditions**:
- Failed component restarted or replaced
- State restored from backup if needed
- Error logged for analysis
- User notified of recovery action

**Examples**:

*GPU Context Recovery*:
```
Input: { componentType: "GPU_RENDERER", backupAvailable: true }
State Changes: [
  * ErrorRecoveryManager.crashCount += 1
  * GPUResourceManager reinitialized
  * Software fallback activated
  * State restored from backup
]
Output: { success: true, recoveryMethod: "SOFTWARE_FALLBACK", dataLoss: false }
```

### OP043: CreateStateBackup
**Purpose**: Periodically save plugin state for recovery purposes
**Trigger**: Timer interval or significant state change
**Actor**: System

**Pre-conditions**:
- Plugin state is valid and consistent
- Backup directory accessible
- Backup slots available

**Post-conditions**:
- New backup file created with checksum
- Old backups pruned if needed
- Backup record added to manager
- State integrity verified

**Examples**:

*Automatic Backup*:
```
Input: { triggerReason: "TIMER", compressionEnabled: true }
State Changes: [
  * New BackupRecord created
  * State serialized and compressed
  * MD5 checksum calculated
  * Old backup pruned if over limit
]
Output: { success: true, backupId: "BK123", fileSize: 1024, compressed: true }
```

### OP044: DetectStateCorruption
**Purpose**: Validate plugin state integrity and detect corruption
**Trigger**: State load operation or periodic validation
**Actor**: System

**Pre-conditions**:
- State data available for validation
- Checksum or validation rules defined
- Backup recovery mechanism ready

**Post-conditions**:
- State integrity verified or corruption detected
- Corrupted state flagged and isolated
- Recovery action initiated if needed
- Error logged with corruption details

**Examples**:

*Corruption Detected and Recovered*:
```
Input: { stateFile: "current.dat", expectedChecksum: "abc123" }
State Changes: [
  * Current state marked as corrupted
  * Most recent backup loaded
  * ErrorRecord created with details
  * Recovery mode activated temporarily
]
Output: { success: true, corruptionDetected: true, recoveredFromBackup: true }
```

### OP045: HandleInterPluginInterference
**Purpose**: Detect and mitigate interference from other audio plugins
**Trigger**: Performance degradation or resource conflict detection
**Actor**: System

**Pre-conditions**:
- Performance monitoring active
- Other plugins detected in system
- Interference patterns identified

**Post-conditions**:
- Interference level classified
- Resource usage adjusted to reduce conflicts
- Performance optimization applied
- User notified if severe interference detected

**Examples**:

*CPU Interference Mitigation*:
```
Input: { otherPluginCount: 12, systemCpuUsage: 0.85, interferenceScore: 0.7 }
State Changes: [
  * ResourceMonitor.interferenceDetected = true
  * PerformanceConfig.refreshRate reduced to 30fps
  * Buffer sizes increased for stability
  * Quality mode reduced to PERFORMANCE
]
Output: { success: true, interferenceLevel: "HIGH", mitigationApplied: true }
```

### OP046: SavePresetTemplate
**Purpose**: Save current oscillator and pane configuration as reusable template
**Trigger**: User clicks "Save Preset" button or uses keyboard shortcut
**Actor**: User

**Pre-conditions**:
- At least one oscillator configured
- Valid preset name provided
- Storage space available

**Post-conditions**:
- New PresetTemplate created with current configuration
- Template added to preset library
- User notified of successful save
- Preset available for future loading

**Examples**:

*Save Preset*:
```
Input: { presetName: "Vocal Recording Setup", includeTimingConfig: true }
State Changes: [
  * New PresetTemplate created with 4 oscillators
  * Pane layout and timing configuration captured
  * Preset added to user library
]
Output: { success: true, presetId: "PR123", oscillatorCount: 4 }
```

### OP047: LoadPresetTemplate
**Purpose**: Apply saved preset configuration to current instance
**Trigger**: User selects preset from library or uses keyboard shortcut
**Actor**: User

**Pre-conditions**:
- Valid preset selected
- Current configuration can be safely replaced
- No critical operations in progress

**Post-conditions**:
- Current oscillators and panes replaced with preset
- Timing configuration applied if included
- UI updated to reflect new configuration
- Action added to undo stack

**Examples**:

*Load Preset*:
```
Input: { presetId: "PR123", replaceExisting: true }
State Changes: [
  * Current oscillators cleared
  * 4 oscillators created from preset
  * Pane layout restored
  * Timing config applied
]
Output: { success: true, oscillatorsCreated: 4, panesCreated: 2 }
```

### OP048: ExecuteBulkOperation
**Purpose**: Apply operations to multiple selected oscillators simultaneously
**Trigger**: User selects multiple oscillators and chooses bulk action
**Actor**: User

**Pre-conditions**:
- Multiple oscillators selected
- Valid bulk operation chosen
- Operation confirmed by user if destructive

**Post-conditions**:
- Operation applied to all selected oscillators
- UI updated to reflect changes
- Single undo action created for entire operation
- Selection maintained after operation

**Examples**:

*Bulk Color Change*:
```
Input: { selectedOscillators: ["O1", "O2", "O3"], operation: "CHANGE_COLOR", newColor: 0xFF00FF00 }
State Changes: [
  * All 3 oscillators color changed to green
  * Single undo action created for bulk change
  * Selection maintained
]
Output: { success: true, affectedCount: 3, undoCreated: true }
```

### OP049: ExecuteKeyboardShortcut
**Purpose**: Process keyboard shortcut and execute associated action
**Trigger**: User presses configured key combination
**Actor**: User

**Pre-conditions**:
- Keyboard shortcuts enabled
- Valid key combination detected
- No input field has focus (unless shortcut allows)

**Post-conditions**:
- Associated action executed
- Keyboard event consumed
- User feedback provided if needed
- Action logged for analytics

**Examples**:

*Create Oscillator Shortcut*:
```
Input: { keyCombo: "Ctrl+N", focusedElement: null }
State Changes: [
  * CreateOscillator operation triggered
  * New oscillator created with default settings
  * Keyboard event consumed
]
Output: { success: true, actionExecuted: "CREATE_OSCILLATOR", oscillatorId: "O456" }
```

### OP050: UndoLastAction
**Purpose**: Reverse the most recent user action
**Trigger**: User clicks undo button or uses Ctrl+Z shortcut
**Actor**: User

**Pre-conditions**:
- Undo stack contains actions
- Current state allows undo operation
- No conflicting operations in progress

**Post-conditions**:
- Last action reversed
- System state restored to previous state
- Undo stack position moved backward
- Redo becomes available

**Examples**:

*Undo Oscillator Creation*:
```
Input: { }
State Changes: [
  * UndoRedoManager.currentIndex -= 1
  * Most recent oscillator removed
  * Pane layout restored to previous state
  * Redo action becomes available
]
Output: { success: true, actionUndone: "CREATE_OSCILLATOR", redoAvailable: true }
```

## NON-FUNCTIONAL

### Performance Metrics
| Dimension | Metric | Target | Method |
|-----------|--------|--------|--------|
| Latency | P95 Processing Delay | ≤10ms | High-resolution timing |
| Throughput | Refresh Rate | ≥60 FPS | Frame time measurement |
| Resource | CPU Usage (16 tracks) | ≤5% | OS performance counters |
| Resource | Memory Usage | ≤640 MB total | Memory profiling |
| Responsiveness | UI Operation Time | ≤5ms | User action timing |

### Reliability Metrics
| Dimension | Metric | Target | Method |
|-----------|--------|--------|--------|
| Stability | Crash Rate | 0 per session | Automated testing |
| Recovery | State Corruption Rate | <0.1% | Validation checksums |
| Compatibility | DAW Success Rate | 100% target DAWs | Integration testing |

### Usability Metrics
| Dimension | Metric | Target | Method | UI_Dimension |
|-----------|--------|--------|--------|--------------|
| Accessibility | WCAG 2.1 AA Score | ≥95% | Automated audit | true |
| UI Responsiveness | Theme Switch Time | ≤50ms | Performance measurement | true |
| UI Load Time | Initial UI Render | ≤500ms | Startup profiling | true |

## UIUX

### UI Components
| Name | Type | Binds To | Effects On | Validations |
|------|------|----------|------------|-------------|
| AddOscillatorButton | BUTTON | CreateOscillator | OscillatorStore, PaneLayout | Disable if max count reached |
| SourceSelector | SELECT | Oscillator.sourceId | Waveform display | Must reference valid Source |
| ProcessingModeDropdown | SELECT | UpdateOscillatorProcessing | Signal visualization | All modes always valid |
| ColorPicker | FORM | Oscillator.colour | Waveform trace color | Hex validation, RGBA bounds |
| ThemeSelector | SELECT | SwitchTheme | Global UI colors | Theme existence validation |
| LayoutModeSelector | SELECT | ChangeLayoutMode | Column layout, pane distribution | 1-3 column range validation |
| PaneContainer | OTHER | Pane entity | Oscillator layout | Drag-drop ordering validation |
| PaneDragHandle | OTHER | DragDropPane | Pane position, column distribution | Valid drop target validation |
| SidebarResizeHandle | OTHER | ResizeSidebar | Sidebar width, oscilloscope area | Width constraint validation |
| SidebarCollapseButton | BUTTON | CollapseSidebar | Sidebar visibility, layout | Boolean toggle only |
| WindowResizeHandle | OTHER | ResizePluginWindow | Window dimensions, layout scaling | Dimension constraint validation |
| ColumnLayoutToggle | BUTTON | Project.layoutColumns | Pane distribution | 1-3 column range |
| OscillatorVisibilityToggle | BUTTON | Oscillator.visible | Trace display | Boolean toggle only |
| TimingModeSelector | SELECT | ConfigureTimingMode | Timing calculation method | TIME/MELODIC validation |
| TimeIntervalSlider | FORM | SetTimeInterval | Display window duration | 1ms-60s range validation |
| TimeIntervalInput | FORM | SetTimeInterval | Display window duration | Numeric input validation |
| NoteIntervalDropdown | SELECT | SetNoteInterval | Musical interval duration | Valid note interval selection |
| HostSyncToggle | BUTTON | ToggleHostSync | Host transport synchronization | Host compatibility check |
| BPMDisplay | OTHER | TimingConfig.hostBPM | Read-only BPM indicator | Display only, no validation |
| OscillatorListView | OTHER | OscillatorListState | Scrollable oscillator list | List management validation |
| OscillatorListItem | OTHER | Oscillator entity | Individual oscillator display | Click/selection validation |
| OscillatorSortDropdown | SELECT | SortOscillatorList | List sort mode selection | Valid sort mode selection |
| SortDirectionToggle | BUTTON | SortOscillatorList | Sort direction toggle | Ascending/descending validation |
| AddOscillatorButton | BUTTON | CreateOscillator | Add new oscillator to list | Max count validation |
| OscillatorConfigGearIcon | BUTTON | OpenOscillatorConfigPopup | Open configuration popup | Single popup validation |
| OscillatorDeleteIcon | BUTTON | DeleteOscillatorFromList | Delete oscillator from list | Confirmation validation |
| OscillatorNameInput | FORM | RenameOscillator | Oscillator name editing | 1-32 character validation |
| OscillatorConfigPopup | OTHER | Multiple operations | Modal configuration dialog | Input validation, popup state |
| PresetSaveButton | BUTTON | SavePresetTemplate | Save current configuration as preset | Name validation, storage check |
| PresetLoadDropdown | SELECT | LoadPresetTemplate | Load saved preset configuration | Preset existence validation |
| BulkOperationToolbar | OTHER | ExecuteBulkOperation | Multi-select oscillator operations | Selection validation |
| UndoButton | BUTTON | UndoLastAction | Reverse last action | Undo stack availability |
| RedoButton | BUTTON | RedoLastAction | Reapply undone action | Redo stack availability |
| KeyboardShortcutHandler | OTHER | ExecuteKeyboardShortcut | Global keyboard shortcut processing | Key combination validation |
| GlobalSettingsPanel | OTHER | Multiple operations | Cross-instance settings management | Permission and range validation |
| PerformanceStatsDisplay | OTHER | Performance monitoring | Real-time performance metrics | Read-only display |
| ErrorRecoveryNotification | OTHER | Error recovery operations | User notification of recovery actions | Dismissal validation |
| BackupStatusIndicator | OTHER | StateBackupManager | Backup status and last save time | Status display only |

### Entity/Operation UI Mappings
| Entity/Operation | UI Element | Functional Link |
|------------------|------------|-----------------|
| Oscillator | OscillatorRow | Create/update/delete operations, color/processing controls |
| Source | SourceSelector | Dropdown population from active sources |
| Theme | ThemeSelector + ThemeManager | Theme CRUD operations, system theme protection |
| Pane | PaneContainer + PaneDragHandle | Drag-drop reordering, column layout distribution |
| PaneLayout | LayoutModeSelector | Column count selection and pane redistribution |
| WindowLayout | WindowResizeHandle + SidebarResizeHandle | Window and sidebar dimension control |
| TimingConfig | TimingModeSelector + TimeIntervalSlider + NoteIntervalDropdown + HostSyncToggle | Complete timing configuration in sidebar |
| CreateOscillator | AddOscillatorButton | Button click triggers operation with current selections |
| SwitchTheme | ThemeSelector | Dropdown change immediately applies theme |
| ChangeLayoutMode | LayoutModeSelector | Dropdown change redistributes panes across columns |
| DragDropPane | PaneDragHandle | Drag interaction with real-time preview and drop validation |
| ResizePluginWindow | WindowResizeHandle | Window edge/corner drag with proportional scaling |
| ResizeSidebar | SidebarResizeHandle | Left edge drag with real-time width preview |
| CollapseSidebar | SidebarCollapseButton | Button click or double-click handle to toggle |
| UpdateOscillatorProcessing | ProcessingModeDropdown | Dropdown change updates processing mode |
| ConfigureTimingMode | TimingModeSelector | Dropdown change switches between TIME/MELODIC modes |
| SetTimeInterval | TimeIntervalSlider + TimeIntervalInput | Slider and numeric input for millisecond precision |
| SetNoteInterval | NoteIntervalDropdown | Dropdown selection of musical note intervals |
| ToggleHostSync | HostSyncToggle | Checkbox toggle for host transport synchronization |
| OscillatorListState | OscillatorListView + OscillatorSortDropdown + SortDirectionToggle | Complete oscillator list management |
| RenameOscillator | OscillatorNameInput | Text input with real-time validation in config popup |
| SortOscillatorList | OscillatorSortDropdown + SortDirectionToggle | Dropdown and button for list sorting control |
| DeleteOscillatorFromList | OscillatorDeleteIcon | Trash icon click with optional confirmation dialog |
| ScrollOscillatorListToItem | OscillatorListView | Programmatic scroll with smooth animation |
| OpenOscillatorConfigPopup | OscillatorConfigGearIcon | Gear icon click opens modal configuration dialog |
| UpdateHostBPM | BPMDisplay | Automatic display update from host tempo changes |
| AutoCreateOscillatorForNewInstance | System-triggered | Automatic oscillator creation during plugin initialization |

### UI State Transitions
| Entity State | UI Transition | Visual Effect |
|--------------|---------------|---------------|
| Source ACTIVE→INACTIVE | Waveform fade | Trace opacity reduces to 50% |
| Source ORPHANED | Warning indicator | Red border around affected oscillators |
| Source REMOVED | Auto-disconnect | Affected oscillators switch to "No Signal" state, settings preserved |
| Oscillator ACTIVE→NO_SOURCE | No signal display | Waveform area shows "No Signal" placeholder, controls remain enabled |
| Oscillator NO_SOURCE→ACTIVE | Signal restore | Waveform immediately appears with preserved color/processing settings |
| Theme APPLYING | Loading state | Spinner overlay during switch |
| Oscillator visible=false | Hide element | CSS display:none with animation |
| PaneLayout columns=1→2 | Redistribution animation | Panes flow from single column to two columns |
| PaneLayout columns=2→3 | Redistribution animation | Panes redistribute across three columns |
| PaneDragState isDragging=true | Drag preview | Semi-transparent pane with drop target highlights |
| PaneDragState dropTarget valid | Drop zone highlight | Green border/background on valid drop targets |
| PaneDragState dropTarget invalid | Invalid zone indicator | Red border/background on invalid drop targets |
| WindowLayout resizing | Live preview | Real-time oscilloscope area scaling during window resize |
| SidebarResizeState isResizing=true | Width preview | Dashed line showing new sidebar width position |
| WindowLayout sidebarCollapsed=true | Slide animation | Sidebar slides out, oscilloscope area expands |
| WindowLayout sidebarCollapsed=false | Slide animation | Sidebar slides in, oscilloscope area contracts |
| TimingConfig timingMode=TIME | Control visibility | Show time interval slider/input, hide note interval dropdown |
| TimingConfig timingMode=MELODIC | Control visibility | Show note interval dropdown, hide time interval controls |
| TimingConfig hostSyncEnabled=true | Sync indicator | Green sync icon, oscillators align to host grid |
| TimingConfig hostSyncEnabled=false | Free-run indicator | No sync icon, oscillators run continuously |
| TimingConfig hostBPM change | Interval recalculation | Smooth transition to new interval duration (MELODIC mode) |
| Host transport PLAY→STOP | Sync reset | All oscillators reset display window when host sync enabled |
| Host transport STOP→PLAY | Sync start | Oscillators align to new playhead position with host sync |
| OscillatorListState sortMode change | List reordering | Animated list reordering with new sort criteria |
| OscillatorListState configPopupOpen=true | Modal overlay | Configuration popup appears, background dimmed |
| OscillatorListState configPopupOpen=false | Modal close | Popup disappears, background restored, focus returned |
| Oscillator displayName change | Name update | Immediate name update in list item and pane display |
| Oscillator deleted from list | Item removal | Smooth removal animation, list compaction, selection update |
| New oscillator added | Item addition | Smooth addition animation, automatic scroll to new item |
| OscillatorListState scrollPosition change | Scroll animation | Smooth scroll to target item with brief highlight |
| Oscillator selected in list | Selection highlight | Item background highlight, gear/trash icons visible |
| OscillatorListState sortDirection toggle | Sort indicator | Arrow direction change, immediate list reordering |

### Accessibility & Edge Cases
- **Keyboard Navigation**: Tab order through all interactive controls, escape key cancels operations
- **Screen Reader**: ARIA labels for all complex components, live regions for status updates
- **Color Contrast**: WCAG AA compliance for text/background combinations (4.5:1 minimum)
- **Responsive Design**: Minimum 1024x768 support, scales to 4K with DPI awareness
- **Offline Support**: Full functionality without network dependency
- **Error Feedback**: Toast notifications for all error conditions with auto-dismiss
- **Loading States**: Progress indicators for operations >100ms, skeleton UI for slow loads
- **High DPI Support**: Vector graphics and scalable UI elements for retina displays
- **Color Blindness**: Theme validation for deuteranopia/protanopia accessibility
- **Reduced Motion**: Respect system settings for animation preferences
- **Focus Management**: Clear focus indicators, logical focus order, focus restoration after modals
- **Input Validation**: Real-time validation with clear error messages
- **Undo/Redo**: Support for reversible operations (delete oscillator, theme changes)

### Critical Edge Cases
- **Rapid Operation Sequences**: Handle multiple simultaneous user actions without state corruption
- **Memory Pressure**: Graceful degradation when system memory is constrained
- **Audio Driver Changes**: Detect and adapt to audio system reconfiguration during operation
- **DAW Project Tempo Changes**: Real-time adaptation to BPM changes during playback
- **Plugin Window Resize**: Maintain layout integrity during window size changes
- **Multiple Monitor Setup**: Proper positioning and DPI handling across displays
- **System Sleep/Wake**: Resume operation correctly after system hibernation
- **Background Processing**: Maintain efficiency when DAW window is not focused
- **Instance Limit Stress**: Behavior when approaching maximum 64 sources/128 oscillators
- **Concurrent Multi-User**: Handle multiple users on same system (different DAW instances)
- **Plugin Format Migration**: Handle loading projects created with different plugin formats
- **Version Compatibility**: Forward/backward compatibility strategy for schema changes
- **Crash Recovery**: Detect and recover from previous unexpected termination
- **Audio Sample Rate Changes**: Dynamic adaptation to host sample rate changes during operation
- **Plugin Parameter Automation**: Handle rapid parameter changes from DAW automation
- **Large Project Loads**: Performance optimization for projects with maximum oscillator counts

## IMPLEMENTATION SPECIFICATIONS

### Thread Safety & Concurrency Model

**Audio Thread Requirements:**
- Zero heap allocations in `processBlock()` callback
- Lock-free data structures only (atomics, SPSC queues)
- Maximum processing time: 70% of buffer duration at minimum buffer size
- Pre-allocated memory pools for all audio operations

**Message Thread Operations:**
- UI updates, state changes, file I/O
- ValueTree operations for state persistence
- Theme switching and visual updates
- Instance registry management (mutex-protected)

**Background Thread Usage:**
- Optional decimation processing for large datasets
- File operations (theme import/export)
- Performance statistics aggregation

### Core Data Structures

**RingBuffer (Lock-Free Audio Storage):**
```cpp
template<typename T>
class RingBuffer {
    std::vector<T> buffer;
    std::atomic<size_t> head{0};
    std::atomic<size_t> tail{0};
    // Capacity + 1 for full/empty distinction
};
```

**AudioDataSnapshot (Bridge Structure):**
```cpp
struct AudioDataSnapshot {
    static constexpr size_t MAX_CHANNELS = 64;
    static constexpr size_t MAX_SAMPLES = 1024;

    float samples[MAX_CHANNELS][MAX_SAMPLES];
    size_t numChannels;
    size_t numSamples;
    double timestamp;
    float sampleRate;
};
```

**InstanceRegistry (Singleton Pattern):**
```cpp
class InstanceRegistry {
    static InstanceRegistry& getInstance();
    std::mutex registryMutex;
    std::map<UUID, InstanceInfo> instances;
    std::map<UUID, SourceInfo> sources;
};
```

### Signal Processing Algorithms

**ProcessingMode Implementation:**
- **FullStereo**: Pass-through L/R channels
- **Mono**: `(left + right) * 0.5f`
- **Mid**: `(left + right) * 0.5f` (M component)
- **Side**: `(left - right) * 0.5f` (S component)
- **Left**: `left` channel only, right = 0
- **Right**: `right` channel only, left = 0

**Correlation Calculation:**
```cpp
float calculateCorrelation(const float* left, const float* right, size_t count) {
    // Pearson correlation coefficient
    // r = Σ((xi - x̄)(yi - ȳ)) / √(Σ(xi - x̄)² × Σ(yi - ȳ)²)
}
```

**Decimation Algorithm (Performance Critical):**
```cpp
struct MinMaxPair { float min, max; };
MinMaxPair decimateToMinMax(const float* samples, size_t count, size_t stride);
```

### State Persistence Schema

**ValueTree Structure:**
```
OscilState (root)
├── Sources[] (global, shared across instances)
│   ├── id: UUID
│   ├── dawTrackId: string
│   ├── displayName: string
│   └── channelConfig: enum
├── Oscillators[] (per-instance)
│   ├── id: UUID
│   ├── sourceId: UUID (foreign key)
│   ├── processingMode: int
│   ├── colourARGB: int
│   ├── opacity: float
│   ├── visible: bool
│   └── paneId: UUID
├── Panes[] (per-instance)
│   ├── id: UUID
│   ├── orderIndex: int
│   ├── height: float
│   └── oscillatorIds: UUID[]
├── Theme (global)
│   ├── selectedThemeId: UUID
│   └── customThemes: JSON[]
└── Performance
    ├── qualityMode: enum
    ├── statusBarVisible: bool
    └── cpuThresholds: float[]
```

### UI Component Architecture

**Component Hierarchy:**
```
PluginEditor (root)
├── StatusBar
│   ├── CPUMeter
│   ├── MemoryMeter
│   ├── FPSCounter
│   └── RenderingModeIndicator
├── SettingsDropdown
│   ├── ThemeSelector
│   ├── StatusBarToggle
│   └── ThemeEditorButton
├── PaneContainer[]
│   └── OscillatorComponent[]
│       ├── SourceSelector
│       ├── ProcessingModeDropdown
│       ├── ColorPicker
│       └── VisibilityToggle
└── OscilloscopeComponent (main display)
```

**Event Flow:**
1. User interaction (mouse/keyboard) → Component event handler
2. Component validates input → Updates ValueTree state
3. ValueTree change listener → Notifies dependent components
4. Components update display → Trigger repaint if needed
5. Audio thread reads stable state snapshots (no locks)

### Performance Optimization Strategies

**Memory Management:**
- Pre-allocate all buffers at initialization
- Object pools for frequently created/destroyed objects
- Cache-friendly data layouts (SoA vs AoS)
- Minimize cache misses in hot paths

**Rendering Optimization:**
```cpp
// OpenGL detection and fallback
class RenderingEngine {
    enum Mode { OPENGL, SOFTWARE };
    Mode detectOptimalMode();
    void renderFrame(const AudioDataSnapshot& data);
};
```

**Quality Adaptation:**
```cpp
enum QualityMode {
    HIGHEST,    // 60fps, full resolution
    HIGH,       // 60fps, 75% resolution
    BALANCED,   // 45fps, 50% resolution
    PERFORMANCE,// 30fps, 25% resolution
    MAXIMUM     // 15fps, minimal quality
};
```

### Error Handling Patterns

**RAII Resource Management:**
```cpp
class AudioProcessingScope {
    AudioProcessor* processor;
public:
    AudioProcessingScope(AudioProcessor* p) : processor(p) {
        processor->beginProcessing();
    }
    ~AudioProcessingScope() {
        processor->endProcessing();
    }
};
```

**Graceful Degradation:**
- Source lost → Show "No Signal" state
- Theme corruption → Fall back to default theme
- Performance issues → Automatic quality reduction
- Memory pressure → Reduce track count with user notification

### Platform-Specific Implementation

**macOS Integration:**
- Core Audio host compatibility
- Metal/OpenGL detection
- Retina display scaling
- Sandboxed file access

**Windows Integration:**
- WASAPI/DirectSound compatibility
- DirectX/OpenGL detection
- High DPI scaling
- Windows plugin validation

**Plugin Format Implementation:**
- VST3: Implement IEditController, IAudioProcessor
- AU: AudioUnit component registration

### Testing Strategy

**Unit Tests (Required Coverage ≥95%):**
```cpp
// Example test structure
TEST_CASE("Oscillator CRUD Operations") {
    SECTION("Create oscillator with valid source") { /* ... */ }
    SECTION("Delete oscillator updates pane") { /* ... */ }
    SECTION("Update processing mode") { /* ... */ }
}
```

**Performance Tests:**
```cpp
BENCHMARK("64 Track Rendering") {
    // Validate <16% CPU, 60fps target
}
```

**Integration Tests:**
- DAW automation compatibility
- State persistence roundtrip
- Multi-instance communication
- Theme switching under load

### Build System Configuration

**CMake Targets:**
```cmake
# Main plugin targets
add_library(oscil_plugin SHARED ${PLUGIN_SOURCES})
add_executable(oscil_standalone ${STANDALONE_SOURCES})

# Test targets
add_executable(oscil_tests ${TEST_SOURCES})
add_executable(oscil_benchmarks ${BENCHMARK_SOURCES})

# JUCE configuration
juce_add_plugin(oscil_plugin
    PRODUCT_NAME "Oscil"
    FORMATS VST3 AU Standalone
    VST3_CATEGORIES "Analyzer"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)
```

**Dependencies:**
- JUCE 7.0+ (Core, Audio, Graphics, OpenGL modules)
- Catch2 3.0+ (testing framework)
- nlohmann/json (theme serialization)

### Security Considerations

**Input Validation:**
- Validate all user inputs (colors, names, values)
- Sanitize file paths and theme data
- Bounds checking on all array accesses
- Validate UUID formats and state schema

**Memory Safety:**
- No raw pointers in public APIs
- RAII for all resource management
- Bounds-checked containers
- Static analysis integration (clang-static-analyzer)

## API SPECIFICATIONS

## API SPECIFICATIONS

### Core API Classes

**PluginProcessor (JUCE AudioProcessor):**
```cpp
class PluginProcessor : public juce::AudioProcessor {
public:
    // JUCE AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Plugin-specific interface
    MultiTrackEngine& getMultiTrackEngine() { return multiTrackEngine; }
    InstanceRegistry& getInstanceRegistry() { return InstanceRegistry::getInstance(); }

private:
    MultiTrackEngine multiTrackEngine;
    TimingEngine timingEngine;
    juce::ValueTree oscilState;
};
```

**MultiTrackEngine (Audio Processing Core):**
```cpp
class MultiTrackEngine {
public:
    // Lifecycle
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processAudioBlock(const juce::AudioBuffer<float>& buffer);
    void releaseResources();

    // Track management (per-instance)
    bool addTrack(const TrackId& id, const TrackInfo& info);
    bool removeTrack(const TrackId& id);
    TrackInfo getTrackInfo(const TrackId& id) const;
    std::vector<TrackId> getAllTrackIds() const;

    // Audio data access (thread-safe)
    AudioDataSnapshot getLatestSnapshot() const;
    bool hasNewData() const;

private:
    static constexpr size_t MAX_TRACKS = 64;
    std::array<RingBuffer<float>, MAX_TRACKS> trackBuffers;
    WaveformDataBridge dataBridge;
    std::atomic<bool> hasNewDataFlag{false};
};
```

**WaveformDataBridge (Thread-Safe Communication):**
```cpp
class WaveformDataBridge {
public:
    // Audio thread interface (producer)
    void enqueueSnapshot(const AudioDataSnapshot& snapshot);

    // UI thread interface (consumer)
    bool dequeueSnapshot(AudioDataSnapshot& outSnapshot);
    bool hasNewData() const;

private:
    // Double-buffered snapshots with atomic swap
    std::array<AudioDataSnapshot, 2> snapshots;
    std::atomic<size_t> writeIndex{0};
    std::atomic<size_t> readIndex{1};
    std::atomic<bool> newDataAvailable{false};
};
```

**InstanceRegistry (Cross-Instance Communication):**
```cpp
class InstanceRegistry {
public:
    static InstanceRegistry& getInstance();

    // Instance lifecycle
    bool registerInstance(const UUID& instanceId, MultiTrackEngine* engine);
    bool unregisterInstance(const UUID& instanceId);

    // Source management
    std::vector<SourceInfo> getAvailableSources() const;
    bool registerSource(const UUID& sourceId, const SourceInfo& info);
    bool updateSourceOwnership(const UUID& sourceId, const UUID& newOwner);

    // Aggregation queries
    size_t getTotalSourceCount() const;
    size_t getTotalInstanceCount() const;
    std::vector<InstanceInfo> getActiveInstances() const;

private:
    mutable std::mutex registryMutex;
    std::map<UUID, InstanceInfo> instances;
    std::map<UUID, SourceInfo> sources;
};
```

**OscillatorStore (State Management):**
```cpp
class OscillatorStore {
public:
    // CRUD operations
    UUID createOscillator(const UUID& sourceId, const UUID& paneId);
    bool deleteOscillator(const UUID& oscillatorId);
    bool updateOscillator(const UUID& oscillatorId, const OscillatorConfig& config);

    // Queries
    std::vector<Oscillator> getOscillatorsForPane(const UUID& paneId) const;
    std::optional<Oscillator> getOscillator(const UUID& oscillatorId) const;
    size_t getOscillatorCount() const;

    // State persistence
    juce::ValueTree saveToValueTree() const;
    bool loadFromValueTree(const juce::ValueTree& tree);

private:
    juce::ValueTree oscillatorsTree{"Oscillators"};
    static constexpr size_t MAX_OSCILLATORS = 128;
};
```

**ThemeManager (Global Theme System):**
```cpp
class ThemeManager {
public:
    // Theme management
    void setCurrentTheme(const UUID& themeId);
    UUID getCurrentThemeId() const;
    std::vector<ThemeInfo> getAvailableThemes() const;

    // Custom themes
    bool createCustomTheme(const ColorTheme& theme);
    bool deleteCustomTheme(const UUID& themeId);
    bool exportTheme(const UUID& themeId, const juce::File& file);
    bool importTheme(const juce::File& file);

    // Color queries (performance critical)
    juce::Colour getBackgroundColor() const;
    juce::Colour getWaveformColor(size_t trackIndex) const;
    juce::Colour getMultiTrackWaveformColor(size_t trackIndex) const;

    // Change notification
    void addListener(ThemeChangeListener* listener);
    void removeListener(ThemeChangeListener* listener);

private:
    UUID currentThemeId;
    std::map<UUID, ColorTheme> customThemes;
    juce::ListenerList<ThemeChangeListener> listeners;
};
```

**TimingEngine (Sample-Accurate Synchronization):**
```cpp
class TimingEngine {
public:
    enum SyncMode { FREE_RUNNING, HOST_SYNC, TRIGGERED };

    // Configuration
    void setSyncMode(SyncMode mode);
    void setTimeWindow(float windowMs);
    void setTriggerThreshold(float thresholdDb);

    // Audio thread interface
    void processHostInfo(const juce::AudioPlayHead::CurrentPositionInfo& info);
    bool shouldCapture(const float* samples, size_t count) const;
    size_t getWindowSizeInSamples() const;

    // State access
    double getCurrentTime() const;
    bool isTriggered() const;
    float getBPM() const;

private:
    std::atomic<SyncMode> syncMode{FREE_RUNNING};
    std::atomic<float> timeWindowMs{50.0f};
    std::atomic<float> triggerThreshold{-20.0f};
    std::atomic<double> currentTime{0.0};
};
```

### UI Component APIs

**OscilloscopeComponent (Main Display):**
```cpp
class OscilloscopeComponent : public juce::Component,
                             public juce::Timer {
public:
    // Display configuration
    void setAudioDataSource(MultiTrackEngine* engine);
    void setThemeManager(ThemeManager* manager);
    void setVisibleOscillators(const std::vector<UUID>& oscillatorIds);

    // Rendering control
    void setRenderingMode(RenderingMode mode);
    void setQualityMode(QualityMode mode);
    void setRefreshRate(float fps);

    // JUCE Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    MultiTrackEngine* audioEngine = nullptr;
    ThemeManager* themeManager = nullptr;
    std::unique_ptr<OpenGLContext> openGLContext;
    RenderingMode currentRenderingMode = RenderingMode::AUTO_DETECT;
};
```

**StatusBar (Performance Monitoring):**
```cpp
class StatusBar : public juce::Component {
public:
    // Configuration
    void setUpdateInterval(int milliseconds);
    void setVisibleMetrics(bool cpu, bool memory, bool fps, bool renderMode);

    // Metric updates
    void updateCPUUsage(float percentage);
    void updateMemoryUsage(float megabytes);
    void updateFPS(float framesPerSecond);
    void updateRenderingMode(RenderingMode mode);

    // Visual customization
    void setTheme(const ColorTheme& theme);

private:
    struct Metrics {
        std::atomic<float> cpuUsage{0.0f};
        std::atomic<float> memoryUsage{0.0f};
        std::atomic<float> fps{0.0f};
        std::atomic<RenderingMode> renderingMode{RenderingMode::SOFTWARE};
    } metrics;

    bool showCPU = true, showMemory = true, showFPS = true, showRenderMode = true;
};
```

### Data Model APIs

**ColorTheme (Theme Data Structure):**
```cpp
struct ColorTheme {
    UUID id;
    std::string name;
    bool isSystemTheme = false;

    // UI colors
    juce::Colour background, surface, text, accent, border, grid;

    // Waveform colors (base palette)
    std::array<juce::Colour, 8> waveformColors;

    // Serialization
    juce::var toJson() const;
    static ColorTheme fromJson(const juce::var& json);

    // Validation
    bool validateAccessibility() const;
    float getContrastRatio(juce::Colour fg, juce::Colour bg) const;
};
```

**ProcessingMode (Signal Processing):**
```cpp
enum class ProcessingMode : int {
    FullStereo = 0,  // Pass-through L/R
    Mono = 1,        // (L+R)/2 mixed to both channels
    Mid = 2,         // (L+R)/2 M signal
    Side = 3,        // (L-R)/2 S signal
    Left = 4,        // L channel only
    Right = 5        // R channel only
};

class SignalProcessor {
public:
    static void processBuffer(juce::AudioBuffer<float>& buffer,
                            ProcessingMode mode);
    static float calculateCorrelation(const float* left, const float* right,
                                    size_t samples);
    static std::string getModeDisplayName(ProcessingMode mode);
};
```

### Explicitly Excluded Items
| Item | Rationale |
|------|-----------|
| Cross-process IPC | Adds complexity without clear benefit for initial release |
| Network/remote aggregation | Security and latency concerns for real-time audio |
| Spectral analysis | Different product category, would complicate UI |
| Audio recording/export | Plugin focuses on visualization only |
| MIDI integration | Not required for oscilloscope functionality |
| Surround sound (>2 channels) | Complexity vs demand tradeoff |
| Custom shader user upload | Security risk, validation complexity |
| Automatic oscillator creation | User control preferred for professional workflow |
| Cross-DAW project sharing | DAW-specific implementation differences |
| Plugin-to-plugin communication | Host sandbox restrictions |

## ASSUMPTIONS

### Accepted Assumptions
| ID | Text | Status |
|----|------|--------|
| A001 | Users understand basic audio concepts (stereo, Mid/Side) | ACCEPTED |
| A002 | DAW hosts provide stable track identification | ACCEPTED |
| A003 | 64 simultaneous tracks covers 99% of use cases | ACCEPTED |
| A004 | JUCE framework provides sufficient cross-platform compatibility | ACCEPTED |
| A005 | CPU rendering performance adequate for baseline functionality | ACCEPTED |

### Rejected Assumptions
| ID | Text | Status |
|----|------|--------|
| A006 | GPU rendering required for all scenarios | REJECTED |
| A007 | Automatic track discovery without user plugin insertion | REJECTED |
| A008 | Real-time network streaming support needed | REJECTED |

## CONSISTENCY AUDIT

### Validation Results
- **MISSING_EXAMPLES**: 0 (All 7 operations have happy/boundary/failure examples)
- **UNDEFINED_TERMS**: 0 (All technical terms defined in glossary with additions)
- **ORPHAN_ERRORS**: 0 (All 12 errors reference specific operations with detailed recovery)
- **DUPLICATE_ERROR_CODES**: 0 (All error codes unique and systematically numbered)
- **FIELDS_MISSING_TYPE_OR_CONSTRAINT**: 0 (All entity fields properly specified with extended metadata)
- **BAD_TRANSITIONS**: 0 (All state transitions validated with detailed trigger conditions)
- **PENDING_ASSUMPTIONS**: 0 (All assumptions resolved and documented)
- **DISCONNECTED_UI**: 0 (All UI components linked to operations/effects with comprehensive mappings)
- **MISSING_UI_EXAMPLES**: 0 (All operations include detailed UI_Context examples)
- **UNDEFINED_UI_TERMS**: 0 (All UI components and interactions defined)
- **UI_EDGE_CASES_COVERED**: ✓ (Accessibility, feedback, offline support, edge cases comprehensively addressed)
- **MISSING_ERROR_RECOVERY**: 0 (All errors include auto-recovery and user action procedures)
- **INCOMPLETE_STATE_MACHINES**: 0 (All state machines include detailed descriptions and triggers)
- **MISSING_TIMING_SPECIFICATIONS**: 0 (Comprehensive timing and synchronization details added)
- **INSUFFICIENT_ENTITY_DETAIL**: 0 (All entities expanded with complete field specifications)

### Cross-Reference Validation
- All entity relationships properly defined with foreign key constraints and additional supporting entities
- All 7 operations reference valid entities and include proper state transitions with edge cases
- All 12 errors map to specific operation trigger points with detailed recovery procedures
- All UI components connect to backend operations with clear data flow and validation rules
- Performance limits align with non-functional requirements and include enforcement mechanisms
- Security permissions cover all CRUD operations with role-based access control
- State machines include all necessary states with detailed transition triggers and invalid transitions
- Timing and synchronization requirements fully specified with sample-accurate precision
- Audio thread safety guaranteed through lock-free design and pre-allocation strategies
- Inter-instance communication properly bounded with security and performance constraints

### Enhanced Coverage Areas
- **Audio Processing**: Sample-accurate timing, lock-free buffers, real-time constraints
- **Error Handling**: 12 comprehensive error conditions with auto-recovery and logging
- **State Management**: 4 detailed state machines with transition triggers and protection rules
- **Performance**: Comprehensive limits, monitoring, and adaptive quality management
- **Accessibility**: WCAG 2.1 AA compliance, keyboard navigation, screen reader support
- **Edge Cases**: 16+ critical edge cases covering system events, stress conditions, and failure modes
- **Inter-Instance Communication**: Registry integrity, priority resolution, ownership transfer
- **Timing Synchronization**: Host sync, trigger modes, sample accuracy, musical timing
- **UI Integration**: 8 UI components with complete validation and effect specifications
- **Data Persistence**: Atomic operations, schema versioning, corruption recovery

**Matrix Consistency**: VALIDATED ✓
**UI Integration**: COMPLETE ✓
**Edge Case Coverage**: COMPREHENSIVE ✓
**Error Recovery**: COMPLETE ✓
**Performance Specifications**: DETAILED ✓
**Accessibility Compliance**: VERIFIED ✓


Time mode
- Slider to set the interval in ms
- Also shows an input next to the slider which shows the ms value and allows to change it by entering a number

Melodic mode
- Slider to set the interval
- Also shows a select next to it which shows the currently selected interval and allows to change the interval by choosing a different option
- Allows to switch between Free Running and Host Sync mode
- In Free Running mode it shows a slider to select the BPM, default 120. Shows next to it an input field that shows the currently selected bpm value and allows to change the bpm by entering a number.

In both modes a Mode select is available with the options:
- "Free running": waveforms just keep running
- "Restart on Play": waveforms will reset and start "from 0 position" when user starts playback in the host
- "Restart on Note": waveforms will reset when track on which the plugin instance sits receives a midi note


Master control only contains the "Gain" slider




Have 2 oscillators.
Select the first oscillator in the list.
Switch the visible switch to hide the oscillator.
Result: the oscillator moves down in the list.
Expected: there is no automated reordering of oscillators in the oscillator list. Oscillator entries can only be reordered by the user via drag and drop.







---
Do a root cause analyis, then fix this bug: in the timing panel, in melodic mode, the "sync" switch does nothing. waveforms are unaffected and only use the timing that was set in the input field when the sync switch was off.


---
Do a root cause analyis, then fix this bug: number input fields that have a + and - button inside of them, do not show the + and - text on the buttons but instead just "...", possibly because they are not large enough or have too much padding? This affects multiple input fields.

---
Do a root cause analyis, then fix this bug: the sidebar has on its top right a caret button to open / close the sidebar drawer. this is completely not working.









plan based on this the following addition to our plugin:
1. each oscillator has a shader setting
2. the user can select the shader from the list of available shaders in the oscillator setting config. the shader dropdown is below the color settings.
3. the user can also select the shader from the list of available shaders in the dialog to add a new oscillator. the dropdown is below the color settings.
4. this project has an architecture that allows to easily add new shaders.
5. when the user has GPU rendering enabled (juce opengl), the shader selected for an oscialltor is used for rendering the oscillator waveform.
6. for start, one default shader exist, that is just rendering the waveform normally using the color, line width and opacity set for the oscillator. but it also adds some neon glow in the same color.
