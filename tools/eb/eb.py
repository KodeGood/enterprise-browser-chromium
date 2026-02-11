import argparse
import os
import subprocess
import time
import json
import hashlib
import sys
import shutil

# Define the project root as one level up from the enterprise_browser directory
# This assumes eb.py is at <project_root>/src/enterprise_browser/tools/eb/eb.py
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
ENTERPRISE_BROWSER_DIR = os.path.join(PROJECT_ROOT, 'src', 'enterprise_browser')
CHROMIUM_SRC_DIR = os.path.join(PROJECT_ROOT, 'src')
VENDOR_DIR = os.path.join(ENTERPRISE_BROWSER_DIR, 'vendor')
DEPOT_TOOLS_DIR = os.path.join(VENDOR_DIR, 'depot_tools')

CONFIG_FILE = os.path.join(ENTERPRISE_BROWSER_DIR, 'config', 'default.json')

def run_command(cmd, cwd=None, check=True, capture_output=False, env=None, text=True, quiet=False):
    """Helper to run shell commands."""
    if not quiet:
        print(f"Running command: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd, check=check, capture_output=capture_output, env=env, text=text)
    if capture_output and not quiet:
        print(result.stdout)
    return result

def load_config():
    """Loads the configuration from default.json."""
    if not os.path.exists(CONFIG_FILE):
        print(f"Error: Configuration file not found at {CONFIG_FILE}")
        exit(1)
    with open(CONFIG_FILE, 'r') as f:
        return json.load(f)

def get_file_hash(file_path):
    """Calculates SHA-256 hash of a file."""
    if not os.path.exists(file_path):
        return None
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def ensure_depot_tools():
    """Ensures depot_tools are present and added to PATH."""
    print("Ensuring depot_tools...")
    if not os.path.exists(DEPOT_TOOLS_DIR):
        print(f"depot_tools not found at {DEPOT_TOOLS_DIR}. Cloning...")
        os.makedirs(VENDOR_DIR, exist_ok=True)
        run_command(['git', 'clone', 'https://chromium.googlesource.com/chromium/tools/depot_tools.git', DEPOT_TOOLS_DIR])

    os.environ['PATH'] = f"{DEPOT_TOOLS_DIR}{os.pathsep}{os.environ['PATH']}"
    print("depot_tools ensured and added to PATH.")

def cmd_init(args):
    """Implements the 'eb init' command."""
    print("Initializing Enterprise Browser build system...")
    ensure_depot_tools()
    config = load_config()
    chromium_git_remote = config['chromium']['git_remote']
    base_revision = config['chromium']['base_revision']

    # 1. Run fetch chromium if missing or not checked out
    is_git_initialized = os.path.exists(os.path.join(CHROMIUM_SRC_DIR, '.git'))
    is_checked_out = False
    if is_git_initialized:
        try:
            run_command(['git', 'rev-parse', 'HEAD'], cwd=CHROMIUM_SRC_DIR, capture_output=True, quiet=True)
            is_checked_out = True
        except subprocess.CalledProcessError:
            is_checked_out = False

    if not is_checked_out:
        print(f"Chromium source not checked out in {CHROMIUM_SRC_DIR}. Initializing/Fetching...")
        os.makedirs(CHROMIUM_SRC_DIR, exist_ok=True)
        if not is_git_initialized:
            # Initialize git in src if not already there
            run_command(['git', 'init'], cwd=CHROMIUM_SRC_DIR)
        # Add remote if not exists
        try:
            run_command(['git', 'remote', 'add', 'origin', chromium_git_remote], cwd=CHROMIUM_SRC_DIR)
        except subprocess.CalledProcessError:
            # Remote might already exist
            run_command(['git', 'remote', 'set-url', 'origin', chromium_git_remote], cwd=CHROMIUM_SRC_DIR)
        # Fetch the specific base_revision
        print(f"Fetching {base_revision}...")
        run_command(['git', 'fetch', '--depth', '1', 'origin', f'refs/tags/{base_revision}:refs/tags/{base_revision}'], cwd=CHROMIUM_SRC_DIR)
        # Checkout the revision
        print(f"Checking out {base_revision}...")
        run_command(['git', 'checkout', '-f', base_revision], cwd=CHROMIUM_SRC_DIR)
    else:
        print("Chromium source already checked out.")

    # 2. Configure .gclient
    gclient_file = os.path.join(PROJECT_ROOT, '.gclient')
    if not os.path.exists(gclient_file):
        print(f"Creating .gclient file at {gclient_file}")
        # We manually write the .gclient file to ensure the name is 'src'
        gclient_content = f"""solutions = [
  {{
    "name": "src",
    "url": "{chromium_git_remote}",
    "managed": False,
    "custom_deps": {{}},
    "custom_vars": {{
        "checkout_clangd": True,
    }},
  }},
]
"""
        with open(gclient_file, 'w') as f:
            f.write(gclient_content)
    else:
        print(f".gclient file already exists at {gclient_file}")

    # 3. gclient sync
    gclient_env = os.environ.copy()
    sync_jobs = 3 # Default jobs for initial sync to avoid WAN choking
    sync_success = False
    retries = 0
    max_retries = 3 # Can be configurable later
    while retries <= max_retries:
        try:
            print(f"Attempting gclient sync (attempt {retries + 1}/{max_retries + 1})...")
            run_command(['gclient', 'sync', '-j', str(sync_jobs)], cwd=PROJECT_ROOT, env=gclient_env)
            sync_success = True
            break
        except subprocess.CalledProcessError as e:
            print(f"gclient sync failed during init (attempt {retries + 1}/{max_retries + 1}): {e}")
            retries += 1
            if retries > max_retries:
                print("Max retries reached. gclient sync failed during init.")
                break
            print("Retrying in 5 seconds...")
            time.sleep(5) # Pause before retrying

    if not sync_success:
        print("Initialization failed: gclient sync was unsuccessful.")
        exit(1)

    # 4. Configure .gclient to map src/enterprise_browser
    if os.path.exists(gclient_file):
        with open(gclient_file, 'r') as f:
            gclient_content = f.read()
        # Check if 'src/enterprise_browser' is already mapped
        if "src/enterprise_browser" not in gclient_content:
            print("Mapping src/enterprise_browser in .gclient...")
            gclient_solution_entry = f"""
solutions.append({{
  'name': 'src/enterprise_browser',
  'url': 'https://github.com/KodeGood/enterprise-browser.git',
  'deps_file': '.DEPS.git',
  'managed': False,
  'custom_vars': {{
      "checkout_clangd": True,
    }},
}})
"""
            with open(gclient_file, 'a') as f:
                f.write(gclient_solution_entry)
            print("src/enterprise_browser mapping added to .gclient.")
        else:
            print("src/enterprise_browser already mapped in .gclient.")
    else:
        print(f"Error: .gclient file not found at {gclient_file} after sync attempt.")
        exit(1)
    print("Initialization complete.")

def cmd_sync(args):
    """Implements the 'eb sync' command."""
    print("Syncing Enterprise Browser build system...")
    config = load_config()
    base_revision = config['chromium']['base_revision']
    ensure_depot_tools()
    # 1. Sync Chromium
    sync_jobs = args.jobs if args.jobs is not None else 3 # Default to 3 if not specified
    sync_cmd = ['gclient', 'sync', '-D', '-r', f'src@{base_revision}', '-j', str(sync_jobs)]
    gclient_env = os.environ.copy()
    sync_success = False
    retries = 0
    max_retries = 3 # Hardcoded for now, could be configurable
    while retries <= max_retries:
        try:
            print(f"Attempting gclient sync (attempt {retries + 1}/{max_retries + 1})...")
            run_command(sync_cmd, cwd=PROJECT_ROOT, env=gclient_env)
            sync_success = True
            break
        except subprocess.CalledProcessError as e:
            print(f"gclient sync failed (attempt {retries + 1}/{max_retries + 1}): {e}")
            retries += 1
            if retries > max_retries:
                print("Max retries reached. gclient sync failed.")
                break
            print("Retrying in 5 seconds...")
            time.sleep(5)
    if not sync_success:
        print("Skipping patch application due to gclient sync failure.")
        return
    cmd_patch_apply(args)
    cmd_branding(args)

def cmd_branding(args):
    """Implements the 'eb branding' command to apply branding assets."""
    print("Applying Enterprise Browser branding...")
    # Define branding paths
    branding_maps = [
        {
            'src': os.path.join(ENTERPRISE_BROWSER_DIR, 'app', 'theme', 'enterprise_browser'),
            'dest': os.path.join(CHROMIUM_SRC_DIR, 'chrome', 'app', 'theme', 'enterprise_browser')
        },
        {
            'src': os.path.join(ENTERPRISE_BROWSER_DIR, 'app', 'theme', 'default_100_percent', 'enterprise_browser'),
            'dest': os.path.join(CHROMIUM_SRC_DIR, 'chrome', 'app', 'theme', 'default_100_percent', 'enterprise_browser')
        },
        {
            'src': os.path.join(ENTERPRISE_BROWSER_DIR, 'app', 'theme', 'default_200_percent', 'enterprise_browser'),
            'dest': os.path.join(CHROMIUM_SRC_DIR, 'chrome', 'app', 'theme', 'default_200_percent', 'enterprise_browser')
        },
        {
            'src': os.path.join(ENTERPRISE_BROWSER_DIR, 'app', 'enterprise_browser_strings.grd'),
            'dest': os.path.join(CHROMIUM_SRC_DIR, 'chrome', 'app', 'enterprise_browser_strings.grd')
        },
        {
            'src': os.path.join(ENTERPRISE_BROWSER_DIR, 'components', 'components_enterprise_browser_strings.grd'),
            'dest': os.path.join(CHROMIUM_SRC_DIR, 'components', 'components_enterprise_browser_strings.grd')
        },
        {
            'src': os.path.join(ENTERPRISE_BROWSER_DIR, 'components', 'vector_icons', 'enterprise_browser'),
            'dest': os.path.join(CHROMIUM_SRC_DIR, 'components', 'vector_icons', 'enterprise_browser')
        }
    ]
    for mapping in branding_maps:
        src_path = mapping['src']
        dest_path = mapping['dest']
        if not os.path.exists(src_path):
            print(f"Warning: Source branding path not found: {src_path}")
            continue
        print(f"Applying branding from {src_path} to {dest_path}...")
        if os.path.isdir(src_path):
            if os.path.exists(dest_path):
                for root, dirs, files in os.walk(src_path):
                    rel_path = os.path.relpath(root, src_path)
                    dest_root = os.path.join(dest_path, rel_path)
                    os.makedirs(dest_root, exist_ok=True)
                    for file in files:
                        s_file = os.path.join(root, file)
                        d_file = os.path.join(dest_root, file)
                        shutil.copy2(s_file, d_file)
            else:
                shutil.copytree(src_path, dest_path)
        else:
            # It's a file
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            shutil.copy2(src_path, dest_path)
    print("Branding application complete.")

def cmd_patch_apply(args):
    """Implements the 'eb patch apply' command."""
    print("Updating patches incrementally...")
    patches_dir = os.path.join(ENTERPRISE_BROWSER_DIR, 'patches')
    if not os.path.exists(patches_dir):
        print("No patches directory found. Skipping.")
        return
    patch_info_files = [f for f in os.listdir(patches_dir) if f.endswith('.patchinfo')]
    active_patches = [f for f in os.listdir(patches_dir) if f.endswith('.patch')]
    processed_patches = set()
    for patch_file in active_patches:
        patch_path = os.path.join(patches_dir, patch_file)
        patch_info_path = patch_path + 'info'
        processed_patches.add(patch_file)
        stale = True
        target_file = None
        if os.path.exists(patch_info_path):
            try:
                with open(patch_info_path, 'r') as f:
                    info = json.load(f)
                if info.get('appliesTo'):
                    target_entry = info['appliesTo'][0]
                    target_file = target_entry.get('path')
                old_patch_hash = info.get('patchChecksum')
                current_patch_hash = get_file_hash(patch_path)
                if current_patch_hash == old_patch_hash:
                    stale = False
            except Exception as e:
                print(f"Warning: Failed to read {patch_info_path}: {e}. Re-applying.")
                stale = True
        if stale:
            print(f"Applying {patch_file}...")
            if not target_file:
                target_file = patch_file[:-6].replace('-', '/')
            target_full_path = os.path.join(CHROMIUM_SRC_DIR, target_file)
            try:
                if os.path.exists(target_full_path):
                    run_command(['git', 'checkout', '--', target_file], cwd=CHROMIUM_SRC_DIR)
                base_hash = get_file_hash(target_full_path)
                run_command(['git', 'apply', '--whitespace=nowarn', patch_path], cwd=CHROMIUM_SRC_DIR)
                new_info = {
                    "schemaVersion": 1,
                    "patchChecksum": get_file_hash(patch_path),
                    "appliesTo": [
                        {
                            "path": target_file,
                            "checksum": base_hash
                        }
                    ]
                }
                with open(patch_info_path, 'w') as f:
                    json.dump(new_info, f, indent=2)
            except subprocess.CalledProcessError as e:
                print(f"Error applying patch {patch_file}: {e}")
                continue
    for info_file in patch_info_files:
        patch_file = info_file[:-4]
        if patch_file not in processed_patches:
            print(f"Removing obsolete patch state for: {patch_file}")
            info_path = os.path.join(patches_dir, info_file)
            try:
                with open(info_path, 'r') as f:
                    info = json.load(f)
                target_file = None
                if info.get('appliesTo'):
                    target_file = info['appliesTo'][0].get('path')
                if target_file:
                    target_full_path = os.path.join(CHROMIUM_SRC_DIR, target_file)
                    if os.path.exists(target_full_path):
                        print(f"Reverting {target_file} as patch is removed.")
                        run_command(['git', 'checkout', '--', target_file], cwd=CHROMIUM_SRC_DIR)
                os.remove(info_path)
            except Exception as e:
                print(f"Warning: Failed to clean up {info_file}: {e}")
    print("Patch application complete.")

def cmd_patch_update(args):
    """Implements the 'eb patch-update' command."""
    print("Updating patches...")
    config = load_config()
    base_revision = config['chromium']['base_revision']
    patches_dir = os.path.join(ENTERPRISE_BROWSER_DIR, 'patches')
    if not args.dry_run:
        os.makedirs(patches_dir, exist_ok=True)
    # 1. Scan for modified files
    diff_cmd = ['git', 'diff', '--name-only', base_revision]
    result = run_command(diff_cmd, cwd=CHROMIUM_SRC_DIR, capture_output=True, quiet=True)
    excluded_extensions = {'.png', '.jpg', '.jpeg', '.gif', '.ico', '.grd', '.grdp', '.patchinfo'}
    excluded_branding_paths = [
        'chrome/app/theme/enterprise_browser',
        'chrome/app/theme/default_100_percent/enterprise_browser',
        'chrome/app/theme/default_200_percent/enterprise_browser'
    ]
    modified_files = []
    for f in result.stdout.splitlines():
        if not f or f.startswith('enterprise_browser/'):
            continue
        is_branding = False
        for b_path in excluded_branding_paths:
            if f.startswith(b_path):
                is_branding = True
                break
        if is_branding:
            continue
        ext = os.path.splitext(f)[1].lower()
        if ext in excluded_extensions:
            print(f"Skipping excluded file: {f}")
            continue
        modified_files.append(f)
    if not modified_files:
        print("No modifications found in Chromium source.")
        return
    print(f"Found {len(modified_files)} modified files. Generating patches...")
    current_patches = set()
    for file_path in modified_files:
        patch_name = file_path.replace('/', '-') + '.patch'
        patch_path = os.path.join(patches_dir, patch_name)
        current_patches.add(patch_name)
        if args.dry_run:
            print(f"[DRY-RUN] Would generate patch for {file_path} -> {patch_name}")
        else:
            patch_result = run_command(['git', 'diff', base_revision, '--', file_path], cwd=CHROMIUM_SRC_DIR, capture_output=True, quiet=True)
            new_patch_content = patch_result.stdout
            is_different = True
            if os.path.exists(patch_path):
                with open(patch_path, 'r', newline='\n') as f:
                    if f.read() == new_patch_content:
                        is_different = False
            if is_different:
                print(f"Updating patch for {file_path} -> {patch_name}")
                with open(patch_path, 'w', newline='\n') as f:
                    f.write(new_patch_content)
            else:
                print(f"Patch for {file_path} is already up to date.")
    # 3. Clean up obsolete patches
    for existing_patch in os.listdir(patches_dir):
        if existing_patch.endswith('.patch') and existing_patch not in current_patches:
            if args.dry_run:
                print(f"[DRY-RUN] Would remove obsolete patch: {existing_patch}")
            else:
                print(f"Removing obsolete patch: {existing_patch}")
                os.remove(os.path.join(patches_dir, existing_patch))
                info_path = os.path.join(patches_dir, existing_patch + 'info')
                if os.path.exists(info_path):
                    os.remove(info_path)
    print("Patch update complete.")

def cmd_build(args):
    """Implements the 'eb build' command."""
    ensure_depot_tools()
    print("Building Chromium...")
    config = load_config()
    target = config['build']['target']
    gn_args_dict = config['build']['default_gn_args']
    out_dir_name = args.out_dir
    out_dir_rel = os.path.join('out', out_dir_name)
    out_dir = os.path.join(CHROMIUM_SRC_DIR, out_dir_rel)
    os.makedirs(out_dir, exist_ok=True)
    gn_args_lines = []
    for key, value in gn_args_dict.items():
        if isinstance(value, bool):
            val_str = str(value).lower()
        elif isinstance(value, str):
            val_str = f'"{value}"'
        else:
            val_str = str(value)
        gn_args_lines.append(f'{key} = {val_str}')
    new_args_content = '\n'.join(gn_args_lines) + '\n'
    new_args_hash = hashlib.sha256(new_args_content.encode()).hexdigest()
    args_gn_path = os.path.join(out_dir, 'args.gn')
    args_hash_path = os.path.join(out_dir, 'args.gn.hash')
    ninja_path = os.path.join(out_dir, 'build.ninja')
    needs_gn_gen = args.force_gn_gen or not os.path.exists(ninja_path)
    if not needs_gn_gen:
        if os.path.exists(args_hash_path):
            with open(args_hash_path, 'r') as f:
                old_hash = f.read().strip()
            if old_hash != new_args_hash:
                print("GN arguments changed. Triggering gn gen...")
                needs_gn_gen = True
        else:
            print("GN argument hash missing. Triggering gn gen...")
            needs_gn_gen = True
    if needs_gn_gen:
        print(f"Generating {args_gn_path}...")
        with open(args_gn_path, 'w') as f:
            f.write(new_args_content)
        with open(args_hash_path, 'w') as f:
            f.write(new_args_hash)
        print(f"Running gn gen {out_dir_rel}...")
        run_command(['gn', 'gen', out_dir_rel], cwd=CHROMIUM_SRC_DIR)
    else:
        print(f"GN arguments unchanged and {out_dir_rel} is initialized. Skipping gn gen.")
    print(f"Running autoninja for target: {target} in {out_dir_rel}...")
    run_command(['autoninja', '-C', out_dir_rel, target], cwd=CHROMIUM_SRC_DIR)
    print("Build complete.")

def cmd_run(args):
    """Implements the 'eb run' command."""
    out_dir_name = args.out_dir
    out_dir = os.path.join(CHROMIUM_SRC_DIR, 'out', out_dir_name)
    if not os.path.exists(out_dir):
        print(f"Error: Output directory not found at {out_dir}. Did you run 'eb build'?")
        exit(1)
    if sys.platform == 'win32':
        binary_name = 'chrome.exe'
    else:
        binary_name = 'chrome'
    binary_path = os.path.join(out_dir, binary_name)
    if not os.path.exists(binary_path):
        if sys.platform == 'darwin':
            app_path = os.path.join(out_dir, 'Chromium.app', 'Contents', 'MacOS', 'Chromium')
            if os.path.exists(app_path):
                binary_path = app_path
        if not os.path.exists(binary_path):
            print(f"Error: Chromium binary not found at {binary_path}. Did you build the 'chrome' target?")
            exit(1)
    run_cmd = [binary_path] + args.args
    print(f"Running: {' '.join(run_cmd)}")
    try:
        subprocess.run(run_cmd)
    except KeyboardInterrupt:
        print("\nBrowser execution stopped.")

def main():
    parser = argparse.ArgumentParser(description="Enterprise Browser Build System CLI.")
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    init_parser = subparsers.add_parser("init", help="Initializes the Chromium build environment.")
    init_parser.set_defaults(func=cmd_init)
    sync_parser = subparsers.add_parser("sync", help="Syncs Chromium to base revision and applies patches.")
    sync_parser.set_defaults(func=cmd_sync)
    sync_parser.add_argument("--jobs", type=int, help="Number of parallel jobs for gclient sync.")
    patch_parser = subparsers.add_parser("patch", help="Patch management commands.")
    patch_subparsers = patch_parser.add_subparsers(dest="patch_command", help="Patch sub-commands")
    patch_apply_parser = patch_subparsers.add_parser("apply", help="Applies patches incrementally.")
    patch_apply_parser.set_defaults(func=cmd_patch_apply)
    patch_update_parser = patch_subparsers.add_parser("update", help="Updates patch files based on local changes.")
    patch_update_parser.set_defaults(func=cmd_patch_update)
    patch_update_parser.add_argument("--dry-run", action="store_true", help="Don't write patches, just show what would be done.")
    branding_parser = subparsers.add_parser("branding", help="Applies Enterprise Browser branding to Chromium source.")
    branding_parser.set_defaults(func=cmd_branding)
    build_parser = subparsers.add_parser("build", help="Builds Chromium.")
    build_parser.set_defaults(func=cmd_build)
    build_parser.add_argument("--out-dir", default="Default", help="Output directory name under src/out.")
    build_parser.add_argument("--force-gn-gen", action="store_true", help="Force gn gen to run.")
    run_parser = subparsers.add_parser("run", help="Runs the built Chromium binary.")
    run_parser.set_defaults(func=cmd_run)
    run_parser.add_argument("--out-dir", default="Default", help="Output directory name under src/out.")
    run_parser.add_argument("args", nargs=argparse.REMAINDER, help="Arguments to pass to the binary.")
    args, unknown = parser.parse_known_args()
    if hasattr(args, "func"):
        if args.command == "run":
            args.args = args.args + unknown
        try:
            args.func(args)
        except subprocess.CalledProcessError as e:
            sys.exit(e.returncode)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()