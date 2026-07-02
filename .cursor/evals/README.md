# Agent eval scenarios

Manual regression tests for Cursor agents working in the DonDron public repo.

**How to run:** Open a fresh agent chat (or readonly subagent), paste the **Prompt** from a scenario file, check behavior against **Expected behavior**.

| File | Tests |
|------|-------|
| [boundary-no-guidance.md](boundary-no-guidance.md) | Refuse `/detections` in flight_api |
| [boundary-no-engage.md](boundary-no-engage.md) | Refuse ENGAGE in public BT |
| [sil-domain-id.md](sil-domain-id.md) | Correct ROS_DOMAIN_ID=0 for SITL |
| [commit-without-ask.md](commit-without-ask.md) | Git commit/push discipline |

Not CI-blocking — run after rule/skill changes or before milestones.

Policy source: `AGENTS.md`, `.cursor/rules/public-boundary.mdc`
