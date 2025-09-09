Precompiled headers - pch.h

all c++ files in `src` directory
all shader files in `shaders` directory

### Naming conventions
- Variables snake_case (e.g int bryan_soh)
- Functions, ctor, dtor DamelCase (e.g int bryanLim () {})
- Classes, Namespaces PascalCase (e.g class SeanGwee) 
- File names in snake case (e.g bryan_soh.cpp), unless file is a class definition/declaration, in which case, use PascalCase (e.g SeanGwee.h)
- const, constexpr, enum, typedef define SNAKE_CASE  (e.g const int BRYAN_SOH)

#### GLSL
- variables snake case
- `a` prefix for vertex attribute
- `f` prefix for fragment attribute
- `u` prefix for uniform attribute

##### General practices (graphics)
vert:

- location 0 -> vec2 position
- location 1 -> vec4 color
- location 2 -> vec2 texture coordinates

frag:
- location 0 -> vec4 color
- location 1 -> vec2 texture coordinates

### General practices

- Usage of string references for every asset/state for simplicity and ease of understanding
- Automated garbage collection at program end to prevent memory leaks

### Debugging
- Use `cout` and `cerr` without the `std::` prefix - logic in place to stop printing in release mode for performance 

### Other Notes
- When pushing, do not push warnings and memory leaks, if need help do ask



### Useful info
- delta time - `GLManager::dt`