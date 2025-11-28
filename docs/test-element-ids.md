# Test Element ID Naming Convention

## Overview

This document defines the naming convention for test element IDs used in E2E automation. All UI components that need to be testable must register with `TestElementRegistry` using these IDs.

## Naming Rules

### Format
```
{category}_{component}_{qualifier}
```

### Rules
1. Use **camelCase** for all IDs
2. Use underscores `_` to separate logical parts
3. Dynamic indices use `_{index}` suffix (0-based)
4. Dynamic IDs use `_{id}` suffix for UUIDs
5. Child elements append `_{childName}` to parent ID

### Examples
```
toolbar_columnSelector          # Toolbar > Column selector dropdown
sidebar_oscillators_item_0      # Sidebar > Oscillators section > Item at index 0
sidebar_oscillators_item_0_editBtn  # Edit button on oscillator item 0
configPopup_nameField           # Config popup > Name text field
pane_0_header                   # Pane at index 0 > Header (drag handle)
```

---

## Element ID Registry

### Toolbar Elements
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `toolbar_columnSelector` | PluginEditor | ComboBox | Column layout dropdown (1/2/3) |
| `toolbar_addOscillatorBtn` | PluginEditor | Button | Add oscillator button |
| `toolbar_themeSelector` | PluginEditor | ComboBox | Theme dropdown |
| `toolbar_sidebarToggleBtn` | PluginEditor | Button | Sidebar collapse/expand |
| `toolbar_settingsBtn` | PluginEditor | Button | Settings dropdown trigger |

### Sidebar Container
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `sidebar` | SidebarComponent | Component | Main sidebar container |
| `sidebar_resizeHandle` | SidebarComponent | Component | Resize drag handle |

### Sources Section
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `sidebar_sources` | SourceSelectorComponent | Component | Sources section container |
| `sidebar_sources_header` | SourceSelectorComponent | Component | Section header (collapsible) |
| `sidebar_sources_item_{sourceId}` | SourceItemComponent | Component | Individual source item |
| `sidebar_sources_item_{sourceId}_indicator` | SourceItemComponent | Component | Activity indicator |
| `sidebar_sources_item_{sourceId}_paneDropdown` | SourceItemComponent | ComboBox | Add to pane dropdown |

### Oscillators Section
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `sidebar_oscillators` | OscillatorPanel | Component | Oscillators section container |
| `sidebar_oscillators_header` | OscillatorPanel | Component | Section header |
| `sidebar_oscillators_toolbar` | OscillatorListToolbar | Component | Filter toolbar |
| `sidebar_oscillators_toolbar_allTab` | OscillatorListToolbar | Button | "All" filter tab |
| `sidebar_oscillators_toolbar_visibleTab` | OscillatorListToolbar | Button | "Visible" filter tab |
| `sidebar_oscillators_toolbar_hiddenTab` | OscillatorListToolbar | Button | "Hidden" filter tab |
| `sidebar_oscillators_list` | OscillatorPanel | Component | Oscillator list container |
| `sidebar_oscillators_item_{index}` | OscillatorListItem | Component | Oscillator item at index |
| `sidebar_oscillators_item_{index}_colorIndicator` | OscillatorListItem | Component | Color swatch |
| `sidebar_oscillators_item_{index}_name` | OscillatorListItem | Label | Oscillator name |
| `sidebar_oscillators_item_{index}_sourceName` | OscillatorListItem | Label | Source name |
| `sidebar_oscillators_item_{index}_settingsBtn` | OscillatorListItem | Button | Settings gear icon |
| `sidebar_oscillators_item_{index}_visibilityBtn` | OscillatorListItem | Button | Eye icon toggle |
| `sidebar_oscillators_item_{index}_deleteBtn` | OscillatorListItem | Button | Delete button |
| `sidebar_oscillators_item_{index}_modeBtn_{mode}` | OscillatorListItem | Button | Mode button (stereo/mono/mid/side/l/r) |

### Timing Section
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `sidebar_timing` | TimingSidebarSection | Component | Timing section container |
| `sidebar_timing_header` | TimingSidebarSection | Component | Section header |
| `sidebar_timing_modeToggle` | TimingSidebarSection | SegmentedButton | TIME/MELODIC mode |
| `sidebar_timing_intervalSlider` | TimingSidebarSection | Slider | Time interval slider |
| `sidebar_timing_noteDropdown` | TimingSidebarSection | ComboBox | Note interval dropdown |
| `sidebar_timing_hostSyncToggle` | TimingSidebarSection | Toggle | Host sync toggle |
| `sidebar_timing_resetOnPlayToggle` | TimingSidebarSection | Toggle | Reset on play toggle |
| `sidebar_timing_bpmDisplay` | TimingSidebarSection | Label | BPM display |
| `sidebar_timing_intervalDisplay` | TimingSidebarSection | Label | Calculated interval |

### Master Controls Section
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `sidebar_masterControls` | MasterControlsSection | Component | Master controls container |
| `sidebar_masterControls_header` | MasterControlsSection | Component | Section header |
| `sidebar_masterControls_timebaseSlider` | MasterControlsSection | Slider | Timebase slider |
| `sidebar_masterControls_timebaseValue` | MasterControlsSection | Label | Timebase value display |
| `sidebar_masterControls_gainSlider` | MasterControlsSection | Slider | Gain slider |
| `sidebar_masterControls_gainValue` | MasterControlsSection | Label | Gain value display |

### Trigger Settings Section
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `sidebar_trigger` | TriggerSettingsSection | Component | Trigger section container |
| `sidebar_trigger_header` | TriggerSettingsSection | Component | Section header |
| `sidebar_trigger_modeDropdown` | TriggerSettingsSection | ComboBox | Trigger mode dropdown |
| `sidebar_trigger_thresholdSlider` | TriggerSettingsSection | Slider | Threshold slider |
| `sidebar_trigger_edgeToggle` | TriggerSettingsSection | SegmentedButton | Rising/Falling edge |

### Display Options Section
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `sidebar_display` | DisplayOptionsSection | Component | Display section container |
| `sidebar_display_header` | DisplayOptionsSection | Component | Section header |
| `sidebar_display_gridToggle` | DisplayOptionsSection | Toggle | Show grid toggle |
| `sidebar_display_autoScaleToggle` | DisplayOptionsSection | Toggle | Auto-scale toggle |
| `sidebar_display_holdToggle` | DisplayOptionsSection | Toggle | Hold display toggle |

### Pane Components
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `pane_{index}` | PaneComponent | Component | Pane at column index |
| `pane_{index}_header` | PaneComponent | Component | Pane header (drag zone) |
| `pane_{index}_waveform` | WaveformComponent | Component | Waveform display |

### Config Popup
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `configPopup` | OscillatorConfigPopup | Component | Popup overlay |
| `configPopup_closeBtn` | OscillatorConfigPopup | Button | Close button |
| `configPopup_nameField` | OscillatorConfigPopup | TextEditor | Name input |
| `configPopup_sourceDropdown` | OscillatorConfigPopup | ComboBox | Source selector |
| `configPopup_paneDropdown` | OscillatorConfigPopup | ComboBox | Pane selector |
| `configPopup_modeSelector` | OscillatorConfigPopup | SegmentedButton | Processing mode |
| `configPopup_colorPicker` | OscillatorConfigPopup | Component | Color picker |
| `configPopup_lineWidthSlider` | OscillatorConfigPopup | Slider | Line width |
| `configPopup_opacitySlider` | OscillatorConfigPopup | Slider | Opacity |
| `configPopup_verticalScaleSlider` | OscillatorConfigPopup | Slider | Vertical scale |
| `configPopup_verticalOffsetSlider` | OscillatorConfigPopup | Slider | Vertical offset |
| `configPopup_deleteBtn` | OscillatorConfigPopup | Button | Delete oscillator |

### Settings Dropdown
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `settingsDropdown` | SettingsDropdown | Component | Settings popup |
| `settingsDropdown_statusBarToggle` | SettingsDropdown | Toggle | Status bar visibility |
| `settingsDropdown_layout1Btn` | SettingsDropdown | Button | 1-column layout |
| `settingsDropdown_layout2Btn` | SettingsDropdown | Button | 2-column layout |
| `settingsDropdown_layout3Btn` | SettingsDropdown | Button | 3-column layout |
| `settingsDropdown_editThemeBtn` | SettingsDropdown | Button | Edit theme button |

### Theme Editor
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `themeEditor` | ThemeEditorComponent | Component | Theme editor modal |
| `themeEditor_closeBtn` | ThemeEditorComponent | Button | Close button |
| `themeEditor_themeList` | ThemeEditorComponent | ListBox | Theme list |
| `themeEditor_themeList_item_{index}` | ThemeEditorComponent | Component | Theme item |
| `themeEditor_createBtn` | ThemeEditorComponent | Button | Create theme |
| `themeEditor_cloneBtn` | ThemeEditorComponent | Button | Clone theme |
| `themeEditor_deleteBtn` | ThemeEditorComponent | Button | Delete theme |
| `themeEditor_exportBtn` | ThemeEditorComponent | Button | Export theme |
| `themeEditor_importBtn` | ThemeEditorComponent | Button | Import theme |
| `themeEditor_nameField` | ThemeEditorComponent | TextEditor | Theme name |
| `themeEditor_colorSwatch_{colorName}` | ThemeEditorComponent | Component | Color swatch |
| `themeEditor_applyBtn` | ThemeEditorComponent | Button | Apply changes |

### Status Bar
| ID | Component | Type | Description |
|----|-----------|------|-------------|
| `statusBar` | StatusBarComponent | Component | Status bar container |
| `statusBar_fpsLabel` | StatusBarComponent | Label | FPS display |
| `statusBar_cpuLabel` | StatusBarComponent | Label | CPU % display |
| `statusBar_memoryLabel` | StatusBarComponent | Label | Memory MB display |
| `statusBar_renderModeLabel` | StatusBarComponent | Label | Rendering mode |
| `statusBar_oscillatorCountLabel` | StatusBarComponent | Label | Oscillator count |
| `statusBar_sourceCountLabel` | StatusBarComponent | Label | Source count |

---

## Usage in Components

### For Single-Instance Components
```cpp
#include "TestableComponent.h"

class MyComponent : public juce::Component
{
public:
    MyComponent()
    {
        REGISTER_TESTABLE("sidebar_timing");
    }

private:
    DECLARE_TESTABLE();
};
```

### For Multi-Instance Components (with index)
```cpp
class OscillatorListItem : public juce::Component
{
public:
    OscillatorListItem(int index)
    {
        REGISTER_TESTABLE(TEST_ID_WITH_INDEX("sidebar_oscillators_item", index));
    }

private:
    DECLARE_TESTABLE();
};
```

### For Child Elements
```cpp
void MyComponent::setupChildren()
{
    addAndMakeVisible(nameField_);
    REGISTER_TESTABLE_CHILD(nameField_, "configPopup_nameField");
}

~MyComponent()
{
    UNREGISTER_TESTABLE_CHILD("configPopup_nameField");
}
```

### For Reusable Components (testId as parameter)
```cpp
class OscilSlider : public juce::Slider
{
public:
    OscilSlider(const juce::String& testId = "")
    {
        if (testId.isNotEmpty())
            REGISTER_TESTABLE(testId);
    }

private:
    DECLARE_TESTABLE();
};

// Usage:
OscilSlider gainSlider_{"sidebar_masterControls_gainSlider"};
```

---

## Querying Elements

### Via HTTP API
```bash
# Get all registered elements
curl http://localhost:8765/ui/state

# Get specific element
curl http://localhost:8765/ui/element/sidebar_timing_intervalSlider

# Click element
curl -X POST http://localhost:8765/ui/click \
  -H "Content-Type: application/json" \
  -d '{"elementId": "toolbar_addOscillatorBtn"}'

# Set slider value
curl -X POST http://localhost:8765/ui/slider \
  -H "Content-Type: application/json" \
  -d '{"elementId": "sidebar_masterControls_gainSlider", "value": -6.0}'
```

### Via Test Scripts
```python
def click_element(element_id):
    response = requests.post(f"{BASE_URL}/ui/click",
        json={"elementId": element_id})
    return response.json()["success"]

# Example
click_element("toolbar_addOscillatorBtn")
```
