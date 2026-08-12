#!/usr/bin/env python3
"""Block workspace-wide Glob searches that hang Cursor Agent.

Cursor can freeze on Glob/Grep with no timeout (see
https://forum.cursor.com/t/agent-is-stuck-on-searching-files-ext-in-dir-name/165788).
The hang in this repo showed up as:

    Searching files **/*.{log,txt,md} in VPN

Fail open on any parse/runtime error so a hook bug cannot freeze the agent.
"""
from __future__ import annotations

import json
import os
import sys


def _load() -> dict:
    raw = sys.stdin.read()
    if not raw.strip():
        return {}
    return json.loads(raw)


def _norm(path: str) -> str:
    return os.path.normpath(os.path.abspath(path))


def _workspace_roots(payload: dict) -> list[str]:
    roots = []
    for key in ("workspace_roots", "workspaceRoots"):
        val = payload.get(key) or []
        if isinstance(val, list):
            roots.extend(str(item) for item in val if item)
    cwd = payload.get("cwd")
    if cwd:
        roots.append(str(cwd))
    out = []
    seen = set()
    for root in roots:
        n = _norm(root)
        if n not in seen:
            seen.add(n)
            out.append(n)
    return out


def _is_workspace_wide(target: str, roots: list[str]) -> bool:
    t = (target or "").strip()
    if not t or t in (".", "./", "/"):
        return True
    t_abs = _norm(t)
    for root in roots:
        if t_abs == root:
            return True
        if os.path.normpath(t) == os.path.basename(root):
            return True
    return False


def _is_recursive_glob(pattern: str) -> bool:
    p = (pattern or "").strip()
    if not p:
        return False
    if p in ("*", "**", "**/*", "**/**"):
        return True
    if "**" in p:
        return True
    if p.startswith("*.") and "{" in p:
        return True
    return False


def _tool_name(payload: dict) -> str:
    return str(
        payload.get("tool_name")
        or payload.get("toolName")
        or payload.get("tool")
        or ""
    ).lower()


def _tool_input(payload: dict) -> dict:
    inp = payload.get("tool_input") or payload.get("toolInput") or {}
    return inp if isinstance(inp, dict) else {}


def _deny(agent_message: str, user_message: str) -> dict:
    return {
        "permission": "deny",
        "agent_message": agent_message,
        "user_message": user_message,
    }


def decide(payload: dict) -> dict:
    name = _tool_name(payload)
    if name not in ("glob", "globtool", "searchfiles", "search_files"):
        return {"permission": "allow"}

    inp = _tool_input(payload)
    pattern = str(
        inp.get("glob_pattern")
        or inp.get("globPattern")
        or inp.get("pattern")
        or inp.get("glob")
        or ""
    )
    target = str(
        inp.get("target_directory")
        or inp.get("targetDirectory")
        or inp.get("path")
        or inp.get("dir")
        or ""
    )
    roots = _workspace_roots(payload)

    if _is_workspace_wide(target, roots) and _is_recursive_glob(pattern):
        return _deny(
            "Workspace-wide Glob is blocked in this repo because it hangs Cursor "
            "(searching **/*.{log,txt,md} in VPN never returns). "
            "Use Glob with target_directory set to a Task folder "
            "(Task5, Task6, Task7, Task8, Task9, Task10, or Информация), "
            "or Read/Grep a specific file. Do not retry the same workspace-wide Glob.",
            "Blocked a workspace-wide file search that can freeze Cursor Agent.",
        )
    return {"permission": "allow"}


def main() -> None:
    try:
        payload = _load()
        result = decide(payload)
    except Exception:
        result = {"permission": "allow"}
    json.dump(result, sys.stdout, ensure_ascii=False)
    sys.stdout.write("\n")


if __name__ == "__main__":
    main()
