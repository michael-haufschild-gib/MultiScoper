# Singleton Removal Migration Plan

## Goal
Remove all singleton patterns and complete the migration to pure Dependency Injection.
No backward compatibility code - clean DI-only architecture.

## Current State Analysis

### Singletons to Remove (Priority Order)

| Singleton | Location | Used By | Priority |
|-----------|----------|---------|----------|
| `ThemeManager::getInstance()` | src/ui/theme/ | PluginFactory, 100+ test usages | P0 |
| `InstanceRegistry::getInstance()` | src/core/ | PluginFactory, test_server, tests | P0 |
| `ShaderRegistry::getInstance()` | src/rendering/ | PluginFactory, tests | P0 |
| `MemoryBudgetManager::getInstance()` | src/core/ | PluginProcessor | P1 |
| `GlobalPreferences::getInstance()` | src/core/ | OscilState | P1 |
| `UIAudioFeedback::getInstance()` | src/ui/ | UI components | P2 |
| `PluginFactory::getInstance()` | src/plugin/ | JUCE entry point | Special |

### JUCE Singletons (Keep - Framework Required)
- `juce::MessageManager::getInstance()` - Thread checking
- `juce::Desktop::getInstance()` - Display/window management

## Architecture Design

### Service Ownership Hierarchy

```
PluginFactory (composition root)
├── owns: ThemeManager (implements IThemeService)
├── owns: InstanceRegistry (implements IInstanceRegistry)
├── owns: ShaderRegistry
├── owns: MemoryBudgetManager (implements IMemoryBudgetService)
└── creates: OscilPluginProcessor
             ├── receives: ServiceContext& (references to all services)
             └── creates: OscilPluginEditor
                          └── receives: ServiceContext&
```

### New Interfaces Needed

1. **IMemoryBudgetService** - Abstract memory budget management
2. **IPreferencesService** - Abstract global preferences (optional, could inline into ServiceContext)

### Updated ServiceContext

```cpp
struct ServiceContext
{
    IInstanceRegistry& instanceRegistry;
    IThemeService& themeService;
    ShaderRegistry& shaderRegistry;
    IMemoryBudgetService& memoryBudgetService;
    // Optional: IPreferencesService& preferences;
};
```

## Implementation Steps

### Phase 1: Core Services (P0)

#### Step 1.1: Update PluginFactory to Own Services
- Create `ThemeManager`, `InstanceRegistry`, `ShaderRegistry` as member variables
- Remove `getInstance()` calls from `createPluginProcessor()`
- Pass references via `ServiceContext`

#### Step 1.2: Remove ThemeManager Singleton
- Remove `getInstance()` method
- Remove `juce::DeletedAtShutdown` inheritance
- Make constructor public
- Update header comment to indicate DI-only

#### Step 1.3: Remove InstanceRegistry Singleton
- Remove `getInstance()` method
- Make constructor public
- Update test_server to receive registry via dependency injection

#### Step 1.4: Remove ShaderRegistry Singleton
- Remove `getInstance()` method
- Make constructor public

### Phase 2: Update Tests

#### Step 2.1: Update OscilTestFixtures.h
- Remove `getThemeManager()` and `getRegistry()` static methods
- Make `OscilPluginTestFixture` own service instances (like `OscilComponentTestFixture`)
- All fixtures use mocks or owned instances

#### Step 2.2: Update Individual Test Files
- Replace `ThemeManager::getInstance()` with fixture-provided service
- Replace `InstanceRegistry::getInstance()` with fixture-provided service
- For tests that need ThemeManager features beyond IThemeService, create helper methods

### Phase 3: Secondary Services (P1)

#### Step 3.1: Create IMemoryBudgetService Interface
- Extract interface from MemoryBudgetManager
- Update ServiceContext to include it
- Remove singleton pattern

#### Step 3.2: Update PluginProcessor
- Remove direct `MemoryBudgetManager::getInstance()` calls
- Use `context.memoryBudgetService`

#### Step 3.3: Handle GlobalPreferences
- Option A: Add to ServiceContext
- Option B: Move to OscilState (it's already accessed via state)

### Phase 4: UI Services (P2)

#### Step 4.1: UIAudioFeedback
- Add to ServiceContext or make it a member of PluginEditor
- Remove singleton pattern

### Phase 5: PluginFactory Special Handling

The `PluginFactory::getInstance()` is special because JUCE's plugin entry point
calls `createPluginInstance()` which needs a factory. Options:

**Option A: Keep Factory Singleton (Recommended)**
- Factory is the composition root - one singleton is acceptable
- It owns all other services
- Clean separation: singleton only at entry point

**Option B: Static Factory Function**
- Replace singleton with static function
- Less flexible for testing but simpler

## File Changes Summary

### Headers to Modify
- `include/core/ServiceContext.h` - Add new service references
- `include/ui/theme/ThemeManager.h` - Remove singleton, DeletedAtShutdown
- `include/core/InstanceRegistry.h` - Remove singleton
- `include/rendering/ShaderRegistry.h` - Remove singleton
- `include/plugin/PluginFactory.h` - Add owned services as members

### Source Files to Modify
- `src/plugin/PluginFactory.cpp` - Own services, remove getInstance calls
- `src/ui/theme/ThemeManager.cpp` - Remove getInstance()
- `src/core/InstanceRegistry.cpp` - Remove getInstance()
- `src/rendering/ShaderRegistry.cpp` - Remove getInstance()
- `src/tools/test_server/*.cpp` - Receive services via DI

### Test Files to Modify (Many)
- `tests/OscilTestFixtures.h` - Own services in fixtures
- All test files using `ThemeManager::getInstance()`
- All test files using `InstanceRegistry::getInstance()`

## Risk Mitigation

1. **Incremental Migration**: Complete Phase 1 first, run all tests, then proceed
2. **Test Coverage**: All 1439 tests must pass after each phase
3. **Build Verification**: VST3/AU validation must pass

## Success Criteria

- [ ] No `getInstance()` calls in src/ (except juce:: and PluginFactory)
- [ ] No `getInstance()` calls in tests/ (all use fixtures)
- [ ] All 1439 tests pass
- [ ] VST3/AU/Standalone builds and validates
- [ ] No memory leaks (leak detectors clean)
