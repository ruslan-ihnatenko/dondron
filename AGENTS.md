# DonDron — Agent Guardrails

Protected paths and legal boundaries for Cursor agents working in the **public** `dondron` repo.

Vault references:
- Legal boundary: `~/Projects/nexus/01_Projects/Robotics/DonDron/Docs/legal-compliance-eu-export-control.md`
- Project branches: `~/Projects/nexus/01_Projects/Robotics/DonDron/Docs/project-branches-and-repos.md`
- Blueprint: `~/Projects/nexus/01_Projects/Robotics/DonDron/Docs/hybrid-architecture-blueprint.md`

## Public vs private module split

| Module | Content | This repo |
|--------|---------|-----------|
| **A — Perception** | `/detections`, bbox + range | Yes (`dondron_perception`) |
| **B — Flight API** | Generic setpoint interface to PX4 | Yes (`dondron_flight_api`) |
| **C — Guidance bridge** | Detection → setpoint mapping | **Never** — private repo Line 2+ |

## Protected — never without explicit user approval

- PX4 flight-critical parameters (`COM_RC_OVERRIDE`, `COM_OBL_ACT`, `COM_OBL_RC_ACT`, geofence `GF_*`)
- Adding a `/detections` subscription to `dondron_flight_api` or any flight-control node
- Detection→setpoint mapping, visual servo, intercept controller, ENGAGE BT subtree in **this repo**
- Commented-out guidance templates or stubs (“TODO: implement intercept/guidance here”)
- Launch files that import private packages (`dondron_guidance_private`)
- Private repo URLs in git history, submodules, or CI config
- Committing secrets, API keys, credentials
- Large binary assets: rosbags, calibration datasets, `.rknn` models, mesh binaries

## High-risk zones — extra care, explain changes in PR/commit body

| Area | Risk |
|------|------|
| `dondron_state_machine` | Public states only: IDLE → ARM → TAKEOFF → SEARCH → ACQUIRE → TRACK. No ENGAGE. |
| `dondron_flight_api` | Frame transforms (body/NED), Offboard mode management — must stay detection-agnostic |
| `dondron_bringup` | Launch topology; Line 1 modes: `sil_manual`, `sil_public` only |
| `dondron_bridge` | uXRCE-DDS agent config, baud rate, domain ID |

## DISENGAGE and safety

DISENGAGE is **not** initiated by the behavior tree. PX4 `COM_RC_OVERRIDE` handles RC stick override at the flight-controller level. The BT monitors flight mode via `/fmu/out/vehicle_status` (or equivalent bridged topic).

## Public language (commits, README, issues)

| Prefer | Avoid in public repo |
|--------|----------------------|
| object recognition, visual tracking | terminal guidance |
| object following, target lock | engage, attack, intercept |
| setpoint interface, flight API | weapon, strike |
| advanced tracking (private module) | turnkey guidance |

Private repo and vault may use precise engineering terms internally.

## Session workflow

**Start:** read vault CONTEXT + active task, `docs/dev_state.md`, this file.

**End:** update `docs/dev_state.md` (SIL status, package progress, known issues) and vault task (status, checkboxes, branch/commit). Edit vault CONTEXT only when something structural changed.

**Git:** commit only when asked — follow `.cursor/rules/github.mdc` and skill `dondron-git-commit`.

Full checklist: `~/Projects/nexus/01_Projects/Nexus_Infrastructure/Docs/User Guide/Agent Session Checklist.md`
