# AGENTS.md - Coding Agent Guidelines for RecoilEngine

This document provides essential information for AI coding agents working on the RecoilEngine codebase.

## Project Overview

RecoilEngine is an open-source real-time strategy (RTS) game engine written in C++23. It is a fork and continuation of the Spring RTS engine (version 105.0).

## Build Commands

### Building the Engine

**Using Docker (Recommended):**
```bash
# Build for Linux
docker-build-v2/build.sh linux

# Build for Windows
docker-build-v2/build.sh windows

# Build with custom CMake options
docker-build-v2/build.sh linux -DCMAKE_BUILD_TYPE=DEBUG
```

**Without Docker:**
```bash
# Create build directory
mkdir -p build && cd build

# Configure
cmake ..

# Build specific target
cmake --build . --target engine-headless -j$(nproc)

# Build all
cmake --build . -j$(nproc)
```

### Build Types
- `DEBUG` - Debug build with full symbols and no optimization
- `RELEASE` - Optimized release build
- `RELWITHDEBINFO` - Release with debug info (default)
- `PROFILE` - Profiling build

### Build Targets
- `engine-legacy` - Main engine build
- `engine-headless` - Headless server build
- `engine-dedicated` - Dedicated server build
- `tests` - Build all test executables
- `check` - Build and run all tests
- `spring-content` - Build game content packages

## Testing

### Test Framework
The project uses **Catch2** for unit testing. Test files are located in the `test/` directory.

### Running Tests

**Build and run all tests:**
```bash
# From build directory
make tests        # Build all test executables
make check        # Build and run all tests via CTest
make test         # Alternative: run via CTest
```

**Run a single test:**
```bash
# Tests are built as executable binaries in the build directory
# Pattern: test_<TestName>

# Run specific test executable
./test_Float3
./test_Matrix44f
./test_SyncedPrimitive
./test_UDPListener

# Run with verbose output
./test_Float3 -s

# Run specific test case
./test_Float3 "TestSection"
```

**Run via CTest:**
```bash
# Run specific test by name
ctest -R Float3 -V

# Run with regex pattern
ctest -R Matrix -V

# List all available tests
ctest -N
```

### Test Locations
- Unit tests: `test/engine/`
- Test sources use `#include <catch_amalgamated.hpp>`
- Each test is compiled as a separate executable named `test_<TestName>`

## Code Style Guidelines

### Indentation and Formatting
- **Use tabs for indentation** (see `.editorconfig`)
- Tab width: configure your editor to display tabs as you prefer
- Line endings: platform-appropriate (LF on Linux, CRLF on Windows)

### File Headers
All source files must begin with the GPL license header:
```cpp
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
```

### Include Guards
Use `#pragma once` for new headers, migrate ifdef guards to `#pragma once` whenever you edit a header file.

### Naming Conventions

**Classes and Structs:**
- PascalCase: `Button`, `GameVersion`, `RectangleOverlapHandler`
- Descriptive names: avoid abbreviations unless widely known

**Functions and Methods:**
- PascalCase for public methods: `DrawSelf()`, `HandleEvent()`
- Getters/setters: `GetMajor()`, `Label()`

**Variables:**
- Member variables: camelCase or snake_case: `label`, `clicked`, `hovered`
- Local variables: camelCase preferred
- Boolean variables: often use `is`, `has` prefixes or simple names: `Hovered`, `Clicked`

**Constants:**
- Compile-time constants: UPPER_CASE or PascalCase
- const variables: camelCase

**Namespaces:**
- lowercase: `agui`, `spring`, `springversion`

**Macros:**
- UPPER_CASE with underscores: `LOG_LEVEL_DEBUG`, `CR_DECLARE_STRUCT`

### Braces and Formatting

**Functions:**
```cpp
// Opening brace on new line
void ClassName::MethodName()
{
	// body
}
```

**Control structures:**
```cpp
// Opening brace on same line
if (condition) {
	// body
} else {
	// body
}

for (int i = 0; i < n; ++i) {
	// body
}

switch (value) {
	case CASE_ONE:
		// handle
		break;
	default:
		// handle
}
```

**Classes:**
```cpp
class ClassName : public BaseClass
{
public:
	ClassName();
	virtual ~ClassName();

	void PublicMethod();

private:
	void privateMethod();

	int memberVariable;
};
```

### Imports and Includes

**Order:**
1. Corresponding header (for .cpp files)
2. System/STL headers (angle brackets)
3. Library headers (angle brackets)
4. Project headers (quotes)

**Example:**
```cpp
#include "ClassName.h"              // Corresponding header

#include <cassert>                  // System headers
#include <string>
#include <vector>

#include <SDL2/SDL.h>              // Library headers

#include "System/Log/ILog.h"       // Project headers
#include "Rendering/GL/myGL.h"
```

**Include paths:**
- Use relative paths from `rts/` directory
- Example: `#include "System/float3.h"`

### Error Handling

**Assertions:**
```cpp
#include <cassert>

assert(pointer != nullptr);
assert(index >= 0 && index < size);
```

**Logging:**
Use the built-in logging system with severity levels:
```cpp
#include "System/Log/ILog.h"

LOG_L(L_DEBUG, "Debug message: %s", value);
LOG_L(L_INFO, "Info message");
LOG_L(L_WARNING, "Warning: %d items", count);
LOG_L(L_ERROR, "Error occurred in %s", functionName);
LOG_L(L_FATAL, "Fatal error - cannot continue");
```

Log levels (defined in `System/Log/Level.h`):
- `L_DEBUG` - Fine-grained debug info
- `L_INFO` - General information
- `L_NOTICE` - Always outputted (default level)
- `L_DEPRECATED` - Deprecation warnings
- `L_WARNING` - Potentially harmful situations
- `L_ERROR` - Errors that allow continued execution
- `L_FATAL` - Severe errors causing abort

**Exceptions:**
```cpp
#include <stdexcept>

throw std::runtime_error("Description of error");
```

Custom exceptions may be defined in specific modules.

### Comments

**File-level:**
- GPL license header (required)
- Brief description of file purpose

**Documentation:**
```cpp
/**
 * @brief Brief description
 *
 * Detailed description of the function/class.
 * 
 * @param paramName Description of parameter
 * @return Description of return value
 */
```

**Inline comments:**
```cpp
// Single-line comments for brief notes
// Use // for C++ code, /* */ for C code
```

### C++ Features

**C++ Standard:** C++23

**Modern C++ usage:**
- Use `constexpr` for compile-time constants
- Prefer `auto` for type deduction when type is obvious
- Use range-based for loops
- Use smart pointers where appropriate
- Use `nullptr` instead of `NULL` or `0`

**Synced code:**
The engine has special macros for synchronized multiplayer code:
```cpp
ENTER_SYNCED_CODE();
// synced operations
LEAVE_SYNCED_CODE();
```

### Platform-Specific Code

Use preprocessor directives for platform-specific code:
```cpp
#ifdef WIN32
	// Windows-specific code
#endif

#ifdef HEADLESS
	// Headless build code
#endif

#if defined(__GNUC__)
	// GCC-specific code
#endif
```

## CMake Guidelines

### CMakeLists.txt Style
- Follow `.cmakelintrc` configuration
- Use tabs for indentation in CMake files
- Keep lines reasonably short

### Adding Tests
In `test/CMakeLists.txt`:
```cmake
set(test_name TestName)
set(test_src
	"${CMAKE_CURRENT_SOURCE_DIR}/path/to/TestFile.cpp"
	${test_Common_sources}
)
set(test_libs
	library_name
)
add_spring_test(${test_name} "${test_src}" "${test_libs}" "${test_flags}")
```

## Project Structure

- `rts/` - Main engine source code
  - `rts/System/` - Core system code
  - `rts/Game/` - Game logic
  - `rts/Sim/` - Simulation code
  - `rts/Rendering/` - Graphics rendering
  - `rts/Lua/` - Lua scripting interface
  - `rts/aGui/` - GUI components
  - `rts/lib/` - External libraries
- `test/` - Unit tests
- `tools/` - Utility tools
- `AI/` - AI interface code
- `cont/` - Content files
- `doc/` - Documentation

## AI Usage Policy

**IMPORTANT:** This project has a strict AI usage policy. See `AI_POLICY.md` for full details.

**Key requirements:**
1. **Disclose all AI usage** - State the tool used and extent of assistance
2. **PRs must reference accepted issues** - No drive-by AI-generated PRs
3. **Human verification required** - All AI-generated code must be tested by a human
4. **No AI-generated media** - Only text and code allowed
5. **Human-in-the-loop required** - Review and edit all AI-generated content

**Maintainers are exempt** from these rules; they use AI at their discretion.

## Important Notes

### Synced Code
The engine uses deterministic simulation for multiplayer. Code that affects game state must maintain sync across clients. Look for `SYNCCHECK` and `streflop` references.

### Threading
The engine uses custom thread pools. See `THREADPOOL` define and related code.

### Testing Changes
- For Lua changes: write a test widget
- For other changes: manual testing procedure required
- Automated tests are encouraged but not always required due to complexity

### Before Submitting PRs
1. Test your changes thoroughly
2. Reference an accepted issue
3. Document what testing was performed
4. Follow the workflow in `contributing.md`
5. Disclose any AI assistance used

## Additional Resources

- Official website: https://recoilengine.org
- Documentation: https://recoilengine.org/docs/
- Build guide: https://recoilengine.org/development/building-without-docker/
- Discord: https://discord.gg/GUpRg6Wz3e
- GitHub issues: https://github.com/beyond-all-reason/RecoilEngine/issues
