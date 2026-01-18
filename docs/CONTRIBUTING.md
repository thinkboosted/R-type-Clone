# 🤝 Contributing Guide

Thank you for your interest in contributing to the R-Type Clone project! This guide will help you get started.

---

## 📋 Table of Contents

- [Code of Conduct](#-code-of-conduct)
- [Getting Started](#-getting-started)
- [Development Workflow](#-development-workflow)
- [Coding Standards](#-coding-standards)
- [Commit Guidelines](#-commit-guidelines)
- [Pull Request Process](#-pull-request-process)
- [Project Architecture](#-project-architecture)

---

## 📜 Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Keep discussions on-topic

---

## 🚀 Getting Started

### 1. Fork the Repository

Click the **Fork** button on GitHub to create your own copy.

### 2. Clone Your Fork

```bash
git clone --recursive https://github.com/YOUR_USERNAME/R-type-Clone.git
cd R-type-Clone
```

### 3. Set Up Development Environment

```bash
# Bootstrap vcpkg
./vcpkg/bootstrap-vcpkg.sh  # Linux
# or
.\vcpkg\bootstrap-vcpkg.bat  # Windows

# Build the project
python3 build.py

# Verify the build
cd build
./r-type_client local
```

### 4. Add Upstream Remote

```bash
git remote add upstream https://github.com/thinkboosted/R-type-Clone.git
```

---

## 🔄 Development Workflow

### Branch Naming Convention

| Prefix | Purpose | Example |
|--------|---------|---------|
| `feature/` | New features | `feature/charge-shot` |
| `fix/` | Bug fixes | `fix/collision-detection` |
| `docs/` | Documentation | `docs/api-reference` |
| `refactor/` | Code refactoring | `refactor/module-loader` |
| `test/` | Test additions | `test/network-unit-tests` |

### Create a Feature Branch

```bash
# Sync with upstream
git fetch upstream
git checkout main
git merge upstream/main

# Create your branch
git checkout -b feature/your-feature-name
```

### Keep Your Branch Updated

```bash
git fetch upstream
git rebase upstream/main
```

---

## 📝 Coding Standards

### C++ Style Guide

#### General Rules

- Use **C++17** features
- Use **smart pointers** (`std::unique_ptr`, `std::shared_ptr`)
- Prefer **RAII** for resource management
- Use `nullptr` instead of `NULL`

#### Naming Conventions

```cpp
// Classes: PascalCase
class ModulesManager {};

// Functions/Methods: camelCase
void processMessages();

// Variables: camelCase
int playerScore;

// Constants: UPPER_SNAKE_CASE
const int MAX_PLAYERS = 4;

// Private members: prefix with underscore
class Example {
private:
    int _privateVar;
};

// Namespaces: camelCase
namespace rtypeEngine {}
```

#### File Organization

```cpp
// Header files (.hpp)
#pragma once

#include <system_headers>
#include "project_headers.hpp"

namespace rtypeEngine {

class MyClass {
public:
    // Public methods
    
protected:
    // Protected methods
    
private:
    // Private members
};

} // namespace rtypeEngine
```

#### Documentation (Doxygen)

```cpp
/**
 * @brief Brief description of the function.
 * 
 * Detailed description if needed.
 * 
 * @param param1 Description of parameter
 * @return Description of return value
 * @throws ExceptionType Description of exception
 */
int myFunction(int param1);
```

### Lua Style Guide

#### Naming Conventions

```lua
-- Modules: PascalCase
local PlayerSystem = {}

-- Functions: camelCase
function PlayerSystem.updatePosition(entity) end

-- Variables: camelCase
local playerHealth = 100

-- Constants: UPPER_SNAKE_CASE
local MAX_ENEMIES = 50
```

#### ECS Components

```lua
-- Component factory functions: PascalCase
function Transform(x, y, z, rx, ry, rz, sx, sy, sz)
    return {
        x = x or 0,
        y = y or 0,
        z = z or 0,
        -- ...
    }
end
```

---

## 💬 Commit Guidelines

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation changes |
| `style` | Code style (formatting, etc.) |
| `refactor` | Code refactoring |
| `test` | Adding/modifying tests |
| `chore` | Build process, dependencies |

### Examples

```bash
# Feature
git commit -m "feat(gameplay): add charge shot mechanic"

# Bug fix
git commit -m "fix(collision): resolve bullet-enemy detection issue"

# Documentation
git commit -m "docs(readme): update installation instructions"

# Refactor
git commit -m "refactor(renderer): optimize entity batch rendering"
```

---

## 🔍 Pull Request Process

### Before Submitting

1. **Test your changes**
   ```bash
   cd build
   ./r-type_client local  # Test in solo mode
   ```

2. **Check for warnings**
   ```bash
   cmake --build . 2>&1 | grep -i warning
   ```

3. **Update documentation** if needed

### Creating a Pull Request

1. Push your branch to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

2. Go to GitHub and create a Pull Request

3. Fill in the PR template:
   - **Title**: Clear description of the change
   - **Description**: What and why
   - **Testing**: How you tested
   - **Screenshots**: If UI changes

### PR Review Checklist

- [ ] Code follows style guidelines
- [ ] Changes are tested
- [ ] Documentation is updated
- [ ] No merge conflicts
- [ ] CI/CD passes

---

## 🏗️ Project Architecture

### Module System

The engine uses a dynamic module architecture:

```
┌─────────────────────────────────────────────────────┐
│                    Application                       │
│  ┌─────────────────────────────────────────────┐    │
│  │              ModulesManager                  │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐       │    │
│  │  │ Window  │ │Renderer │ │  ECS    │ ...   │    │
│  │  │ Manager │ │         │ │ Manager │       │    │
│  │  └────┬────┘ └────┬────┘ └────┬────┘       │    │
│  │       │           │           │             │    │
│  │       └───────────┴───────────┘             │    │
│  │                   │                          │    │
│  │           ┌───────┴───────┐                 │    │
│  │           │  ZeroMQ Bus   │                 │    │
│  │           │  (Pub/Sub)    │                 │    │
│  │           └───────────────┘                 │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

### Key Directories

| Directory | Purpose |
|-----------|---------|
| `src/engine/modules/` | Module implementations |
| `src/engine/app/` | Application base classes |
| `src/game/` | R-Type game implementation |
| `assets/scripts/` | Lua game logic |

### Adding a New Module

1. Create interface in `src/engine/modules/YourModule/IYourModule.hpp`
2. Implement in `src/engine/modules/YourModule/Impl/`
3. Add CMakeLists.txt for the module
4. Register in ModulesManager

See [Architecture Documentation](ARCHITECTURE.md) for details.

---

## 🐛 Reporting Issues

### Bug Reports

Include:
- **Description**: What happened?
- **Expected**: What should happen?
- **Steps to Reproduce**: How to trigger the bug
- **Environment**: OS, compiler, etc.
- **Logs**: Any error messages

### Feature Requests

Include:
- **Description**: What you want
- **Use Case**: Why it's useful
- **Proposed Solution**: How it could work

---

## 📞 Getting Help

- **GitHub Issues**: For bugs and features
- **GitHub Discussions**: For questions
- **Code Review**: Request review from maintainers

---

## 🙏 Thank You!

Every contribution makes this project better. We appreciate your time and effort!
