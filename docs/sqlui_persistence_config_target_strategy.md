# SQLUI Persistence Settings Production Config Target Strategy

This document records the decision gate for future production/user config writes from the SQLUI persistence settings Apply path.

The current implementation deliberately keeps broad/default Apply unavailable. SQLUICore has a default non-mutating apply entrypoint/result path, UI-safe result display rows, SQLUISamples sample/dev display surfaces, a smoke-owned config target scaffold, and an apply config target policy/resolver skeleton. The policy layer exposes `ResolveDocumentedProductionTargetStrategy()` so code and smoke coverage can identify the documented production target strategy without inferring unsafe config locations. It also exposes a descriptor for the selected target, `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, and a guarded production target enablement request resolver. The selected target now has one narrow writer: an explicit backend-only `RequestPersistenceSettingsApply` request may create/write that file with only `Backend=<value>`. It also has one narrow reader: an explicit backend-only readback helper can parse that same backend value when caller code asks for it. Production/default Apply still does not write config unless that guarded request is used, readback is not consumed at startup, neither path changes live provider/repository state, and neither path creates, opens, migrates, seeds, resets, or deletes database files.

PR #144 is a documentation-only checkpoint for that backend-only write. It records the #143 behavior and safety boundaries without adding runtime code, scripts, config changes, smoke flags, widget assets, maps, startup wiring, provider lifecycle work, database work, or packaged behavior.

PR #145 added the matching backend-only readback checkpoint. It is a caller-invoked parser only: it reads `Backend` from `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, reports absent/valid/invalid states, and does not apply runtime settings, initialize providers/repositories, create or open databases, run migrations, copy seeds, delete files, or change startup behavior.

PR #136 introduced this strategy as a docs-only decision gate. That strategy checkpoint added no runtime code, settings controls, config writes, committed config changes, provider lifecycle behavior, database work, scripts, Build.cs changes, plugin descriptor changes, maps, assets, CI, or packaged behavior. The follow-up policy-resolution slice keeps those runtime safety boundaries intact. This design checkpoint chooses the production target shape for a future implementation, but it still does not write to that target or enable production Apply.

## Production Config Target Resolution Checkpoint

PR #137 turns the documented strategy into SQLUICore policy resolution without enabling broad writes. `ResolveDocumentedProductionTargetStrategy()` gives the project/runtime target a code-level identity so smoke coverage and scoped implementations can refer to the same decision point, but the resolver result remains unavailable and non-writable for default/general Apply. The follow-up descriptor slice names the selected relative target path while still keeping `ConfigFilePath` empty because no broad writable production path is exposed.

PR #138 is the docs-only checkpoint for that policy-resolution slice. It adds no runtime code, scripts, config changes, Build.cs changes, plugin descriptor changes, assets, maps, smoke flags, generated files, packaged outputs, database files, CI, production/user config writes, runtime settings application, settings controls, startup behavior, provider/repository lifecycle behavior, database creation, database write-open, migrations, seed copy, or behavior changes.

This checkpoint proves target resolution only:

- SQLUICore can represent the documented selected target without exposing `DefaultEngine.ini`, generated `Saved/Config`, user/global editor settings, or any broad writable production path.
- Default/runtime Apply remains unavailable/not implemented and non-writable unless the explicit backend-only production request is used.
- Explicit smoke-owned targets under `Saved/SQLUI/SmokeTests` remain the only full-draft write-capable targets.
- The selected project/runtime target is implemented only for backend-value writes; broader writes remain unavailable.
- SQLUICore can describe `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` as SQLUICore/plugin-managed, non-committed, not `DefaultEngine.ini`, not generated `Saved/Config`, and not user/global editor settings.
- The descriptor does not create the target file or parent directory.
- `-UsePersistenceSettingsDraftProbe` verifies deterministic resolution, repo `Config` / `Saved/Config` preservation, no DB creation, no DB write-open, no migrations, no seed copy, no provider/repository lifecycle work, and smoke-owned cleanup.

It does not add production/user config writes, runtime settings application, committed config changes, settings controls, backend selector UI, SQLite path editing, provider auto-init controls, reset/delete behavior, startup wiring, provider/repository initialization, database work, or file deletion outside smoke-owned cleanup.

## Backend-Only Production Apply Write Checkpoint

PR #143 is the first selected-target write checkpoint. It keeps the target and enablement gates from the earlier checkpoints, but adds one deliberately narrow mutation: an explicit guarded backend-only `RequestPersistenceSettingsApply` request may create/write `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` with only:

```ini
[SQLUI.PersistenceSettings]
Backend=<value>
```

This is not broad settings Apply. The default runtime path, status surfaces, descriptor resolution, and guarded enablement resolution do not silently create the file or parent directory. The write path exists only when the caller requests backend-only production Apply and the production target is explicitly enabled for that request.

The exact write scope is backend only. The writer does not serialize SQLite path, JSON file directory, provider auto-init, migration, seed-copy, schema-init, reset/delete, provider/repository lifecycle, or UI-control state. Writing `Backend=SQLite` records only the selected backend value; it does not create a SQLite database, create database directories, initialize a provider or repository, run migrations, copy seeds, open SQLite for writing, or change startup behavior. Applying that backend choice may require a future restart, reopen, provider reset, or reinitialization flow, but #143 does not perform those lifecycle actions.

`-UsePersistenceSettingsDraftProbe` is the validation path for this checkpoint. It verifies that default/no-request Apply does not write, descriptor and guarded enablement resolution do not create the target, explicit backend-only Apply can create the target, the generated file contains only the expected backend value, invalid drafts are refused without mutation, no-change requests are no-ops, provider auto-init changes are rejected for this path, repo `Config` and generated `Saved/Config` remain unchanged, no DB/lifecycle/migration/seed-copy work runs, and smoke cleanup removes the probe-created `RuntimeSettings.ini` plus the empty `PersistenceSettings` directory when the probe created it.

## Backend-Only Runtime Settings Readback Checkpoint

PR #145 gives the selected target an explicit backend-only readback helper. `FSQLUIPersistenceSettingsRuntimeSettingsReader` reads `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` only when caller code asks for it, parses only the `Backend` value from `[SQLUI.PersistenceSettings]`, and reports a structured result.

This readback is not startup consumption. It is not wired into module startup, subsystem initialization, provider initialization, repository factory defaults, widgets, maps, timers, polling, or config-backed runtime settings. An absent `RuntimeSettings.ini` is a neutral no-override result and does not create the file or parent directory. A valid backend value is returned as parsed data only; it is not applied to the active provider, repository, or runtime settings. Missing, malformed, or unknown backend values are reported as validation/errors without repairing, deleting, rewriting, or falling back silently.

The reader intentionally stays backend-only. It does not read or interpret SQLite database path, provider auto-init, migration, seed-copy, schema-init, reset/delete, startup, lifecycle, or UI-control settings. It does not create database files, open SQLite for writing, run migrations, copy seeds, initialize providers/repositories, or delete files.

`-UsePersistenceSettingsDraftProbe` now validates this readback alongside the backend-only write: absent-file reads create nothing, readback after a smoke-owned backend-only write returns the backend value, unknown/missing backend content is rejected without mutation, repo `Config` and generated `Saved/Config` snapshots remain unchanged, no runtime/provider/database side effects occur, and the probe-created `RuntimeSettings.ini` plus empty parent directory are removed.

Future expansion order should stay conservative:

1. Keep this backend-only write/readback pair as the checkpointed base.
2. Add any broader settings merge/preview behavior as caller-invoked read-only reporting first, with no startup consumption or provider lifecycle work.
3. Add startup consumption separately only after the merge/preview semantics are smoke-tested.
4. Add provider auto-init writes only after restart/reopen/reinitialize messaging and default-off behavior are represented.
5. Add SQLite path writes only in a separate path-safety PR with relative/absolute path policy, packaged path expectations, and no-default-create behavior covered.
6. Add backend selector UI only after the SQLUICore behavior is fully smoke-tested.
7. Run packaged validation before startup, packaged runtime, default-map, provider lifecycle, or runtime config consumption starts reading this file.

## Guarded Production Target Enablement Request

The guarded enablement resolver is intentionally policy-only. `ResolveDocumentedProductionTargetStrategyWithEnablement()` accepts an explicit request value so future Apply work can distinguish "no production target requested" from "production target requested but blocked." The resolver itself remains non-writable and side-effect free; the separate backend-only apply request is the only selected-target writer.

When no enablement request is supplied, the documented production target stays unavailable, non-writable, and production Apply disabled. When an enablement request is supplied to the resolver, SQLUICore records that request but still exposes no general writable `ConfigFilePath` and keeps the broad target policy blocked. The backend-only apply path is separate: it uses an explicit apply request and writes only the `Backend` value to the selected `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` target.

This guarded request does not:

- create a production/user config target.
- make `DefaultEngine.ini`, generated `Saved/Config`, user/global editor settings, or `USQLUILayoutRepositoryRuntimeSettings` writable from Apply.
- create directories or files.
- write config.
- apply runtime settings.
- create, open, migrate, seed, reset, or delete database files.
- initialize providers or repositories.
- add UI controls.

The purpose is to make the enablement gate explicit without turning descriptor resolution into a broad writer. Future real-write PRs must still implement and validate each additional setting before any production/user config write beyond backend-only can occur.

## Guarded Production Target Enablement Checkpoint

PR #139 added the guarded SQLUICore production target enablement request/resolution. This checkpoint documents what that implementation proves: SQLUICore can represent an explicit request to enable the documented production target, record that request, and still block broad target writes through policy while allowing later scoped writers to opt into a narrower path.

The resolver request remains blocked, non-writable, deterministic, and side-effect free. Production/default Apply remains unavailable/not implemented unless the explicit backend-only request is used, and explicit smoke-owned targets remain the only full-draft write-capable targets. No committed config was added, no live runtime settings are applied, no provider/repository lifecycle behavior runs, no DB files are created or opened for writing, no migrations run, no seed copy runs, and cleanup removes only smoke-owned or probe-created artifacts.

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
- general write capability: false.
- implementation/write-enabled state: backend-only writes are handled by the explicit apply request, not by descriptor resolution.
- committed config, `DefaultEngine.ini`, generated `Saved/Config`, and user/global editor settings usage: false.

This descriptor is not a writer. It does not create `Saved/SQLUI/PersistenceSettings`, does not create `RuntimeSettings.ini`, does not open the file, does not write config, and does not enable production Apply. The policy result keeps the descriptor separate from `ConfigFilePath`; `ConfigFilePath` remains empty for production/default policy results because no writable path is exposed.

## Concrete Production Target Design

The selected production target is a SQLUICore-owned settings artifact:

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

The first write implementation stores only the minimum validated setting needed to prove the target safely: backend selection. Provider auto-init can follow only after restart/reopen/reinitialize messaging and startup impact are separately represented and validated. SQLite database path, seed database path, schema-create flags, seed-copy flags, reset/delete choices, migration controls, provider/repository lifecycle triggers, and UI control state remain out of the first write scope.

Because this target lives under `Saved/SQLUI`, future runtime database files can continue to live under `Saved/SQLUI/LayoutRepositories` without conflating settings persistence with database contents. A settings file write must not create, open, migrate, seed, reset, or delete any database file as a side effect.

## Production Target Question

Real Apply behavior needs a deliberate target decision before implementation. The target must answer where SQLUICore writes the validated persistence settings draft when a user or product chooses Apply.

The current policy skeleton intentionally refuses to infer that target:

- `ResolveDefaultRuntimeTarget()` reports production/default Apply unavailable and non-writable.
- `ResolveExplicitTarget()` can resolve only explicit smoke/test-owned targets under `Saved/SQLUI/SmokeTests`.
- `ResolveDocumentedProductionTargetStrategy()` represents the documented production strategy, includes the selected descriptor path, keeps `ConfigFilePath` empty, and keeps broad production Apply disabled.
- `ResolveFutureProjectUserConfigTarget()` delegates to the documented production strategy resolution, so broad project/runtime target selection remains unavailable/not implemented outside separately scoped writers.

Smoke-owned targets are not production targets. They exist so commandlet smoke coverage can prove validation, narrow serialization, config preservation, and cleanup without touching project defaults, generated `Saved/Config`, user/global editor settings, provider lifecycle, or SQLite files.

Future production writes must not be inferred implicitly from Unreal config conventions, repository defaults, active backend state, widgets, or command-line options.

## Candidate Target Inventory

| Candidate | Current Decision | Default/Scope Risk | Packaged/Startup Risk | Smoke/Side-Effect Notes |
| --- | --- | --- | --- | --- |
| Committed project defaults, such as `DefaultEngine.ini` or plugin default config | Rejected for runtime Apply | High. This can mutate source-controlled defaults and affect every checkout. | High. It can silently change startup behavior in editor and packaged builds, including SQLite selection or provider auto-init. | Not a smoke target. Runtime Apply must not write committed project or plugin defaults and must not create DB files as a side effect. |
| Generated project `Saved/Config` | Not selected | Medium. It is project-local and usually not committed, but it can still affect future editor/project launches. | Needs separate review. It can affect startup/config load behavior and requires packaged validation before it can be a real target. | Smoke use would need isolated paths, diff/restore checks, and proof that Apply does not create DB files or initialize providers. |
| User-specific Unreal config or global editor settings | Rejected unless a future PR deliberately scopes it | High. It can surprise users, cross projects, or escape the project scope. | Needs separate review if ever scoped; user/global settings can affect future sessions outside SQLUI's current project-owned policy. | Not a smoke target today. SQLUI should not write user/global editor settings unexpectedly and must not use them to enable SQLite/provider auto-init. |
| Existing `USQLUILayoutRepositoryRuntimeSettings` config-backed object | Not selected for runtime writes yet | Medium. It is the current config-backed runtime settings policy surface and has safe defaults, but it is `Config=Game`. | Needs explicit startup impact review and packaged validation if writes can change runtime behavior. | Not a write target yet. Any future use needs explicit target semantics, validation, diff/snapshot checks, and proof that config Apply alone does not create DBs or run provider/repository lifecycle. |
| SQLUICore-owned persistence settings file under `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` | Selected design, implemented for backend-only writes and explicit backend-only readback | Lower than committed defaults because it is generated project/runtime state under SQLUI ownership. It is currently limited to a single backend value. | Needs packaged path behavior validated before broader writes or startup consumption are considered production-ready. | Must stay separate from `Saved/SQLUI/SmokeTests` and `Saved/SQLUI/LayoutRepositories`; current smoke coverage proves backend-only write/no-op/refusal behavior, absent/valid/invalid backend-only readback behavior, and no DB creation, migration, seed copy, provider init, repository init, reset/delete, provider auto-init, or SQLite path serialization/read side effect. |
| Other plugin-managed settings file under `Saved/SQLUI` | Deferred | Similar risk profile, but less precise than the selected `PersistenceSettings/RuntimeSettings.ini` target. | Would need the same packaged path validation. | Do not invent an alternate filename in implementation without updating this strategy and smoke expectations. |
| Existing explicit smoke-owned target under `Saved/SQLUI/SmokeTests` | Acceptable for smoke only | Low when explicitly requested from smoke code. It is not a production/user target. | Not a packaged/runtime policy target. | This remains the only full-draft write-capable path. It must stay explicit, temporary, cleaned up, and separated from the production target; it does not enable SQLite/provider auto-init by default. |
| Continue with no production write target | Historical pre-backend-write state only | Lowest. No real settings file is written. | No packaged validation required because no startup/config behavior changes. | Superseded by the backend-only write slice; still useful as the fallback policy for any setting outside the scoped backend value. |

## Recommended Next Target Strategy

The selected production target is now write-enabled only for backend choice. It is the SQLUICore-owned file under `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, and SQLUICore can describe that target, but production/default Apply must remain unavailable for every setting beyond the backend value until additional scopes have concrete SQLUICore writer implementation and smoke coverage.

The recommended next step is to implement that target behind the existing SQLUICore policy boundary, not in widgets or SQLUISamples. That future implementation PR should:

- extend the SQLUICore-owned resolver/writer for `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini` only through separately scoped settings.
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
- broad/default production target policy unavailable.
- documented production target strategy represented, non-writable for broad/default Apply, descriptor-backed, and deterministic.
- selected production target descriptor resolves to `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, reports SQLUICore/plugin-managed ownership, rejects committed/default/generated/user config surfaces, and does not create the target file or directory by descriptor resolution alone.
- guarded production target enablement requests are recorded, deterministic, and side-effect free at the resolver layer.
- explicit backend-only production apply can create/write the selected target with only `Backend=<value>` and cleans up the probe-created file.
- explicit backend-only readback treats an absent target as no override, reads a valid backend value without applying it, rejects missing/unknown backend values, and cleans up probe-created invalid/valid target files.
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

Packaged validation becomes required for future implementations that make startup/config/default-map/provider lifecycle behavior, packaged runtime config loading, packaged DB path resolution, or provider/repository lifecycle consume this target. Backend-only writes and explicit backend-only readback remain editor-smoke-covered checkpoints by themselves because they do not change startup or packaged behavior. The packaged validation should continue to preserve the MSVC toolchain guidance in [`sqlui_packaged_build_validation.md`](sqlui_packaged_build_validation.md).

## Current Decision

Production Apply remains unavailable.

The only full-draft write-capable path today is the explicit smoke-owned config target under `Saved/SQLUI/SmokeTests`. This document selects the production target design as `Saved/SQLUI/PersistenceSettings/RuntimeSettings.ini`, owned by SQLUICore and used only for validated persistence settings intent. SQLUICore can describe that selected target, write only the backend value through an explicit guarded apply request, and read back only that backend value through an explicit caller-invoked helper. The current policy still exposes no broad writable production `ConfigFilePath`; startup consumption, SQLite path, provider auto-init, migration/seed/reset/delete settings, and lifecycle behavior require later SQLUICore policy PRs with focused validation.
