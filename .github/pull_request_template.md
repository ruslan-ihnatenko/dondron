## Summary

<!-- What changed and why (1-3 bullets) -->

-

## Test plan

- [ ] `./scripts/check-public-boundary.sh`
- [ ] `colcon build` (affected packages or full `dondron_*` set)
- [ ] `colcon test` + `colcon test-result --verbose` (if tests added/changed)
- [ ] Main PC SIL smoke (`dondron-sil-smoke-test` skill) — if flight_api, state_machine, or bringup changed
- [ ] M2 TRACK gate — if mission BT or `sil_public.launch.py` changed

## Public boundary

- [ ] No `/detections` subscription in `dondron_flight_api`
- [ ] No ENGAGE / guidance / private package imports

## Learning note (optional)

<!-- If this PR relates to a study topic, link vault cheatsheet -->
