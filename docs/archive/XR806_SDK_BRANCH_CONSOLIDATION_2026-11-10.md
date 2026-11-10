# XR806 SDK branch consolidation — 2026-11-10

Canonical branch: `main`. Keep `Convert-tuya2esphome-ota` intact.

## Integrated active fixes

- [PR #6](https://github.com/bavci1988/xr806-sdk/pull/6) merged with SoftAP DHCP improvements, corrected DNS/UDP 53 ownership, and the WXU experimental 2 MiB sysinfo/preference-sector layout. SDK flash writes, brownout recovery and hardware OTA remain **hardware-unverified**.
- [PR #10](https://github.com/bavci1988/xr806-sdk/pull/10) merged a seven-configuration native ARM test matrix, including normal web server and captive portal, with the public ESPHome fork now checked out from `dev`. Its host low-power policy tests understand `HTTP_BUSY`.

## Preserved, not activated

The following historical branch heads have been retained as Git commit ancestors, resolving their older working-tree content in favor of current `main`:

- `dependabot/github_actions/actions/checkout-7` (`235dce8`)
- `dependabot/github_actions/actions/setup-python-7` (`5e4b4df`)
- `dependabot/github_actions/actions/upload-artifact-7` (`e995a28`)
- `test/trv16-wxu-full-build-20261109` (`136c27d`)

These Dependabot updates target older unpinned major tags; current `main` already has newer exact action tags. Their commits are therefore preserved in the merge ancestry **without downgrading** the workflow.

The TRV16 branch also holds a private 314-line full configuration. Its content remains recoverable from its original commit ancestry; it has **not** been copied into the normal `main` working tree, in order not to publish or automatically process existing private credentials. Its workflow was updated on that historic branch to checkout public ESPHome `dev`.

The completed PR #6 and PR #10 feature branch heads are also ancestors of `main`, not independent work.

## Automated reference deletion

`.github/workflows/xr806-prune-merged-branches.yml` is a narrowly scoped, one-shot `main` push workflow. For each explicitly named head it checks (1) exact known SHA and (2) zero commits ahead of `main` before deleting the ref using repository-scoped `contents: write`. It never targets `main` or `Convert-tuya2esphome-ota`.

If repository policy prevents the workflow token from deleting refs, delete these six verified-integrated branches manually. After a successful cleanup run, remove the one-shot workflow file to eliminate unnecessary future write-capable automation.

## Second Dependabot reconciliation

A single grouped Dependabot branch (`dependabot/github_actions/github-actions-72bae3d27f`, head `62d5641009820b32194baa92789340094a890032`) was created immediately after enabling grouped version updates. Its history is retained as another merge parent. The stage7b workflow now uses exact action versions that are newer or equal to those proposed by the bot; already pinned actions in other workflows are not downgraded.

To avoid recreating disposable GitHub Actions version-PR branches, `.github/dependabot.yml` now sets `open-pull-requests-limit: 0` for **version updates**. This can be changed later; separate security updates are governed by repository security settings. A second narrowly scoped one-shot workflow removes this one grouped branch after an explicit SHA and zero-ahead comparison. Remove the workflow file once it succeeds.
