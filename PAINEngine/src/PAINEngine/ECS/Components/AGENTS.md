# ECS Components Directory - Agent Guidelines

## Adding a New Component

When adding a new ECS component to the game engine, follow these steps:

### 1. Create Component Header
- Create `c<ComponentName>.h` in `ECS/Components/`
- Follow the pattern in `cAnimation.h` or `cMeshRenderer.h`
- Include:
  - `#pragma once`
  - `#include "pch.h"`
  - `#include "LayeredSystems/LevelEditor/EditorAttributes.h"`
  - `#include "AssetTypes.h"`

### 2. Define Component Struct
- Add `static constexpr bool ShouldSerialize = true;`
- Add all serializable fields
- Add runtime-only fields (marked as DO NOT SERIALIZE in comments)

### 3. Add Reflection (for Editor UI)
```cpp
REFL_TYPE(PAIN::ComponentName)
REFL_FIELD(field1, PAIN::Editor::Attributes::DisplayName("Label"))
REFL_FIELD(field2, PAIN::Editor::Attributes::Range(min, max))
REFL_FIELD(field3, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Type))
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::ComponentName>);
```

### 4. Register in AllComponents.h
- Add to `AllGameplayComponents` tuple
- Add `getComponentName<T>()` specialization

### 5. Include in Core.h
- Add `#include "ECS/Components/c<ComponentName>.h"`

### 6. Register in Controller.cpp
- Add `registerComponent<ComponentName>("ComponentName");` in `registerAllComponents()`

## Key Patterns

- Use `Assets::GUID` for asset references
- Use `glm::vec3`, `glm::vec4` for vectors
- Use enums for state machines
- Keep runtime state separate from serialized data
