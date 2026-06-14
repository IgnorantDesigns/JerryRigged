# SQLUI Persistence Settings Production Config Target Strategy

This document records the decision gate for future production/user config writes from the SQLUI persistence settings Apply path.

The current implementation deliberately keeps real Apply unavailable. SQLUICore has a non-mutating apply entrypoint/result skeleton, UI-safe result display rows, SQLUISamples sample/dev display surfaces, a smoke-owned config target scaffold, and an apply config target policy/resolver skeleton. The policy layer now also exposes `ResolveDocumentedProductionTargetStrategy()` so code and smoke coverage can identify the documented future production target strategy without making it writable. It also exposes a descriptor for the selected future target, `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, and a guarded production target enablement request resolver. The selected target is known in code for validation/display purposes only: it is not write-enabled, not implemented for writes, and no file or directory is created. Production/default Apply still does not write config, does not change live runtime settings, does not initialize providers or repositories, and does not create, open, migrate, seed, reset, or delete database files.

PR #136 introduced this strategy as a docs-only decision gate. That strategy checkpoint added no runtime code, settings controls, config writes, committed config changes, provider lifecycle behavior, database work, scripts, Build.cs changes, plugin descriptor changes, maps, assets, CI, or packaged behavior. The follow-up policy-resolution slice keeps those runtime safety boundaries intact. This design checkpoint chooses the production target shape for a future implementation, but it still does not write to that target or enable production Apply.

## Production Config Target Resolution Checkpoint

PR #137 turns the documented strategy into SQLUICore policy resolution without enabling writes. `ResolveDocumentedProductionTargetStrategy()` gives the future project/user target a code-level identity so smoke coverage and future implementations can refer to the same decision point, but the result remains unavailable, non-writable, and production Apply remains disabled. The follow-up descriptor slice names the selected relative target path while still keeping `ConfigFilePath` empty because no writable production path is exposed.

PR #138 is the docs-only checkpoint for that policy-resolution slice. It adds no runtime code, scripts, config changes, Build.cs changes, plugin descriptor changes, assets, maps, smoke flags, generated files, packaged outputs, database files, CI, production/user config writes, runtime settings application, settings controls, startup behavior, provider/repository lifecycle behavior, database creation, database write-open, migrations, seed copy, or behavior changes.

This checkpoint proves target resolution only:

- SQLUICore can represent the documented future real target without exposing `DefaultEngine.ini`, generated `Saved/Config`, user/global editor settings, or any other writable production path.
- Default/runtime Apply remains unavailable/not implemented and non-writable.
- Explicit smoke-owned targets under `Saved/SQLUI/SmokeTests` remain the only write-capable targets.
- The selected future real project/user target remains unavailable/non-writable until a later PR deliberately implements and validates writes to the descriptor target.
- SQLUICore can describe `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` as SQLUICore/plugin-managed, non-committed, not `DefaultEngine.ini`, not generated `Saved/Config`, and not user/global editor settings.
- The descriptor does not create the target file or parent directory.
- `-UsePersistenceSettingsDraftProbe` verifies deterministic resolution, repo `Config` / `Saved/Config` preservation, no DB creation, no DB write-open, no migrations, no seed copy, no provider/repository lifecycle work, and smoke-owned cleanup.

It does not add production/user config writes, runtime settings application, committed config changes, settings controls, backend selector UI, SQLite path editing, provider auto-init controls, reset/delete behavior, startup wiring, provider/repository initialization, database work, or file deletion outside smoke-owned cleanup.

## Guarded Production Target Enablement Request

The guarded enablement resolver is intentionally policy-only. `ResolveDocumentedProductionTargetStrategyWithEnablement()` accepts an explicit request value so future Apply work can distinguish "no production target requested" from "production target requested but blocked." In this slice both states remain non-writable because the selected descriptor target has no writer implementation yet.

When no enablement request is supplied, the documented production target stays unavailable, non-writable, and production Apply disabled. When an enablement request is supplied, SQLUICore records that request and returns a warning that enablement remains blocked because the selected `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` target is not implemented yet. The result still exposes no writable `ConfigFilePath`, has `bCanWrite=false`, and keeps production Apply disabled.

This guarded request does not:

- create a production/user config target.
- make `DefaultEngine.ini`, generated `Saved/Config`, user/global editor settings, or `USQLUILayoutRepositoryRuntimeSettings` writable from Apply.
- create directories or files.
- write config.
- apply runtime settings.
- create, open, migrate, seed, reset, or delete database files.
- initialize providers or repositories.
- add UI controls.

The purpose is to make the future enablement gate explicit without making the selected target writable prematurely. A later real-write PR must still implement and validate the SQLUICore-owned `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` target before any production/user config write can occur.

## Guarded Production Target Enablement Checkpoint

PR #139 added the guarded SQLUICore production target enablement request/resolution. This checkpoint documents what that implementation proves: SQLUICore can now represent an explicit request to enable the documented production target, record that request, and still block it through policy while the selected target remains unimplemented.

The request remains blocked, non-writable, deterministic, and side-effect free. Production/default Apply remains unavailable/not implemented, real user/runtime config writes remain disabled, and explicit smoke-owned targets remain the only write-capable targets. No committed config was added, no runtime settings are applied, no provider/repository lifecycle behavior runs, no DB files are created or opened for writing, no migrations run, no seed copy runs, and cleanup removes only smoke-owned artifacts.

This design checkpoint chooses the concrete production target strategy for future implementation:

- a SQLUICore-owned, plugin-managed persistence settings file under `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`.

That file is the future storage surface for persistence settings intent only. It is not a database, not a layout repository file, not a seed database, not a migration source, and not a widget-owned artifact. `DefaultEngine.ini`, committed defaults, user/global editor settings, and widget-owned writes remain rejected. Generated `Saved/Config` remains rejected/deferred unless a separate PR justifies it, proves restore/diff behavior, and runs packaged validation when startup/config/default-map/provider lifecycle or packaged runtime behavior can be affected.

The descriptor checkpoint adds code-level target description only. The guarded enablement request still resolves as blocked and non-writable until a future implementation adds a SQLUICore writer/helper for this exact `Saved/SQLUI/PersistenceSettings` target and proves it through smoke coverage.

## Production Target Descriptor Checkpoint

SQLUICore now exposes a descriptor for the selected production target through the apply config target policy. The descriptor identifies:

- symbolic target name: `SQLUI.PersistenceSettings.RuntimeSettings`.
- relative target path: `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`.
- ownership: SQLUICore/plugin-managed.
- production target: true.
- smoke-owned target: false.
- write capability: false.
- implementation/write-enabled state: false.
- committed config, `DefaultEngine.ini`, generated `Saved/Config`, and user/global editor settings usage: false.

This descriptor is not a writer. It does not create `Saved/SQLUI/PersistenceSettings`, does not create `RuntimeSettings.ini`, does not open the file, does not write config, and does not enable production Apply. The policy result keeps the descriptor separate from `ConfigFilePath`; `ConfigFilePath` remains empty for production/default policy results because no writable path is exposed.

## Concrete Production Target Design

The selected future production target is a SQLUICore-owned settings artifact:

```text
Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini
```

Implementation should resolve this relative to `FPaths::ProjectSavedDir()` or the equivalent SQLUICore runtime saved-directory policy. The target directory is project/runtime-owned generated state, not source-controlled config. It must stay separate from:

- `Saved/SQLUI/SmokeTests`, which remains smoke-owned and temporary.
- `Saved/SQLUI/LayoutRepositories`, which remains the default relative area for SQLite layout repository database files.
- `Saved/Config`, which remains rejected/deferred for runtime Apply.
- committed project/plugin config such as `DefaultEngine.ini`.
- user/global editor settings.

The file purpose is narrow: store validated SQLUI persistence settings intent for future runtime policy. It should not contain layout document JSON, SQLite schema data, migration rows, seed database content, or status/cache output.

The first write implementation should store only the minimum validated settings needed to prove the target safely. The recommended first scope is backend selection only. Provider auto-init can follow only after restart/reopen/reinitialize messaging and startup impact are separately represented and validated. SQLite database path, seed database path, schema-create flags, seed-copy flags, reset/delete choices, migration controls, provider/repository lifecycle triggers, and UI control state remain out of the first write scope.

Because this target lives under `Saved/SQLUI`, future runtime database files can continue to live under `Saved/SQLUI/LayoutRepositories` without conflating settings persistence with database contents. A settings file write must not create, open, migrate, seed, reset, or delete any database file as a side effect.

## Production Target Question

Real Apply behavior needs a deliberate target decision before implementation. The target must answer where SQLUICore writes the validated persistence settings draft when a user or product chooses Apply.

The current policy skeleton intentionally refuses to infer that target:

- `ResolveDefaultRuntimeTarget()` reports production/default Apply unavailable and non-writable.
- `ResolveExplicitTarget()` can resolve only explicit smoke/test-owned targets under `Saved/SQLUI/SmokeTests`.
- `ResolveDocumentedProductionTargetStrategy()` represents the documented production strategy as a future project/user target kind, includes the selected non-writable descriptor path, keeps `ConfigFilePath` empty, keeps `bCanWrite=false`, and keeps production Apply disabled.
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
| SQLUICore-owned persistence settings file under `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` | Selected design, not implemented or write-enabled | Lower than committed defaults because it is generated project/runtime state under SQLUI ownership. It still needs a SQLUICore helper/policy implementation before use. | Needs packaged path behavior validated before writes are considered production-ready. | Must stay separate from `Saved/SQLUI/SmokeTests` and `Saved/SQLUI/LayoutRepositories`; must prove no DB creation, migration, seed copy, provider init, repository init, reset/delete, or provider auto-init side effect. |
| Other plugin-managed settings file under `Saved/SQLUI` | Deferred | Similar risk profile, but less precise than the selected `PersistenceSettings/RuntimeSettings.ini` target. | Would need the same packaged path validation. | Do not invent an alternate filename in implementation without updating this strategy and smoke expectations. |
| Existing explicit smoke-owned target under `Saved/SQLUI/SmokeTests` | Acceptable for smoke only | Low when explicitly requested from smoke code. It is not a production/user target. | Not a packaged/runtime policy target. | This is the only write-capable path today. It must stay explicit, temporary, cleaned up, and separated from real production targets; it does not enable SQLite/provider auto-init by default. |
| Continue with no production write target | Current implementation state only | Lowest. No real settings file is written. | No packaged validation required because no startup/config behavior changes. | The design now chooses the future target, but code must remain blocked until a later implementation PR safely writes it. |

## Recommended Next Target Strategy

No existing production target is write-enabled today. The selected future target is the SQLUICore-owned file under `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, and SQLUICore can now describe that target, but production/default Apply must remain unavailable until that target has a concrete SQLUICore writer implementation and smoke coverage.

The recommended next step is to implement that target behind the existing SQLUICore policy boundary, not in widgets or SQLUISamples. That future implementation PR should:

- add a SQLUICore-owned resolver/writer for `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`.
- keep `ResolveDefaultRuntimeTarget()` non-writable unless the future implementation explicitly enables this selected target.
- keep `ResolveDocumentedProductionTargetStrategyWithEnablement()` blocked until the target is implemented and the request is policy-accepted.
- keep explicit smoke-owned targets under `Saved/SQLUI/SmokeTests` separate from the production target.
- add smoke coverage for target path resolution, valid backend-only writes, invalid draft refusal, no-change/no-op, config preservation, no database side effects, no provider/repository lifecycle side effects, and cleanup.

The first real write should be narrow. Prefer backend choice only. Backend plus provider auto-init should wait until the target and restart/reinitialize messaging are fully represented. SQLite database path editing and writing should be a separate PR because path safety, package behavior, relative path resolution, and default DB creation rules need focused review.

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
- the exact selected path `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, unless this strategy is updated first.
- a safety rationale for the path/surface.
- proof the target is not committed config, `DefaultEngine.ini`, or user/global editor settings.
- draft validation before write.
- invalid draft refusal without mutation.
- no-change/no-op behavior for unchanged drafts.
- config diff or snapshot checks.
- smoke coverage proving the default/runtime target behavior remains intentional.
- smoke coverage proving smoke-owned and real target paths remain separate.
- smoke coverage proving committed config remains unchanged.
- smoke coverage proving generated config changes are either absent or deliberately scoped and restored.
- smoke coverage proving no DB files or DB directories are created by config Apply alone.
- smoke coverage proving the settings file write does not create or touch `Saved/SQLUI/LayoutRepositories` unless a separate database lifecycle action explicitly does so.
- smoke coverage proving no provider or repository lifecycle runs during config write.
- smoke coverage proving no migrations or seed copy run during config write.
- user-readable result/status messages for success, no-op, validation failure, unavailable target, and restart/reopen/reinitialize requirements.
- packaged validation if startup, config load, default map behavior, provider lifecycle, repository lifecycle, or packaged runtime behavior can be affected.

## First Minimal Real Write Scope

The first production write slice should not attempt to implement the whole settings screen.

Recommended narrow scopes, in order:

1. backend choice only, written to the selected `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` target after validation.
2. backend plus provider auto-init only after restart/reinitialize messaging is clear and default-off behavior is preserved.
3. SQLite path only in a later path-specific PR with path validation, relative path policy, packaged path expectations, and no-default-create behavior covered.

Schema initialization, DB creation, seed copy, reset/delete behavior, and provider/repository initialization are not config-write side effects. They remain separate SQLUICore policy/lifecycle actions.

## Smoke And Validation Requirements

Existing `-UsePersistenceSettingsDraftProbe` coverage already confirms the current state:

- default/runtime target unavailable.
- smoke-owned target explicit only.
- future real target unavailable.
- documented production target strategy represented, non-writable, descriptor-backed, and deterministic.
- selected production target descriptor resolves to `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, reports SQLUICore/plugin-managed ownership, rejects committed/default/generated/user config surfaces, and does not create the target file or directory.
- guarded production target enablement requests are recorded but remain blocked, non-writable, deterministic, and side-effect free.
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

Packaged validation becomes required for the future implementation that writes this target if the PR can affect startup/config/default-map/provider lifecycle behavior, packaged runtime config loading, packaged DB path resolution, or provider/repository lifecycle. Even if the first implementation is only a backend-only settings file write, packaged validation should be considered before claiming production readiness for packaged runtime settings Apply. The packaged validation should continue to preserve the MSVC toolchain guidance in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md).

## Current Decision

Production Apply remains unavailable.

The only write-capable path today is the explicit smoke-owned config target under `Saved/SQLUI/SmokeTests`. This document selects the future production target design as `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, owned by SQLUICore and used only for validated persistence settings intent. SQLUICore can now describe that selected target, but the current policy still represents the production target as non-writable and exposes no writable `ConfigFilePath` because no implementation writes that selected target yet. Explicit production target enablement can be requested in policy, but that request remains blocked and not accepted until a later SQLUICore policy PR implements and validates the selected target.
