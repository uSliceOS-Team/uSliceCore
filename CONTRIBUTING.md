# Contributing to µSliceCore

Thank you for helping improve µSliceCore.

The project values small, reviewable changes, explicit behavior, reproducible tests, and documentation that distinguishes verified properties from design expectations.

## Project stage

Until the `v0.1.0` tag is published, `main` is the integration branch for the 0.1.0 release candidate. Functional behavior of the 0.1.0 core is frozen; current work focuses on tests, documentation, and verification of that behavior.

After the 0.1.0 release:

* `main` is protected;
* changes reach shared branches through pull requests;
* functional changes intended for the next release target its designated development branch;
* documentation or test corrections for released behavior must not silently change the documented API contract.

## Before opening a pull request

For defects, portability problems, or API changes, open or reference an Issue first. Small documentation corrections may be submitted directly as a pull request.

Keep each pull request focused on one problem. Explain:

* what behavior or documentation changes;
* why the change is needed;
* which tests provide evidence;
* whether the change affects compatibility or public API behavior.

## Requirements

Core changes must:

* remain compatible with C++17;
* introduce no dynamic allocation in `tasks/` or `time/`;
* preserve the documented cooperative, single-stack execution model unless an approved design issue explicitly changes it;
* include or update relevant tests;
* update `docs/TECHNICAL_REFERENCE.md` when public behavior changes;
* update `tests/TRACEABILITY.md` when test evidence changes;
* update `docs/KNOWN_ISSUES.md` when a limitation is added, changed, or resolved.

Do not introduce timing, portability, safety, MISRA, or regulatory-compliance claims without corresponding evidence.

## Validation

Before submitting, run the checks available in your environment:

* `bash tests/run_tests.sh`
* `bash tests/run_task_size_32.sh`
* `bash tools/cppcheck.sh`
* the host-side example build described in `examples/logic_controlled_tasks/README.md`

The pull request must pass all GitHub Actions jobs.

## Style and commits

Follow `.clang-format` for C and C++ formatting.

Use short, scoped commit messages where practical:

* `fix: ...`
* `test: ...`
* `docs: ...`
* `ci: ...`
* `refactor: ...`

Do not force-push shared or protected branches. When rewriting a private pull-request branch is necessary, use `--force-with-lease` and make sure no one else depends on its current history.

## License

By submitting a contribution, you agree that it may be distributed under the repository’s Apache License 2.0.
