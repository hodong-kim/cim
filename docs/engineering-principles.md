# Engineering Principles

Performance, safety, reliability, and maintainability under large-scale and
extreme conditions are the primary criteria for evaluating all designs,
architectures, implementations, and operational decisions.

## Minimum Sufficient Design

A minimum sufficient design is the smallest design that fully satisfies the
defined goal and the engineering properties required to make that goal sound.
It is not the design with the fewest components, abstractions, checks, or lines
of code.

For a goal A, retain every structure, invariant, boundary, or capability needed
to achieve A with the required correctness, performance, safety, reliability,
maintainability, and testability under the intended operating conditions. If
removing an element makes A brittle, weakens those properties, creates a known
architectural dead end, or predictably requires redesign to complete, operate,
or safely evolve A, that removal is underdesign rather than simplification.

Do not add generality solely for hypothetical future goals B or C. Extensibility
is part of maintainability when it is necessary to complete, operate, or safely
evolve A without violating A's contracts or invariants. Speculative extension
points, generic frameworks, and abstractions without such a requirement shall
be deferred until a demonstrated need exists.

Distinguish required structure from a particular implementation mechanism.
Required ownership, lifecycle, concurrency, failure, persistence, isolation,
or extension boundaries may need to be established up front, while the
concrete mechanism should remain undecided until constraints, evidence, or
implementation work justify choosing it.

When deciding whether an element belongs in the design, ask whether omitting it
would compromise correctness, performance, safety, reliability,
maintainability, testability, or an extension path necessary for A. If so,
retain it. If its only justification is an uncertain future requirement
unrelated to completing, operating, or safely evolving A, defer it.

## API Design

Cim's canonical API reports recoverable failures through CimError.
C-facing APIs shall not expose Ada exceptions across the ABI boundary.
Ada-facing exception-raising APIs are convenience wrappers and shall be
implemented on top of the canonical status-code API.

## Language-Native API Design

Cim supports both C and Ada applications. C support shall not degrade Ada
usability, and Ada support shall not degrade C usability.

C-facing APIs should feel natural to C programmers, as if they were designed
and implemented for C. Ada-facing APIs should feel natural to Ada programmers,
as if they were designed and implemented for Ada.

Shared implementation details, bindings, or ABI requirements shall not force
either language interface into an unnatural or inconvenient design.

## State Model and Idempotency

Reliability shall be obtained through explicit state models, ownership rules,
and valid state transitions, not by treating repeated release, cleanup, or
destruction operations as successful no-ops.

A repeated release, cleanup, or destruction request usually indicates a logic
error. Such cases should be reported as INVALID_STATE, INVALID_HANDLE, or
CONTRACT_VIOLATION unless a specific API explicitly documents different
behavior.
