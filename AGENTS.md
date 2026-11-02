# Instructions for AI Agents

Before doing any work in this repository:

1. Read `AGENT_HANDOFF.md` in full. It is the shared, current project handoff and must be kept up to date.
2. Read the relevant documents under `docs/`, especially `docs/DEVELOPMENT_STATUS.md`, `docs/ROADMAP.md`, `docs/ARCHITECTURE.md`, and `docs/BUILDING.md`.
3. Inspect `git status --short --branch`, the current branch, recent commits, and the exact working-tree diff. Treat all pre-existing modifications and untracked files as user/other-agent work: do not discard, reset, overwrite, stage, or commit them unless explicitly authorized.
4. Verify the current source, build environment, and device state before relying on historical notes. Distinguish user-reported hardware results from tests you personally ran.

## Required handoff discipline

- Keep `AGENT_HANDOFF.md` updated as work progresses, not just at the end.
- Before handing off, record the task completed, files changed, tests actually run and their results, commit SHA, push status, remaining risks, and the next concrete action.
- Record failures and incomplete tests honestly. Never claim a build, flash, or physical test passed unless verified.
- Commit and push only changes you own. Review the staged diff before committing; never include unrelated pre-existing work.
- Work directly on the designated project branch (`main`) unless the user or repository workflow directs otherwise. Do not create a PR or change branches without authorization.
- Never commit Wi-Fi passwords, API keys, tokens, pairing/login codes, or private endpoint credentials.
- Preserve the existing SenseCAP Indicator D1 / ESP32-S3 architecture, LVGL UI-thread safety, bounded network operations, and no-extra-hardware project constraint.
- Do not flash hardware until the connected port is positively identified. COM21 was the verified ESP32-S3 port in prior tests; COM22 was intentionally left untouched. Re-identify before any future flash.

## Source of truth

Use the repository's current code and Git state as the source of truth for implementation. Use `AGENT_HANDOFF.md` for the latest cross-agent summary, and the detailed documents in `docs/` for architecture, build, status, and roadmap information. If documents disagree, verify against code/history and update the stale documentation.
