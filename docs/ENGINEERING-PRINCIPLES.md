# Engineering Principles

Performance, safety, reliability, and maintainability under large-scale and
extreme conditions are the primary criteria for evaluating all designs,
architectures, implementations, and operational decisions.

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
