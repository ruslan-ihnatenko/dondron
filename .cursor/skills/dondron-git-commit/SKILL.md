---
name: dondron-git-commit
description: DonDron git commit workflow — classify changes by type/scope, split commits when needed, Conventional Commits via HEREDOC. Use when user asks to commit, push, create a PR, or stage changes for git.
---

# DonDron git commit workflow

Reference: `.cursor/rules/github.mdc`

## When to run

- User explicitly asks to **commit**, **push**, or **open a PR**
- **Never** commit proactively

## Step 1 — Inspect (run in parallel)

```bash
git status
git diff                    # unstaged
git diff --cached           # staged
git log -3 --oneline        # match existing style
```

Check branch tracks remote; note if ahead/behind.

## Step 2 — Classify changes

Group changed files using the routing table in `github.mdc`:

| Paths | Type | Scope |
|-------|------|-------|
| `docker/`, `.devcontainer/` | `env` | `docker` |
| `simulation/worlds/` | `sim` | `worlds` |
| `ros2_ws/src/dondron_<pkg>/` | `feat`/`fix`/`model` | matching scope |
| `docs/` only | `docs` | `repo` |
| `.cursor/` only | `chore` | `cursor` |
| `AGENTS.md`, `README.md` | `docs`/`chore` | `repo` |

- **New capability** → `feat`
- **Bug fix** → `fix`
- **URDF/SDF/mesh only** → `model`
- **Dev tooling / rules / skills** → `chore(cursor)` or `env(docker)`

## Step 3 — Decide split vs single commit

**Split** when changes span:

- Different types (`feat` + `docs`, `feat` + `chore`)
- Unrelated scopes (`perception` + `flight-api`) unless one atomic integration
- Agent toolkit + application code

**Single commit** when:

- One package, one logical change
- Docs update tightly coupled to same feature (prefer split unless trivial one-liner README)

**Never stage:** `ros2_ws/build/`, `install/`, `log/`, secrets, rosbags, `.rknn`, large binaries.

## Step 4 — Draft message

Format:

```
<type>(<scope>): <imperative subject ≤72 chars>

<body: 1-2 sentences on why, not file list>
```

Checklist:

- [ ] Subject is imperative, lowercase, no trailing period
- [ ] Scope is smallest match
- [ ] Body explains **why** when not obvious
- [ ] Public language per `AGENTS.md` (no intercept/engage/terminal guidance)
- [ ] No vault task IDs or inline TODOs in message

## Step 5 — Stage and commit

Stage **only** files for this commit:

```bash
git add <paths-for-this-commit>
git commit -m "$(cat <<'EOF'
feat(perception): publish stub Detection2DArray on /detections

Enables SIL bringup before RKNN inference is wired.
EOF
)"
```

If hook modifies files: fix and create a **new** commit (do not amend unless user rules allow).

Repeat steps 4–5 for each split commit.

## Step 6 — Verify

```bash
git status
git log -1 --format=full
```

Confirm clean working tree (or report remaining unstaged changes for next commit).

## Step 7 — Push (only if user asked)

```bash
git push -u origin HEAD   # new branch
git push                  # existing tracking branch
```

## Step 8 — Pull request (only if user asked)

```bash
git status
git log <base>...HEAD --oneline
git diff <base>...HEAD
gh pr create --title "<type>(<scope>): <subject>" --body "$(cat <<'EOF'
## Summary
- ...

## Test plan
- [ ] colcon build
- [ ] ...
EOF
)"
```

Return PR URL to user.

## Step 9 — Session handoff (if end of session)

After commit: note hash in `docs/dev_state.md` and vault task `## Notes`.

## Safety

- Never `git config` changes
- Never `--no-verify`, `--force` push to `main`, or amend pushed commits unless user explicitly requests
- Warn if user asks to commit secrets or large binaries
