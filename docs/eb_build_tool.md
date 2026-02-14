# Enterprise Browser Build Tool

## 1. Overview & Goals
The goal is to provide a cross-platform (Linux, macOS, Windows) management tool for a custom Chromium fork. The system uses a **Patch-and-Overlay** strategy where the custom code lives in a separate repository (`src/enterprise_browser`) and modifications to Chromium core are maintained as a flat collection of per-file patches.

### Core Objectives
*   **Decoupled Source:** Keep Chromium source clean; all custom logic resides in the `enterprise_browser` repo.
*   **Structure Mirroring:** The `enterprise_browser` repo mirrors the Chromium directory structure for new files.
*   **Per-File Patching:** Modifications to existing Chromium files are stored as individual patches (one patch per modified file).
*   **Standard Tooling:** Leverage `depot_tools` (gn, ninja/siso, gclient).
*   **Developer Friendly:** Automate patch generation and application.

---

## 2. Repository & Workspace Layout
The system assumes a standard Chromium `<project_tree>` structure:

```text
<project_tree>/
├── .gclient                # Managed by eb init
├── src/                    # The Chromium "src" repository
    ├── .git/
    ├── enterprise_browser/ # The repository (Source of Truth)
    │   ├── .git/
    │   ├── chrome/         # Mirrors src/chrome/
    │   │   ├── browser/
    │   │   │   └── policy/
    │   │   │       └── custom_provider.cc
    │   ├── config/
    │   │   └── default.json
    │   ├── patches/        # Flat list of patches
    │   │   ├── ui-gfx-image-image_skia.cc.patch
    │   │   └── chrome-browser-ui-browser.cc.patch
    │   ├── tools/
    │   │   └── eb/         # CLI tools
    │   │       ├── eb.py   # Main CLI Logic
    │   │       ├── eb      # Linux/macOS wrapper
    │   │       └── eb.bat  # Windows wrapper
    │   ├── vendor/         # Third-party tools (depot_tools)
    │   └── ...
    └── ... (standard chromium files)
```

### Source of Truth
*   **New Files:** Reside in `enterprise_browser/` mirroring the target structure.
*   **Modified Files:** The *diff* resides in `enterprise_browser/patches/` as a named `.patch` file.

---

## 3. Configuration System
Configuration is handled via `config/default.json` and overrides.

### `config/default.json`
```json
{
  "chromium": {
    "base_revision": "144.0.7559.83",
    "git_remote": "https://github.com/KodeGood/chromium.git"
  },
  "build": {
    "target": "chrome",
    "default_gn_args": { ... }
  }
}
```

---

## 4. Patch Management System

### Strategy: One Patch Per File
Instead of a monolithic feature patch series, the system maintains one patch file per modified Chromium source file.

**Naming Convention:**
*   Source: `ui/gfx/image/image_skia.cc`
*   Patch: `patches/ui-gfx-image-image_skia.cc.patch`
*   Rule: Replace directory separators `/` with `-`.

### Patch Application (`eb sync`)
1.  **Reset:** Hard reset `src/` to the `base_revision` defined in config so the tree is a clean, reproducible baseline before any patching takes place.
2.  **Apply:** Iterate through all `*.patch` files in `enterprise_browser/patches/`.
3.  **Method:** Use `git apply` for each file so only the working tree changes; since patches are per-file, order is irrelevant and no commits are created in the Chromium repo during patching.
4.  **New Files:** (Implicit) New files exist in `src/enterprise_browser/`. Patches (e.g., to `BUILD.gn`) must reference them via `//enterprise_browser/...`.

### Patch Generation (`eb patch-update`)
1.  **Scan:** Run `git diff --name-only <base_revision>` in `src/` (excluding `enterprise_browser/`).
2.  **Generate:** For each modified file:
    *   Compute destination patch path (e.g., `patches/foo-bar.cc.patch`).
    *   Run `git diff <base_revision> -- <file> > <patch_path>`.
3.  **Clean:** Optionally remove patch files for files that are no longer modified (compared to base).
4.  **Result:** The `patches/` directory now reflects the exact state of the dirty tree.

---

## 5. CLI Commands

### `eb init`
*   Checks/Installs `depot_tools` into the `vendor/` directory.
*   Runs `fetch chromium` if missing.
*   Configures `.gclient` to map `src/enterprise_browser`.

### `eb sync`
1.  **Sync Chromium:** Runs `gclient sync -D -r src@<base_revision> [--jobs <N>]`.
    *   **Parallel Jobs:** Defaults to 3 jobs (`-j 3`). This conservative default is chosen to avoid network instability or being blocked by remotes during the initial multi-gigabyte sync. Users on high-bandwidth connections can significantly speed up the process by passing a higher value (e.g., `--jobs 16`).
    *   **Windows Performance Note:** On Windows, the initial sync may appear to hang at "Updating depot_tools..." for 30-60 minutes. This is caused by Git's `index-pack` operation and `depot_tools` self-updates. Adding the project directory to your antivirus (Windows Defender) exclusion list is highly recommended.
    *   **Retry Mechanism:** If `gclient sync` fails, `eb sync` will attempt to retry the operation a configurable number of times.
2.  **Apply Patches:** Automatically runs `eb patch apply`.
3.  **Apply Branding:** Automatically runs `eb branding`.

### `eb patch apply`
*   Applies patches from `enterprise_browser/patches/` incrementally using `.patchinfo` metadata.
*   **Stale Detection:** Compares the SHA-256 hash of the `.patch` file against the recorded hash in `.patchinfo`.
*   **Safety:** Reverts individual Chromium files to their base state before applying/re-applying patches.
*   **Cleanup:** Reverts Chromium files for which patches have been removed from the `patches/` directory.

### `eb branding`
*   Applies Enterprise Browser branding assets to the Chromium source tree.
*   **Mappings:**
    *   `app/theme/enterprise_browser/**` -> `src/chrome/app/theme/enterprise_browser/`
    *   `app/theme/default_100_percent/enterprise_browser/**` -> `src/chrome/app/theme/default_100_percent/enterprise_browser/`
    *   `app/theme/default_200_percent/enterprise_browser/**` -> `src/chrome/app/theme/default_200_percent/enterprise_browser/`
    *   `app/resources/**` -> `src/chrome/app/resources/`
    *   `app/enterprise_browser_strings.grd` -> `src/chrome/app/enterprise_browser_strings.grd`
    *   `app/settings_enterprise_browser_strings.grdp` -> `src/chrome/app/settings_enterprise_browser_strings.grdp`
    *   `components/components_enterprise_browser_strings.grd` -> `src/components/components_enterprise_browser_strings.grd`
    *   `components/vector_icons/enterprise_browser/**` -> `src/components/vector_icons/enterprise_browser/`
*   **Behavior:** Transparently copies files and creates destination directories as needed.

### `eb patch update`
*   Updates the `patches/` folder based on current local changes in `src/`.
*   **Workflow:**
    1.  Dev modifies Chromium source.
    2.  Dev runs `eb patch update [--dry-run]`.
    3.  Per-file `.patch` files are created/updated in `patches/`.
    4.  **Exclusions:** Automatically skips binary assets (images), localized resources (`.grd`), and metadata files (`.patchinfo`).
    5.  Dev commits the updated `.patch` files to the `enterprise_browser` repo.

### `eb build`
*   Generates `args.gn` from `config/default.json`.
*   **ActionGuard:** Skips `gn gen` if GN arguments haven't changed and `build.ninja` exists.
*   **`--force-gn-gen`**: Manually forces `gn gen` to run even if no changes are detected.
*   Runs `autoninja -C out/Default chrome`.
*   **Note:** This command does not automatically run `eb sync`. It is expected that the tree is already synchronized.

### `eb run`
*   Executes the built Chromium binary from the output directory.
*   Automatically identifies the correct platform-specific binary path.
*   **Forwarding:** Passes all additional command-line arguments directly to Chromium.
*   **Example:** `eb run --user-data-dir=/tmp/eb_profile --no-first-run`

---

## 6. Patch Tracking (`.patchinfo`)
To ensure synchronization is fast and idempotent, the system maintains a `.patchinfo` (JSON) metadata file for every applied patch. This file follows the Brave Browser schema for compatibility.

**Example `.patchinfo` Schema:**
```json
{
  "schemaVersion": 1,
  "patchChecksum": "efc6b56c5479b3060f7584ada1fcd9bfdec642a14eefe06bc133b0d5e6f16d8d",
  "appliesTo": [
    {
      "path": "ui/gfx/image/image_skia.cc",
      "checksum": "aac727022999a262bc7006f0d00ed68b9272dcf7f1683665ef396511fc1551cf"
    }
  ]
}
```

**Stored Metadata:**
*   `schemaVersion`: Version of the metadata format.
*   `patchChecksum`: SHA-256 hash of the `.patch` file content.
*   `appliesTo`: A list of target files modified by the patch.
    *   `path`: Relative path to the Chromium source file.
    *   `checksum`: SHA-256 hash of the target file at its clean base revision.

These files are used by `eb patch apply` to decide which files need re-patching and are automatically updated upon successful application.

---

## 7. Implementation Notes
*   **Path Handling:** All patch paths are relative to `src/`.
*   **Line Endings:** Force LF in generated patches.
*   **Git Interaction:**
    *   Use `subprocess.run(['git', ...])`.
    *   Ensure `cwd` is set correctly for diffs (root of `src`).
*   **Safety:** `eb patch update` should warn if it's about to overwrite/delete patches, or just do it (since it's under git control in `enterprise_browser`).

## 8. Acceptance Criteria
1.  **Mirroring:** Custom files in `enterprise_browser` are accessible to the build (via GN patches).
2.  **Granularity:** Modifying a single Chromium file results in a single corresponding `.patch` file update.
3.  **Isolation:** No manual commits are ever made to the `src/` git history; all state is preserved in `enterprise_browser`.
4.  **Platform Support:** Wrapper scripts (`eb`, `eb.bat`) allow easy execution across Linux, macOS, and Windows.
5.  **Robust Synchronization:** `eb init` and `eb sync` handle network issues with retries and ensure the correct Chromium tag is checked out.
6.  **Flexible Builds:** `eb build` correctly configures and builds Chromium targets with support for custom output directories and optimized GN generation.
7.  **Incremental Efficiency:** Patching only affects files that have actually changed, preserving build artifacts and third-party dependencies.
8.  **Direct Testing:** `eb run` provides a convenient way to launch the built browser with custom parameters for rapid testing.
