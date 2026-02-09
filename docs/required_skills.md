# Required Skills for Enterprise Browser Implementation

This document outlines the core skills and expertise required for an agent to successfully implement the features detailed in the enterprise browser project plan. The project's success hinges on proficiency in navigating and extending the complex Chromium codebase.

---

### 1. Expert-Level C++ Proficiency

-   **Description**: The entire Chromium codebase is written in high-performance, modern C++. The implementation agent must be able to read, understand, and write C++ code that is not only correct but also idiomatic to the Chromium project.
-   **Key Abilities**:
    -   Deep understanding of C++17/C++20 features.
    -   Proficiency with Chromium's specific C++ patterns, including their smart pointer library (`scoped_refptr`, `raw_ptr`), callback system (`base::Callback`), and threading models.
    -   Strict adherence to the Chromium C++ Style Guide (`styleguide/c++/`).

### 2. Chromium Architecture Comprehension

-   **Description**: Our strategy is to "extend, don't modify." This requires a deep architectural understanding to find the correct "seams" for extension without creating a brittle, hard-to-maintain fork.
-   **Key Abilities**:
    -   Ability to trace execution flow across different processes (Browser, Renderer, GPU).
    -   Skill in identifying the correct observer classes (`WebContentsObserver`), interfaces (`NavigationThrottle`), and service factories (`KeyedServiceFactory`) to hook into.
    -   A strong mental model of the navigation pipeline, policy subsystem, and rendering stack.

### 3. Build System Management (GN/Ninja)

-   **Description**: To integrate any new code, the agent must be proficient in editing Chromium's `BUILD.gn` files. This is a non-negotiable skill for building and linking new features.
-   **Key Abilities**:
    -   Defining new build targets (components, executables).
    -   Managing dependencies between our new components and existing Chromium components.
    -   Diagnosing and resolving build errors related to dependency graphs or compiler configurations.

### 4. Automated Testing Expertise

-   **Description**: The project plan requires that all new features be accompanied by comprehensive tests. This is critical for ensuring quality and preventing regressions during Chromium rebases.
-   **Key Abilities**:
    -   Writing effective and isolated **Unit Tests (`*_unittest.cc`)** using the Google Test framework.
    -   Creating robust **Browser Tests (`*_browsertest.cc`)** that validate feature functionality in the context of a live, multi-process browser environment.

### 5. Systematic Debugging and Problem-Solving

-   **Description**: The development process will be an iterative cycle of building, testing, and debugging. The agent must have a strong capability to systematically diagnose and resolve complex issues.
-   **Key Abilities**:
    -   Parsing and understanding complex C++ compiler and linker errors.
    -   Analyzing test logs, stack traces, and crash dumps to pinpoint root causes.
    -   Applying a methodical approach to problem-solving within a massive and intricate codebase.
