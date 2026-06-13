# SQLUI Persistence Settings Production Config Target Strategy

This document records the decision gate for future production/user config writes from the SQLUI persistence settings Apply path.

The current implementation deliberately keeps real Apply unavailable. SQLUICore has a non-mutating apply entrypoint/result skeleton, UI-safe result display rows, SQLUISamples sample/dev display surfaces, a smoke-owned config target scaffold, and an apply config target policy/resolver skeleton. The policy layer now also exposes `ResolveDocumentedProductionTargetStrategy()` so code and smoke coverage can identify the documented future production target strategy without making it writable. Production/default Apply still does not write config, does not change live runtime settings, does not initialize providers or repositories, and does not create, open, migrate, seed, reset, or delete database files.

PR #136 introduced this strategy as a docs-only decision gate. That strategy checkpoint added no runtime code, settings controls, config writes, committed config changes, provider lifecycle behavior, database work, scripts, Build.cs changes, plugin descriptor changes, maps, assets, CI, or packaged behavior. The follow-up policy-resolution slice keeps those runtime safety boundaries intact.

## Production Config Target Resolution Checkpoint

PR #137 turns the documented strategy into SQLUICore policy resolution without enabling writes. `ResolveDocumentedProductionTargetStrategy()` gives the future project/user target a code-level identity so smoke coverage and future implementations can refer to the same decision point, but the result remains unavailable, non-writable, pathless, and production Apply remains disabled.

PR #138 is the docs-only checkpoint for that policy-resolution slice. It adds no runtime code, scripts, config changes, Build.cs changes, plugin descriptor changes, assets, maps, smoke flags, generated files, packaged outputs, database files, CI, production/user config writes, runtime settings application, settings controls, startup behavior, provider/repository lifecycle behavior, database creation, database write-open, migrations, seed copy, or behavior changes.

This checkpoint proves target resolution only:

- SQLUICore can represent the documented future real target without exposing `DefaultEngine.ini`, generated `Saved/Config`, user/global editor settings, or any other writable production path.
- Default/runtime Apply remains unavailable/not implemented and non-writable.
- Explicit smoke-owned targets under `Saved/SQLUI/SmokeTests` remain the only write-capable targets.
- The future real project/user target remains unavailable/non-writable until a later PR deliberately chooses and validates a real target.
- `-UsePersistenceSettingsDraftProbe` verifies deterministic resolution, repo `Config` / `Saved/Config` preservation, no DB creation, no DB write-open, no migrations, no seed copy, no provider/repository lifecycle work, and smoke-owned cleanup.

It does not add production/user config writes, runtime settings application, committed config changes, settings controls, backend selector UI, SQLite path editing, provider auto-init controls, reset/delete behavior, startup wiring, provider/repository initialization, database work, or file deletion outside smoke-owned cleanup.

## Production Target Question

Real Apply behavior needs a deliberate target decision before implementation. The target must answer where SQLUICore writes the validated persistence settings draft when a user or product chooses Apply.

The current policy skeleton intentionally refuses to infer that target:

- `ResolveDefaultRuntimeTarget()` reports production/default Apply unavailable and non-writable.
- `ResolveExplicitTarget()` can resolve only explicit smoke/test-owned targets under `Saved/SQLUI/SmokeTests`.
- `ResolveDocumentedProductionTargetStrategy()` represents the documented production strategy as a future project/user target kind, but with no writable path, `bCanWrite=false`, and production Apply disabled.
- `ResolveFutureProjectUserConfigTarget()` delegates to the documented production strategy resolution, so the future real project/user target remains unavailable/not implemented.

Smoke-owned targets are not production targets. They exist so commandlet smoke coverage can prove validation, narrow serialization, config preservation, and cleanup without touching project defaults, generated `Saved/Config`, user/global editor settings, provider lifecycle, or SQLite files.

Future production writes must not be inferred implicitly from Unreal config conventions, repository defaults, active backend state, widgets, or command-line options.

## Candidate Target Inventory

| Candidate | Current Decision | Default/Scope Risk | Packaged/Startup Risk | Smoke/Side-Effect Notes |
| --- | --- | --- | --- | --- |
| Committed project defaults, such as `DefaultEngine.ini` or plugin default config | Rejected for runtime Apply | High. This can mutate source-controlled defaults and affect every checkout. | High. It can silently change startup behavior in editor and packaged builds, including SQLite selection or provider auto-init. | Not a smoke target. Runtime Apply must not write committed project or plugin defaults and must not create DB files as a side effect. |
| Generated project `Saved/Config` | Not selected | Medium. It is project-local and usually not committed, but it can still affect future editor/project launches. | Needs separate review. It can affect startup/config load behavior and requires packaged validation before it can be a real target. | Smoke use would need isolated paths, diff/restore checks, and proof that Apply does not create DB files or initialize providers. |
| User-specific Unreal config or global editor settings | Rejected unless a future PR deliberately scopes it | High. It can surprise users, cross projects, or escape the project scope. | Needs separate review if ever scoped; user/global settings can affect future sessions outside SQLUI's current project-owned policy. | Not a smoke target today. SQLUI should not write user/global editor settings unexpectedly and must not use them to enable SQLite/provider auto-init. |
| Existing `USQLUILayoutRepositoryRuntimeSettings` config-backed object | Not selected for runtime writes yet | Medium. It is the current config-backed runtime settings policy surface and has safe defaults, but it is `Config=Game`. | Needs explicit startup impact review and packaged validation if writes can change runtime behavior. | Not a write target yet. Any future use needs explicit target semantics, validation, diff/snapshot checks, and proof that config Apply alone does not create DBs or run provider/repository lifecycle. |
| SQLUICore-owned runtime settings file under `Saved/SQLUI` | Plausible future target, not implemented | Lower than committed defaults if scoped per project/runtime, but no production helper or format exists yet. | Needs packaged path behavior documented and validated before use. | Could become smoke-safe only with a non-smoke production path plus isolated smoke paths. It must prove no DB creation, migration, seed copy, provider init, repository init, or provider auto-init side effect. |
| Plugin-managed settings file under `Saved/SQLUI` | Plausible future target, not implemented | Lower than committed defaults if kept project/plugin scoped, but still needs a SQLUICore-owned policy boundary. | Needs packaged path behavior documented and validated before use. | Must stay separate from `Saved/SQLUI/SmokeTests`, prove cleanup for smoke artifacts, and prove it does not create DBs or initialize providers as a side effect. |
| Existing explicit smoke-owned target under `Saved/SQLUI/SmokeTests` | Acceptable for smoke only | Low when explicitly requested from smoke code. It is not a production/user target. | Not a packaged/runtime policy target. | This is the only write-capable path today. It must stay explicit, temporary, cleaned up, and separated from real production targets; it does not enable SQLite/provider auto-init by default. |
| Continue with no production write target | Recommended now | Lowest. No real settings file is written. | No packaged validation required because no startup/config behavior changes. | Preserves the current safety state while policy/result wording and validation requirements are documented. No DB files, provider lifecycle, migrations, seed copy, or config writes occur. |

## Recommended Next Target Strategy

No existing production target is safe enough to enable implicitly today.

The recommended next step is to keep production/default Apply unavailable and add only policy/result documentation or helper surfaces until a future PR deliberately chooses the real target. That future PR should either:

- define a SQLUICore-owned real target under `Saved/SQLUI` with clear project/user scope and packaged path behavior, or
- explicitly justify another target such as generated project `Saved/Config`, with smoke coverage, config diff/snapshot checks, and packaged validation.

The first real write should be narrow. Prefer backend choice only, or backend plus provider auto-init only if the target and restart/reinitialize messaging are fully represented. SQLite database path editing and writing should be a separate PR because path safety, package behavior, relative path resolution, and default DB creation rules need focused review.

Any real target strategy must preserve:

- `InMemory` as the default backend.
- SQLite as opt-in only.
- Provider auto-init off by default.
- No committed default config changes.
- No default DB creation.
- No provider or repository lifecycle work during config write.
- No migrations, seed copy, reset/delete, or database write-open as side effects of Apply.

## Rejected Unsafe Targets

Future Apply implementations must not:

- write `DefaultEngine.ini` from runtime Apply.
- mutate committed plugin or project defaults.
- write user/global editor settings unexpectedly.
- use widget-owned file or config writes.
- make `Saved/Config` a production target without separate justification and packaged validation.
- choose a target that silently changes startup behavior, provider lifecycle, repository lifecycle, or default backend policy.
- choose a target that creates SQLite database files, directories, migrations, or seed-copy output as a side effect.
- infer a production target from the currently active repository, command-line override, sample surface, or smoke target.

## Enablement Prerequisites

A future PR that enables any real production/user config target must include:

- an explicit SQLUICore target policy update.
- draft validation before write.
- invalid draft refusal without mutation.
- no-change/no-op behavior for unchanged drafts.
- config diff or snapshot checks.
- smoke coverage proving smoke-owned and real target paths remain separate.
- smoke coverage proving committed config remains unchanged.
- smoke coverage proving generated config changes are either absent or deliberately scoped and restored.
- smoke coverage proving no DB files or DB directories are created by config Apply alone.
- smoke coverage proving no provider or repository lifecycle runs during config write.
- smoke coverage proving no migrations or seed copy run during config write.
- user-readable result/status messages for success, no-op, validation failure, unavailable target, and restart/reopen/reinitialize requirements.
- packaged validation if startup, config load, default map behavior, provider lifecycle, repository lifecycle, or packaged runtime behavior can be affected.

## First Minimal Real Write Scope

The first production write slice should not attempt to implement the whole settings screen.

Recommended narrow scopes, in order:

1. backend choice only, if the target is explicit and safe.
2. backend plus provider auto-init only after restart/reinitialize messaging is clear and default-off behavior is preserved.
3. SQLite path only in a later path-specific PR with path validation, relative path policy, packaged path expectations, and no-default-create behavior covered.

Schema initialization, DB creation, seed copy, reset/delete behavior, and provider/repository initialization are not config-write side effects. They remain separate SQLUICore policy/lifecycle actions.

## Smoke And Validation Requirements

Existing `-UsePersistenceSettingsDraftProbe` coverage already confirms the current state:

- default/runtime target unavailable.
- smoke-owned target explicit only.
- future real target unavailable.
- documented production target strategy represented, non-writable, pathless, and deterministic.
- invalid drafts do not mutate the smoke target.
- no-change drafts remain no-op.
- repo `Config` and `Saved/Config` stay unchanged.
- SQLite paths can be previewed without DB creation.
- provider auto-init remains pending policy only.
- provider/repository lifecycle work does not run.
- migrations and seed copy do not run.
- smoke-owned artifacts are removed.

Future real-target PRs must extend this coverage rather than replacing it. They must keep cleanup checks mandatory for smoke-owned artifacts and add explicit checks for any real target artifacts they create.

## Packaged Validation Policy

This strategy checkpoint is docs-only and does not require packaged validation.

Packaged validation becomes required for a future real target when the PR can affect startup/config/default-map/provider lifecycle behavior, packaged runtime config loading, packaged DB path resolution, or provider/repository lifecycle. The packaged validation should continue to preserve the MSVC toolchain guidance in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md).

## Current Decision

Production Apply remains unavailable.

The only write-capable path today is the explicit smoke-owned config target under `Saved/SQLUI/SmokeTests`. The documented production target strategy is represented in SQLUICore policy as a non-writable future project/user target with no real path selected. Future real project/user config targets remain unavailable until a later SQLUICore policy PR chooses and validates a safe target.
