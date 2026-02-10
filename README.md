# Enterprise Browser

This repository contains the source code, patches, and management tools for a custom Enterprise Browser built on top of Chromium.

## Architecture & Strategy

The project employs a **Patch-and-Overlay** strategy to maintain a clean separation between the core Chromium source and custom enterprise-specific logic.

- **Custom Logic (Overlay):** All new files and enterprise features are located in this `enterprise_browser/` directory. The internal structure mirrors the Chromium repository (e.g., `enterprise_browser/browser/` corresponds to `src/chrome/browser/`).
- **Chromium Modifications (Patches):** Changes to existing Chromium files are stored as individual `.patch` files in the `patches/` directory. Each patch corresponds to exactly one modified file in the parent Chromium source tree.

## Repository Layout

```text
enterprise_browser/
├── app/                # Custom app-level resources (mirrors Chromium structure)
├── browser/            # Custom browser-level features (mirrors Chromium structure)
├── build/              # Custom build configurations and GN templates
├── config/             # Build and sync configuration (default.json)
├── docs/               # Technical documentation and design specifications
├── ebcm/               # Enterprise Browser Cloud Management utilities
├── patches/            # Per-file patches for the Chromium source
├── tools/eb/           # The 'eb' CLI management tool
└── enterprise_browser.gni # Main build configuration file
```

## The `eb` CLI Tool

The `eb` tool (located at `tools/eb/eb`) is the primary interface for managing the development environment, build process, and patch synchronization.

### Common Commands

- **`eb init`**: Bootstraps the environment, installs `depot_tools`, and configures the workspace.
- **`eb sync`**: Synchronizes the Chromium source to the specified base revision and automatically applies all patches.
- **`eb build`**: Generates build arguments and compiles the browser (e.g., `eb build chrome`).
- **`eb run`**: Launches the built browser with optional command-line arguments.
- **`eb patch update`**: Scans the Chromium source for local modifications and updates the corresponding files in the `patches/` directory.

## Development Guidelines

### Guarding Changes
All modifications to core Chromium files (via patches) and conditional logic in the overlay must be guarded:
- **C++:** `#if BUILDFLAG(IS_ENTERPRISE_BROWSER) ... #endif`
- **GN:** `if (is_enterprise_browser) { ... }`

### Mirroring Structure
When adding new files, always place them in a directory within `enterprise_browser/` that reflects where they would logically reside in the Chromium tree.

### Licensing
New files created in this repository must use the Apache License, Version 2.0, with the following copyright notice:
`Copyright (c) 2026 Jani Hautakangas <jani@kodegood.com>`

## Documentation
Refer to the `docs/` directory for detailed documentation:
- `chrome_enterprise_design.md`: Overview of Chromium's existing enterprise architecture.
- `eb_build_tool.md`: Specification of the `eb` build system and patch management.
