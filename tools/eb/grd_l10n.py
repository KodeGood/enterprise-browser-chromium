"""Branding replacements for .grd, .grdp, and .xtb localization files.

Owns the regex list used to rewrite Chromium strings into Enterprise Browser
strings, plus the file map describing which chromium sources are rebased into
which enterprise_browser paths.
"""

import glob
import os
import re


# Branding replacements applied to file content. Order matters: more specific
# patterns come first so they take precedence over broader ones. Negative
# lookaheads preserve compound terms whose original spelling must survive
# (ChromiumOS, ChromeOS, Chromebook, Chromecast, ChromeVox, ChromeLabs).
#
# Known limitation: when these patterns are applied to .xtb translations,
# locales that transliterate the product name into their own script (e.g.
# Hindi "क्रोमियम", Russian "Хром") aren't touched at all, leaving the
# original Chromium branding in those translations. Treated as a best-effort
# fallback until a real translation pipeline (e.g. Crowdin) is layered on top.
BRANDING_REPLACEMENTS = [
    (r'The\sChromium\sAuthors', r'The Enterprise Browser Authors'),
    (r'Google\sChrome', r'Enterprise Browser'),
    (r'Chromium(?!OS)', r'Enterprise Browser'),
    (r'Chrome(?!OS|book|cast|Vox|Labs)', r'Enterprise Browser'),
]


# Rebase map: each entry describes one or more files to read from the chromium
# source tree, transform with the branding replacements, and write into the
# enterprise_browser tree.
#
#   src             - path relative to chromium src/ (single file)
#   src_glob        - glob relative to chromium src/ (many files)
#   dest            - path relative to enterprise_browser/ (single file)
#   dest_from_src   - callable(src_rel) -> dest_rel (for src_glob entries)
#   path_renames    - list of (find, replace) plain-string substitutions
#                     applied to the file content after branding replacements.
#                     Used to rewrite embedded file/path references inside
#                     a .grd file (e.g. its <file path="..."/> entries).
FILE_MAPPINGS = [
    {
        'src': 'chrome/app/chromium_strings.grd',
        'dest': 'app/enterprise_browser_strings.grd',
        'path_renames': [
            ('resources/chromium_strings_',
             'resources/enterprise_browser_strings_'),
            ('settings_chromium_strings.grdp',
             'settings_enterprise_browser_strings.grdp'),
        ],
    },
    {
        'src': 'chrome/app/settings_chromium_strings.grdp',
        'dest': 'app/settings_enterprise_browser_strings.grdp',
    },
    {
        'src': 'components/components_chromium_strings.grd',
        'dest': 'components/components_enterprise_browser_strings.grd',
    },
    {
        'src_glob': 'chrome/app/resources/chromium_strings_*.xtb',
        'dest_from_src': lambda rel: os.path.join(
            'app', 'resources',
            os.path.basename(rel).replace(
                'chromium_strings_', 'enterprise_browser_strings_')),
    },
]


def apply_branding(text):
    """Returns text with all branding replacements applied."""
    for (pattern, repl) in BRANDING_REPLACEMENTS:
        text = re.sub(pattern, repl, text)
    return text


def rebase_file(src_path, dest_path, path_renames=None):
    """Reads src_path, applies branding (and optional path renames), writes
    the result to dest_path. Uses utf-8 and preserves existing line endings."""
    with open(src_path, 'r', encoding='utf-8', newline='') as f:
        content = f.read()
    content = apply_branding(content)
    if path_renames:
        for (find, replace) in path_renames:
            content = content.replace(find, replace)
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    with open(dest_path, 'w', encoding='utf-8', newline='') as f:
        f.write(content)


def iter_rebase_entries(chromium_src_dir, enterprise_browser_dir):
    """Yields (src_abs, dest_abs, path_renames) tuples for every file the
    rebase command needs to process. Missing source files are silently
    skipped here; the caller is expected to warn about them."""
    for mapping in FILE_MAPPINGS:
        path_renames = mapping.get('path_renames')
        if 'src' in mapping:
            src = os.path.join(chromium_src_dir, mapping['src'])
            dest = os.path.join(enterprise_browser_dir, mapping['dest'])
            yield src, dest, path_renames
        elif 'src_glob' in mapping:
            pattern = os.path.join(chromium_src_dir, mapping['src_glob'])
            for src in sorted(glob.glob(pattern)):
                rel = os.path.relpath(src, chromium_src_dir)
                dest_rel = mapping['dest_from_src'](rel)
                dest = os.path.join(enterprise_browser_dir, dest_rel)
                yield src, dest, path_renames
