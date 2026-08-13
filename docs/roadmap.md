# Cim Roadmap

Last updated: 2026-08-14

This roadmap tracks engineering priorities for Cim. It is not a release-date
commitment. Work is ordered by safety, reliability, plugin compatibility,
performance, and maintainability under large-scale and extreme conditions.

## Current Baseline

Cim currently provides:

- a versioned C-facing input method API and an Ada core;
- `static-pic` and `relocatable` GNAT stand-alone library builds;
- a position-independent `libcim.a` as the only supported static artifact;
- runtime loading of one active Cim plugin with `RTLD_LOCAL` isolation;
- plugin ABI validation, required-vtable validation, initialization failure
  handling, reference-counted lifetime management, and unload after the last
  context;
- GTK 3 and Qt 6 input method bridge modules;
- a native libhangul sample plugin;
- task-local error reporting and explicit contract-violation handling;
- Unicode scalar-value indexing with Clair-based conversion and validation; and
- process, failure-path, contract, lifecycle, and multithreaded runtime tests.

## Completed Priorities 1-5

Core embedding, plugin isolation, and toolkit bridge hardening are complete.

The completed core hardening establishes:

- automatic GNAT lifecycle handling for `static-pic` embedding and relocatable
  stand-alone-library initialization;
- private symbol isolation for embedded Cim, Clair, and GNAT objects;
- an explicit plugin loading and closing state machine with external loader and
  plugin calls outside protected actions;
- balanced plugin attachment lifecycle across independent Cim hosts; and
- nested-host regression coverage for two embedded hosts, a shared inner
  plugin DSO, failure isolation, repeated teardown, and a separately loaded
  `libcim.so`.

The completed toolkit bridge hardening establishes:

- documented callback-table and `user_data` ownership, arbitrary-thread
  callback delivery, per-context mutable state, and toolkit-owner-thread
  dispatch;
- explicit GTK simple-context fallback and Qt invalid-state behavior when Cim
  input-context creation fails, without passing null handles to Cim APIs;
- owned deferred callback payloads plus teardown rules that prevent queued or
  in-flight callbacks from accessing destroyed bridge objects;
- synchronous cross-thread surrounding-text and deletion handling with
  deterministic teardown;
- checked rectangle, UTF-8, Unicode scalar, UTF-16, Pango, candidate-table, and
  candidate-window conversions; and
- bridge-level regression coverage for simultaneous contexts, creation
  failure, cross-thread delivery, pending teardown, supplementary Unicode,
  surrounding text, deletion ranges, preedit attributes, and candidate bounds.

These properties remain regression requirements and are not active design
items.

## 2.1.x Maintenance Policy

Cim 2.1.x is maintenance-only. No 2.2.0 release is planned.

- Preserve the published Cim 2.1 C ABI and Ada source interfaces.
- Limit changes to confirmed bug fixes, correctness, safety, and reliability
  fixes for existing behavior, regression coverage, and documentation needed
  to describe the corrected behavior.
- Do not add new public APIs, event namespaces, platform support claims, or
  speculative behavior in 2.1.x.
- Keep Linux and FreeBSD as the validated platform baseline.
- Add build or test tooling only when it is directly needed to reproduce,
  verify, or prevent regression of a maintenance fix.

## 3.0 Direction

Cim 3.0 is the next planned feature and public API development line. Its design
will be driven by demonstrated integration requirements discovered while
Guiyom develops as a platform-neutral GUI toolkit. IME integration may require
substantial changes, so Cim will not freeze those decisions into a 2.x minor
release prematurely.

Before defining Cim 3.0, audit the public API and ABI against Minimum
Sufficient Design and demonstrated integration requirements. In particular,
reconsider:

- the Cim key, modifier, hardware-code, event, and callback model;
- normalization boundaries between toolkit or platform-native events and Cim;
- units and valid ranges for candidate auxiliary cursor positions;
- platform-specific shared-library names, loader flags, symbol exports, and
  runtime search paths where validated target support requires them;
- compatibility-only 2.x surface, including `CIM_ERROR_INVALID_ARGUMENT`,
  `CIM_ERROR_MUTEX_FAILED`, and the notification API; and
- whether both `Cim.C` and `Cim.Runtime` should remain published Ada library
  interfaces.

Do not add new 2.x behavior solely to justify compatibility-only interfaces or
to anticipate the 3.0 design.

## Release Policy

### 2.1.1 (completed 2026-08-14)

Cim 2.1.1 preserves the Cim 2.1 public C ABI and published Ada source
interfaces. Its compatible-fix release gate is complete:

- completed core hardening regressions remain passing;
- confirmed bridge lifetime and failure-handling defects are corrected; and
- `static-pic` and `relocatable` test suites pass.

### Later 2.1.x releases

Later 2.1.x releases, if needed, are limited to maintenance fixes under the
policy above. They must preserve the Cim 2.1 public C ABI and published Ada
source interfaces.

### 3.0.0

Cim 3.0 is the next planned point for intentional public API and ABI redesign.
There is no planned 2.2.0 release between the 2.1.x maintenance line and 3.0.

## Non-Goals

- Cim plugins must not depend on accidental lookup of private host symbols.
- Independent Cim instances must not share opaque input-context handles.
- Contract violations must not be converted into successful no-ops.
- Plugin unload must not be used as a substitute for explicit input-context
  ownership and cleanup.
- A public manual initialization API is not planned for normal C clients.

## Definition of Done

A roadmap item is complete only when:

- its ownership, threading, failure, and lifecycle contracts are documented;
- implementation and regression tests cover success and failure paths;
- final artifacts expose only their intended ABI symbols;
- `static-pic` and `relocatable` configurations behave consistently at the
  public API;
- repeated load, use, teardown, and reload cycles are deterministic; and
- no unresolved high-severity correctness issue remains in the affected path.
