---
name: dondron-session-handoff
description: End-of-session handoff for DonDron — update docs/dev_state.md and nexus vault task. Use when session ends, user asks to wrap up, or before switching tasks.
---

# DonDron session handoff

Run at **end of every agent session** in the `dondron` code repo.

## 1. Update code repo — `docs/dev_state.md`

| Section | Action |
|---------|--------|
| **Last known SIL status** | Add row if SIL was run: date, machine, PX4 target, agent connected, one-line notes |
| **Package progress** | Check off completed packages; note partial work in line item |
| **Pending work** | Strike done items; add next concrete step |
| **Known issues** | Add blockers surfaced; remove resolved items |

Do **not** paste full logs or rosbags — summary only.

## 2. Update vault task

Path: `~/Projects/nexus/01_Projects/Robotics/DonDron/Tasks/<slug>.md`

| Field | Action |
|-------|--------|
| `status` | `in_progress` while working → `done` / `blocked` when finished |
| Checklist | Check off completed steps |
| `## Notes` | Branch name, commit hash (if committed), blockers, next step |

Use full task template from `~/Projects/nexus/99_Templates/Task.md` — never omit frontmatter fields.

After commit: record hash via skill `dondron-git-commit` step 9 or note in handoff summary.

## 3. Do not update unless structural change

- Vault `CONTEXT.md` — only new machine, repo, or pipeline step
- Vault blueprint — architecture stays in vault docs, not duplicated into code

## 4. Confirm to user

Brief summary: what changed, branch/commit if any, vault task id updated, next recommended step.
