# Feature Requirements: Visual Preset Management UI

**Status**: Draft
**Priority**: High
**Dependencies**: 007_visual_preset_system.md
**Related To**: VisualPresetManager, OscillatorConfigDialog

---

## 1. Overview

Provide a complete user interface for managing visual presets. Users can browse, preview, create, edit, duplicate, rename, delete, import, and export visual presets. The interface must expose ALL rendering engine capabilities (80+ parameters) in an organized, intuitive editor.

### 1.1 Goals

- Enable users to create custom visual styles without coding
- Provide real-time preview while editing
- Organize ~80 parameters into logical, navigable sections
- Support preset organization (favorites, search, categories)
- Maintain consistency with existing Oscil UI patterns

### 1.2 Success Criteria

- User can create a preset from scratch in under 3 minutes
- All rendering parameters are accessible in the editor
- Live preview updates within 100ms of parameter change
- Presets can be shared via export/import

---

## 2. Entry Points

### 2.1 Primary Entry: Preset Browser Button

| Property | Value |
|----------|-------|
| Location | Sidebar, below oscillator list header |
| Component | `OscilButton` with icon |
| Icon | `resources/icomoon/SVG/equalizer.svg` |
| Label | "Presets" |
| TestId | `sidebar_presets_button` |
| Action | Open Preset Browser Dialog |

### 2.2 Secondary Entry: Oscillator Config Dialog

| Property | Value |
|----------|-------|
| Location | Existing "Visual Preset" dropdown in OscillatorConfigDialog |
| Component | `OscilDropdown` + "Edit" button |
| TestId | `oscillator_config_preset_edit_button` |
| Action | Open Preset Editor for selected preset |

### 2.3 Context Menu Entry

| Property | Value |
|----------|-------|
| Location | Right-click on preset in dropdown or browser |
| Options | Edit, Duplicate, Rename, Delete, Export, Toggle Favorite |

---

## 3. Preset Browser Dialog

### 3.1 Dialog Properties

| Property | Value |
|----------|-------|
| Component | `OscilModal` + custom content |
| Size | Large (640x520) |
| Title | "Visual Presets" |
| TestId | `preset_browser_dialog` |

### 3.2 Layout Structure

```
+----------------------------------------------------------+
| Visual Presets                                      [X]   |
+----------------------------------------------------------+
| [+ New]  [Import]  |  [Search: ____________]  [Filter: v]|
+----------------------------------------------------------+
| Categories         | Preset Grid/List                     |
| ----------------   | ------------------------------------ |
| > All Presets (12) | +--------+  +--------+  +--------+  |
|   Favorites (3)    | |        |  |        |  |        |  |
|   System (4)       | | thumb  |  | thumb  |  | thumb  |  |
|   User (8)         | |        |  |        |  |        |  |
|                    | +--------+  +--------+  +--------+  |
|                    | Preset 1    Preset 2    Preset 3    |
|                    |                                      |
|                    | +--------+  +--------+  +--------+  |
|                    | |        |  |        |  |        |  |
|                    | | thumb  |  | thumb  |  | thumb  |  |
|                    | |        |  |        |  |        |  |
|                    | +--------+  +--------+  +--------+  |
|                    | Preset 4    Preset 5    Preset 6    |
+----------------------------------------------------------+
|                                    [Cancel]  [Apply]      |
+----------------------------------------------------------+
```

### 3.3 Category Sidebar

| Item | TestId | Behavior |
|------|--------|----------|
| All Presets | `preset_category_all` | Show all presets |
| Favorites | `preset_category_favorites` | Show favorited presets |
| System | `preset_category_system` | Show built-in presets |
| User | `preset_category_user` | Show user-created presets |

### 3.4 Toolbar Components

| Component | TestId | Action |
|-----------|--------|--------|
| New Button | `preset_browser_new_button` | Open Preset Editor (blank) |
| Import Button | `preset_browser_import_button` | Open file picker for .json |
| Search Field | `preset_browser_search_field` | Filter presets by name |
| Filter Dropdown | `preset_browser_filter_dropdown` | Filter by shader type |

### 3.5 Preset Card Component

| Property | Value |
|----------|-------|
| Size | 120x100 pixels |
| Thumbnail | 120x80 preview image |
| Name | Below thumbnail, ellipsis if too long |
| Favorite Icon | Star in top-right corner |
| System Badge | Lock icon for read-only presets |
| Hover | Slight scale up (1.02), show action overlay |
| Click | Select preset |
| Double-click | Apply and close dialog |
| Right-click | Context menu |

### 3.6 Preset Card Actions (Hover Overlay)

| Button | Icon | TestId | Condition |
|--------|------|--------|-----------|
| Edit | `pencil.svg` | `preset_card_edit_{id}` | Always |
| Duplicate | `copy.svg` | `preset_card_duplicate_{id}` | Always |
| Delete | `bin.svg` | `preset_card_delete_{id}` | User presets only |
| Export | `upload.svg` | `preset_card_export_{id}` | Always |
| Favorite | `star.svg` (filled/empty) | `preset_card_favorite_{id}` | Always |

### 3.7 Filter Options

| Option | Value |
|--------|-------|
| All Types | Show all |
| 2D Shaders | Basic2D, NeonGlow, GradientFill, DualOutline, PlasmaSine, DigitalGlitch |
| 3D Shaders | VolumetricRibbon, WireframeMesh, VectorFlow, StringTheory, ElectricFlower, ElectricFiligree |
| Materials | GlassRefraction, LiquidChrome, Crystalline |

---

## 4. Preset Editor Dialog

### 4.1 Dialog Properties

| Property | Value |
|----------|-------|
| Component | `OscilModal` + custom content |
| Size | FullScreen or 900x700 (configurable) |
| Title | "Edit Preset: {name}" or "New Preset" |
| TestId | `preset_editor_dialog` |

### 4.2 Overall Layout

```
+--------------------------------------------------------------------------+
| Edit Preset: Neon Glow                                              [X]  |
+--------------------------------------------------------------------------+
| Name: [Neon Glow___________]  [Favorite *]                               |
| Description: [Optional description_______________________]               |
+--------------------------------------------------------------------------+
| +-------------------------+  +----------------------------------------+  |
| |                         |  |  TABS: [Shader] [Effects] [Particles]  |  |
| |                         |  |        [3D/Camera] [Materials]         |  |
| |      LIVE PREVIEW       |  +----------------------------------------+  |
| |                         |  |                                        |  |
| |    (Waveform renders    |  |  === SHADER SELECTION ===              |  |
| |     with current        |  |                                        |  |
| |     settings)           |  |  Type: [NeonGlow          v]           |  |
| |                         |  |                                        |  |
| |                         |  |  === COMPOSITING ===                   |  |
| |                         |  |                                        |  |
| |                         |  |  Blend Mode: [Additive     v]          |  |
| |                         |  |  Opacity: [=======|===] 1.0            |  |
| |                         |  |                                        |  |
| +-------------------------+  +----------------------------------------+  |
+--------------------------------------------------------------------------+
|                     [Reset to Default]  [Cancel]  [Save Preset]          |
+--------------------------------------------------------------------------+
```

### 4.3 Header Section

| Field | Component | TestId | Validation |
|-------|-----------|--------|------------|
| Name | `OscilTextField` | `preset_editor_name_field` | 1-64 chars, unique |
| Description | `OscilTextField` | `preset_editor_description_field` | 0-256 chars |
| Favorite | `OscilToggle` | `preset_editor_favorite_toggle` | - |

### 4.4 Live Preview Panel

| Property | Value |
|----------|-------|
| Size | 320x240 minimum, flexible |
| Content | Animated waveform with current preset applied |
| Audio Source | Synthetic sine wave (440Hz) or silent with generated pattern |
| Update Rate | Real-time (60fps), debounced by 50ms for parameter changes |
| Border | 1px theme.borderColor |
| Background | Matches preset background or theme.panelBackground |

### 4.5 Tab Navigation

| Tab | TestId | Contents |
|-----|--------|----------|
| Shader | `preset_tab_shader` | Shader type, blend mode, opacity |
| Effects | `preset_tab_effects` | All 11 post-processing effects |
| Particles | `preset_tab_particles` | Particle system settings |
| 3D/Camera | `preset_tab_3d` | 3D settings, lighting |
| Materials | `preset_tab_materials` | Material properties |

---

## 5. Tab Contents

### 5.1 Shader Tab

#### 5.1.1 Shader Selection Section

| Field | Component | TestId | Options |
|-------|-----------|--------|---------|
| Shader Type | `OscilDropdown` | `preset_shader_type_dropdown` | All 15 ShaderType enum values |

Display shader options in groups:
- **2D Shaders**: Basic2D, NeonGlow, GradientFill, DualOutline, PlasmaSine, DigitalGlitch
- **3D Shaders**: VolumetricRibbon, WireframeMesh, VectorFlow, StringTheory, ElectricFlower, ElectricFiligree
- **Material Shaders**: GlassRefraction, LiquidChrome, Crystalline

#### 5.1.2 Compositing Section

| Field | Component | TestId | Range/Options |
|-------|-----------|--------|---------------|
| Blend Mode | `OscilDropdown` | `preset_blend_mode_dropdown` | Alpha, Additive, Multiply, Screen |
| Opacity | `OscilSlider` | `preset_opacity_slider` | 0.0 - 1.0, default 1.0 |

### 5.2 Effects Tab

Organize as accordion sections, each expandable:

#### 5.2.1 Effects Accordion Structure

```
+------------------------------------------+
| [v] Bloom                           [ON] |
+------------------------------------------+
|   Intensity: [====|====] 1.0             |
|   Threshold: [======|==] 0.8             |
|   Iterations: [==|======] 4              |
|   Downsample: [===|=====] 6              |
|   Spread: [====|====] 1.0                |
|   Soft Knee: [===|=====] 0.5             |
+------------------------------------------+
| [>] Trails                         [OFF] |
+------------------------------------------+
| [>] Color Grade                    [OFF] |
+------------------------------------------+
| ... (remaining effects) ...              |
+------------------------------------------+
```

#### 5.2.2 Effect Sections

Each effect section contains:
- Enable toggle in header (right side)
- Accordion expand/collapse (left chevron)
- All effect parameters as form fields when expanded
- Disabled appearance when toggle is OFF

| Effect | Section TestId | Enable TestId |
|--------|----------------|---------------|
| Bloom | `preset_effect_bloom_section` | `preset_effect_bloom_enabled` |
| Radial Blur | `preset_effect_radial_blur_section` | `preset_effect_radial_blur_enabled` |
| Trails | `preset_effect_trails_section` | `preset_effect_trails_enabled` |
| Color Grade | `preset_effect_color_grade_section` | `preset_effect_color_grade_enabled` |
| Vignette | `preset_effect_vignette_section` | `preset_effect_vignette_enabled` |
| Film Grain | `preset_effect_film_grain_section` | `preset_effect_film_grain_enabled` |
| Chromatic Aberration | `preset_effect_chromatic_aberration_section` | `preset_effect_chromatic_aberration_enabled` |
| Scanlines | `preset_effect_scanlines_section` | `preset_effect_scanlines_enabled` |
| Distortion | `preset_effect_distortion_section` | `preset_effect_distortion_enabled` |
| Glitch | `preset_effect_glitch_section` | `preset_effect_glitch_enabled` |
| Tilt Shift | `preset_effect_tilt_shift_section` | `preset_effect_tilt_shift_enabled` |

#### 5.2.3 Bloom Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Intensity | `OscilSlider` | `preset_bloom_intensity` | 0.0 - 2.0 |
| Threshold | `OscilSlider` | `preset_bloom_threshold` | 0.0 - 1.0 |
| Iterations | `OscilSlider` (int) | `preset_bloom_iterations` | 2 - 8 |
| Downsample Steps | `OscilSlider` (int) | `preset_bloom_downsample` | 2 - 8 |
| Spread | `OscilSlider` | `preset_bloom_spread` | 0.5 - 3.0 |
| Soft Knee | `OscilSlider` | `preset_bloom_soft_knee` | 0.0 - 1.0 |

#### 5.2.4 Radial Blur Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Amount | `OscilSlider` | `preset_radial_blur_amount` | 0.0 - 0.5 |
| Glow | `OscilSlider` | `preset_radial_blur_glow` | 0.0 - 2.0 |
| Samples | `OscilSlider` (int) | `preset_radial_blur_samples` | 2 - 8 |

#### 5.2.5 Trails Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Decay | `OscilSlider` | `preset_trails_decay` | 0.01 - 0.5 |
| Opacity | `OscilSlider` | `preset_trails_opacity` | 0.0 - 1.0 |

#### 5.2.6 Color Grade Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Brightness | `OscilSlider` | `preset_color_brightness` | -1.0 - 1.0 |
| Contrast | `OscilSlider` | `preset_color_contrast` | 0.5 - 2.0 |
| Saturation | `OscilSlider` | `preset_color_saturation` | 0.0 - 2.0 |
| Temperature | `OscilSlider` | `preset_color_temperature` | -1.0 - 1.0 |
| Tint | `OscilSlider` | `preset_color_tint` | -1.0 - 1.0 |
| Shadows | `OscilColorPicker` button | `preset_color_shadows` | ARGB |
| Highlights | `OscilColorPicker` button | `preset_color_highlights` | ARGB |

#### 5.2.7 Vignette Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Intensity | `OscilSlider` | `preset_vignette_intensity` | 0.0 - 1.0 |
| Softness | `OscilSlider` | `preset_vignette_softness` | 0.0 - 1.0 |
| Color | `OscilColorPicker` button | `preset_vignette_color` | ARGB |

#### 5.2.8 Film Grain Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Intensity | `OscilSlider` | `preset_grain_intensity` | 0.0 - 0.5 |
| Speed | `OscilSlider` | `preset_grain_speed` | 1.0 - 60.0 |

#### 5.2.9 Chromatic Aberration Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Intensity | `OscilSlider` | `preset_chromatic_intensity` | 0.0 - 0.02 |

#### 5.2.10 Scanlines Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Intensity | `OscilSlider` | `preset_scanlines_intensity` | 0.0 - 1.0 |
| Density | `OscilSlider` | `preset_scanlines_density` | 0.5 - 4.0 |
| Phosphor Glow | `OscilToggle` | `preset_scanlines_phosphor` | boolean |

#### 5.2.11 Distortion Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Intensity | `OscilSlider` | `preset_distortion_intensity` | 0.0 - 0.5 |
| Frequency | `OscilSlider` | `preset_distortion_frequency` | 1.0 - 20.0 |
| Speed | `OscilSlider` | `preset_distortion_speed` | 0.1 - 5.0 |

#### 5.2.12 Glitch Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Intensity | `OscilSlider` | `preset_glitch_intensity` | 0.0 - 1.0 |
| Block Size | `OscilSlider` | `preset_glitch_block_size` | 0.01 - 0.2 |
| Line Shift | `OscilSlider` | `preset_glitch_line_shift` | 0.0 - 0.1 |
| Color Separation | `OscilSlider` | `preset_glitch_color_sep` | 0.0 - 0.05 |
| Flicker Rate | `OscilSlider` | `preset_glitch_flicker` | 1.0 - 30.0 |

#### 5.2.13 Tilt Shift Parameters

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Position | `OscilSlider` | `preset_tilt_position` | 0.0 - 1.0 |
| Range | `OscilSlider` | `preset_tilt_range` | 0.0 - 1.0 |
| Blur Radius | `OscilSlider` | `preset_tilt_blur` | 0.0 - 10.0 |
| Iterations | `OscilSlider` (int) | `preset_tilt_iterations` | 1 - 8 |

### 5.3 Particles Tab

#### 5.3.1 Enable Section

| Field | Component | TestId |
|-------|-----------|--------|
| Enable Particles | `OscilToggle` | `preset_particles_enabled` |

When disabled, entire tab content is dimmed/disabled.

#### 5.3.2 Emission Section

| Parameter | Component | TestId | Options/Range |
|-----------|-----------|--------|---------------|
| Emission Mode | `OscilDropdown` | `preset_particles_mode` | AlongWaveform, AtPeaks, AtZeroCrossings, Continuous, Burst |
| Emission Rate | `OscilSlider` | `preset_particles_rate` | 1.0 - 1000.0 |
| Particle Life | `OscilSlider` | `preset_particles_life` | 0.1 - 10.0 |
| Particle Size | `OscilSlider` | `preset_particles_size` | 1.0 - 32.0 |
| Particle Color | `OscilColorPicker` button | `preset_particles_color` | ARGB |
| Blend Mode | `OscilDropdown` | `preset_particles_blend` | Additive, Alpha, Multiply, Screen |

#### 5.3.3 Physics Section

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Gravity | `OscilSlider` | `preset_particles_gravity` | -100.0 - 100.0 |
| Drag | `OscilSlider` | `preset_particles_drag` | 0.0 - 1.0 |
| Randomness | `OscilSlider` | `preset_particles_randomness` | 0.0 - 1.0 |
| Velocity Scale | `OscilSlider` | `preset_particles_velocity` | 0.0 - 5.0 |

#### 5.3.4 Audio Reactivity Section

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Audio Reactive | `OscilToggle` | `preset_particles_audio_reactive` | boolean |
| Emission Boost | `OscilSlider` | `preset_particles_audio_boost` | 1.0 - 10.0 |

#### 5.3.5 Texture Section

| Parameter | Component | TestId | Options/Range |
|-----------|-----------|--------|---------------|
| Texture | `OscilDropdown` | `preset_particles_texture` | "(none)", "sparkle", "dot", "star", etc. |
| Texture Rows | `OscilSlider` (int) | `preset_particles_tex_rows` | 1 - 16 |
| Texture Columns | `OscilSlider` (int) | `preset_particles_tex_cols` | 1 - 16 |

#### 5.3.6 Advanced Section (Collapsible)

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Soft Particles | `OscilToggle` | `preset_particles_soft` | boolean |
| Depth Sensitivity | `OscilSlider` | `preset_particles_soft_depth` | 0.1 - 5.0 |
| Use Turbulence | `OscilToggle` | `preset_particles_turbulence` | boolean |
| Turbulence Strength | `OscilSlider` | `preset_particles_turb_strength` | 0.0 - 10.0 |
| Turbulence Scale | `OscilSlider` | `preset_particles_turb_scale` | 0.1 - 2.0 |
| Turbulence Speed | `OscilSlider` | `preset_particles_turb_speed` | 0.1 - 2.0 |

### 5.4 3D/Camera Tab

#### 5.4.1 Enable Section

| Field | Component | TestId |
|-------|-----------|--------|
| Enable 3D | `OscilToggle` | `preset_3d_enabled` |

Note: When ShaderType is a 3D or Material shader, this is automatically enabled and locked.

#### 5.4.2 Camera Section

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Camera Distance | `OscilSlider` | `preset_3d_distance` | 1.0 - 20.0 |
| Camera Pitch (X) | `OscilSlider` | `preset_3d_angle_x` | -90.0 - 90.0 |
| Camera Yaw (Y) | `OscilSlider` | `preset_3d_angle_y` | 0.0 - 360.0 |
| Auto Rotate | `OscilToggle` | `preset_3d_auto_rotate` | boolean |
| Rotation Speed | `OscilSlider` | `preset_3d_rotate_speed` | 0.0 - 60.0 |

#### 5.4.3 Mesh Section

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Resolution X | `OscilSlider` (int) | `preset_3d_mesh_res_x` | 16 - 512 |
| Resolution Z | `OscilSlider` (int) | `preset_3d_mesh_res_z` | 8 - 128 |
| Mesh Scale | `OscilSlider` | `preset_3d_mesh_scale` | 0.1 - 5.0 |

#### 5.4.4 Lighting Section

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Light Direction X | `OscilSlider` | `preset_light_dir_x` | -1.0 - 1.0 |
| Light Direction Y | `OscilSlider` | `preset_light_dir_y` | -1.0 - 1.0 |
| Light Direction Z | `OscilSlider` | `preset_light_dir_z` | -1.0 - 1.0 |
| Ambient Color | `OscilColorPicker` button | `preset_light_ambient` | RGB |
| Diffuse Color | `OscilColorPicker` button | `preset_light_diffuse` | RGB |
| Specular Color | `OscilColorPicker` button | `preset_light_specular` | RGB |
| Specular Power | `OscilSlider` | `preset_light_spec_power` | 1.0 - 256.0 |
| Specular Intensity | `OscilSlider` | `preset_light_spec_intensity` | 0.0 - 2.0 |

### 5.5 Materials Tab

#### 5.5.1 Enable Section

| Field | Component | TestId |
|-------|-----------|--------|
| Enable Materials | `OscilToggle` | `preset_material_enabled` |

Note: When ShaderType is a Material shader, this is automatically enabled and locked.

#### 5.5.2 Surface Properties Section

| Parameter | Component | TestId | Range |
|-----------|-----------|--------|-------|
| Reflectivity | `OscilSlider` | `preset_material_reflectivity` | 0.0 - 1.0 |
| Refractive Index | `OscilSlider` | `preset_material_ior` | 1.0 - 3.0 |
| Fresnel Power | `OscilSlider` | `preset_material_fresnel` | 0.5 - 5.0 |
| Roughness | `OscilSlider` | `preset_material_roughness` | 0.0 - 1.0 |
| Tint Color | `OscilColorPicker` button | `preset_material_tint` | ARGB |

#### 5.5.3 Environment Section

| Parameter | Component | TestId | Options |
|-----------|-----------|--------|---------|
| Use Environment Map | `OscilToggle` | `preset_material_use_env` | boolean |
| Environment Map | `OscilDropdown` | `preset_material_env_map` | "default_studio", "sunset", "night", "abstract" |

---

## 6. User Flows

### 6.1 Create New Preset

```
USER FLOW: Create New Preset
PRECONDITION: User is in plugin UI
POSTCONDITION: New preset saved and available

STEPS:
1. User clicks "Presets" button in sidebar
   → Preset Browser Dialog opens
   → Shows all presets in grid view

2. User clicks "+ New" button
   → Preset Editor Dialog opens
   → Title: "New Preset"
   → All fields at default values
   → Live preview shows default shader

3. User enters preset name
   → VALIDATION: 1-64 chars, unique, no reserved names
   → Error shown inline if invalid

4. User selects shader type from dropdown
   → Live preview updates immediately
   → If 3D shader selected, 3D tab becomes relevant
   → If Material shader, Materials tab becomes relevant

5. User switches to Effects tab
   → Accordion sections for all 11 effects
   → User expands "Bloom" section
   → User toggles Bloom ON
   → User adjusts Intensity slider
   → Live preview updates with bloom

6. User configures additional effects as desired
   → Each change updates live preview

7. User clicks "Save Preset" button
   → VALIDATION: Name not empty, name unique
   → IF VALID: Preset saved to user/ directory
   → Success toast: "Preset '{name}' saved"
   → Dialog closes
   → Preset Browser refreshes (if open)

8. User can now select preset in oscillator config
   → Preset appears in dropdown under "User" section
```

### 6.2 Edit Existing Preset

```
USER FLOW: Edit Existing Preset
PRECONDITION: Preset exists, user can edit (not system preset)
POSTCONDITION: Preset updated

STEPS:
1. User opens Preset Browser

2. User locates preset to edit
   → Via category filter, search, or scrolling

3. User hovers over preset card
   → Action overlay appears

4. User clicks Edit (pencil) button
   → Preset Editor Dialog opens
   → Title: "Edit Preset: {name}"
   → All fields populated with preset values
   → Live preview shows preset

5. User modifies settings
   → Any tabs, any parameters
   → Live preview updates

6. User clicks "Save Preset"
   → Existing file overwritten
   → Success toast
   → modifiedAt timestamp updated
```

### 6.3 Duplicate Preset

```
USER FLOW: Duplicate Preset
PRECONDITION: Preset exists
POSTCONDITION: New preset created as copy

STEPS:
1. User hovers over preset card in Browser

2. User clicks Duplicate (copy) button
   → Prompt dialog: "Enter name for duplicate"
   → Default: "{original name} Copy"

3. User enters name or accepts default

4. User clicks OK
   → New preset created with all same settings
   → New UUID assigned
   → createdAt = now
   → Category = User (even if duplicating System preset)
   → Success toast
   → Browser refreshes, shows new preset
```

### 6.4 Delete Preset

```
USER FLOW: Delete Preset
PRECONDITION: User preset exists
POSTCONDITION: Preset removed

STEPS:
1. User hovers over USER preset card
   → Delete button visible (not shown for System presets)

2. User clicks Delete (bin) button
   → Confirmation dialog: "Delete preset '{name}'? This cannot be undone."
   → [Cancel] [Delete] buttons

3. User clicks Delete
   → Preset file deleted from disk
   → metadata.json updated
   → IF any oscillator uses this preset:
     → Oscillator.visualPresetId set to "default"
     → Warning toast: "1 oscillator(s) reset to default"
   → Success toast: "Preset deleted"
   → Browser refreshes
```

### 6.5 Import Preset

```
USER FLOW: Import Preset
PRECONDITION: User has .json preset file
POSTCONDITION: Preset imported to User category

STEPS:
1. User clicks "Import" in Preset Browser

2. System file picker opens
   → Filter: *.json files only

3. User selects file and clicks Open

4. System validates file
   → IF INVALID JSON: Error dialog "Invalid preset file"
   → IF SCHEMA MISMATCH: Error dialog "Incompatible preset version"
   → IF NAME CONFLICT: Prompt "Preset '{name}' exists. Rename to?"
     → User enters new name or cancels

5. Preset copied to user/ directory
   → UUID regenerated (prevent duplicates)
   → Category set to User
   → Success toast: "Preset imported"
   → Browser refreshes
```

### 6.6 Export Preset

```
USER FLOW: Export Preset
PRECONDITION: Preset exists
POSTCONDITION: Preset saved to user-chosen location

STEPS:
1. User hovers over preset card

2. User clicks Export (upload) button

3. System save dialog opens
   → Default filename: "{preset_name}.json"
   → Filter: *.json

4. User chooses location and clicks Save

5. Preset JSON written to file
   → Success toast: "Preset exported to {path}"
```

### 6.7 Toggle Favorite

```
USER FLOW: Toggle Favorite
STEPS:
1. User clicks star icon on preset card
   → IF currently not favorite: Star fills, preset added to favorites
   → IF currently favorite: Star empties, preset removed from favorites
2. metadata.json updated immediately
3. IF in Favorites category view: List refreshes
```

### 6.8 Apply Preset to Oscillator

```
USER FLOW: Apply Preset from Browser
STEPS:
1. User selects preset in Browser (single click)
   → Preset highlighted

2. User clicks "Apply" button
   → IF oscillator selected in sidebar:
     → Oscillator.visualPresetId updated
     → Waveform re-renders with preset
     → Dialog closes
   → IF no oscillator selected:
     → Info toast: "Select an oscillator first"
     → Dialog stays open

ALTERNATIVE: Double-click preset
   → Same as select + Apply
```

---

## 7. Validation & Error Handling

### 7.1 Validation Rules

| Field | Rules | Error Message |
|-------|-------|---------------|
| Preset Name | Required, 1-64 chars | "Name is required" / "Name too long (max 64 characters)" |
| Preset Name | Alphanumeric, space, hyphen, underscore | "Name contains invalid characters" |
| Preset Name | Not "default", "none", "new" | "Reserved name cannot be used" |
| Preset Name | Unique in category | "A preset with this name already exists" |
| Description | Max 256 chars | "Description too long (max 256 characters)" |
| Numeric params | Within defined min/max | "{param} must be between {min} and {max}" |

### 7.2 Error Display

| Context | Display Method |
|---------|----------------|
| Field validation | Red border on field + error text below |
| Form validation | Error banner at top of form |
| File operations | Toast notification (error variant) |
| Confirmation | Modal dialog |

### 7.3 Unsaved Changes

When user tries to close editor with unsaved changes:

```
Dialog: "Unsaved Changes"
Message: "You have unsaved changes. Save before closing?"
Buttons: [Don't Save] [Cancel] [Save]
```

---

## 8. Keyboard Navigation

### 8.1 Browser Dialog

| Key | Action |
|-----|--------|
| Arrow keys | Navigate preset grid |
| Enter | Apply selected preset |
| Delete | Delete selected preset (if user preset) |
| Escape | Close dialog |
| Ctrl/Cmd + N | New preset |
| Ctrl/Cmd + F | Focus search field |

### 8.2 Editor Dialog

| Key | Action |
|-----|--------|
| Tab | Navigate between fields |
| Shift + Tab | Navigate backwards |
| Ctrl/Cmd + S | Save preset |
| Escape | Close (with unsaved changes check) |
| Ctrl/Cmd + Z | Undo last change |
| Ctrl/Cmd + Shift + Z | Redo |

---

## 9. Accessibility

### 9.1 Screen Reader Support

- All form fields have associated labels
- Preset cards announce: "{name}, {category} preset, {favorite status}"
- Tab panels use ARIA tablist pattern
- Accordion sections use ARIA expanded state
- Color pickers announce selected color values

### 9.2 Reduced Motion

- When system prefers reduced motion:
  - Disable live preview animation
  - Instant state changes (no spring animations)
  - Static thumbnail instead of animated preview

### 9.3 Color Contrast

- All text meets WCAG AA contrast (4.5:1)
- Error states use red + icon (not color alone)
- Focus rings visible on all interactive elements

---

## 10. Component Specifications

### 10.1 Preset Card Dimensions

| Property | Value |
|----------|-------|
| Card width | 120px |
| Card height | 100px |
| Thumbnail height | 80px |
| Name label height | 20px |
| Grid gap | 12px |
| Cards per row | Dynamic (floor((container_width - 24) / 132)) |

### 10.2 Editor Panel Dimensions

| Property | Value |
|----------|-------|
| Preview panel | 320px min-width, flexible height |
| Settings panel | 480px min-width |
| Tab bar height | 40px |
| Accordion header | 36px |
| Field row height | 32px |
| Section spacing | 16px |

### 10.3 Slider Specifications

| Property | Value |
|----------|-------|
| Track height | 4px |
| Thumb size | 16px |
| Label width | 80px (right-aligned) |
| Value display | Right of slider, 60px width |

---

## 11. Files to Create/Modify

### 11.1 New UI Files

| File | Purpose |
|------|---------|
| `include/ui/dialogs/PresetBrowserDialog.h` | Browser dialog header |
| `src/ui/dialogs/PresetBrowserDialog.cpp` | Browser implementation |
| `include/ui/dialogs/PresetEditorDialog.h` | Editor dialog header |
| `src/ui/dialogs/PresetEditorDialog.cpp` | Editor implementation |
| `include/ui/components/PresetCard.h` | Preset card component |
| `src/ui/components/PresetCard.cpp` | Card implementation |
| `include/ui/components/EffectAccordion.h` | Effects accordion |
| `src/ui/components/EffectAccordion.cpp` | Accordion implementation |
| `include/ui/components/PresetPreviewPanel.h` | Live preview component |
| `src/ui/components/PresetPreviewPanel.cpp` | Preview implementation |

### 11.2 Modified Files

| File | Changes |
|------|---------|
| `src/ui/layout/SidebarComponent.cpp` | Add Presets button |
| `src/ui/dialogs/OscillatorConfigDialog.cpp` | Add Edit button next to preset dropdown |
| `src/ui/managers/DialogManager.cpp` | Add preset dialog management |
| `include/ui/managers/DialogManager.h` | Add preset dialog methods |

---

## 12. Test IDs Summary

All interactive elements must have TestId for E2E testing:

| Category | Pattern |
|----------|---------|
| Browser buttons | `preset_browser_{action}_button` |
| Browser categories | `preset_category_{name}` |
| Preset cards | `preset_card_{id}` |
| Card actions | `preset_card_{action}_{id}` |
| Editor fields | `preset_editor_{field}` |
| Tabs | `preset_tab_{name}` |
| Effect sections | `preset_effect_{name}_section` |
| Effect enables | `preset_effect_{name}_enabled` |
| Effect params | `preset_{effect}_{param}` |
| Particle params | `preset_particles_{param}` |
| 3D params | `preset_3d_{param}` |
| Material params | `preset_material_{param}` |
| Lighting params | `preset_light_{param}` |

---

## 13. Quality Gate Checklist

Before marking this feature complete, verify:

### 13.1 Browser Functionality
- [ ] All presets display in grid
- [ ] Category filtering works
- [ ] Search filters by name
- [ ] Shader type filter works
- [ ] Double-click applies preset
- [ ] Context menu shows all options
- [ ] System presets show lock badge
- [ ] Favorites show star
- [ ] Hover overlay shows action buttons

### 13.2 Editor Functionality
- [ ] All 15 shader types selectable
- [ ] All 11 effects have accordion sections
- [ ] All effect parameters exposed
- [ ] All particle parameters exposed
- [ ] All 3D/camera parameters exposed
- [ ] All material parameters exposed
- [ ] All lighting parameters exposed
- [ ] Live preview updates in real-time
- [ ] Save creates/updates file correctly
- [ ] Unsaved changes prompt works

### 13.3 CRUD Operations
- [ ] Create new preset works
- [ ] Edit user preset works
- [ ] System presets read-only
- [ ] Duplicate creates new file
- [ ] Delete removes file and updates metadata
- [ ] Import validates and copies file
- [ ] Export writes valid JSON
- [ ] Favorite toggle persists

### 13.4 Validation
- [ ] Name validation shows errors
- [ ] Duplicate name rejected
- [ ] Reserved names rejected
- [ ] Parameter ranges enforced
- [ ] Invalid file import rejected

### 13.5 Accessibility
- [ ] All fields keyboard navigable
- [ ] Screen reader announces correctly
- [ ] Reduced motion respected
- [ ] Focus visible on all elements

### 13.6 Performance
- [ ] Browser loads in < 200ms
- [ ] Live preview at 60fps
- [ ] Parameter change to preview < 100ms
- [ ] Save completes in < 100ms
