# Failure Model

Cim distinguishes internal contract violations from external system failures.

Internal contract violations shall not be silently converted into normal
successful control flow.

A call to an operation implies that the caller expects the documented
preconditions of that operation to be satisfied.

If a parameter, field, callback, handle, pointer, index, length, or enum value
is required to be valid by contract, an unexpected invalid value is a contract
violation. Such a value shall not be silently handled as a successful no-op,
truncated value, clamped value, fallback value, or ordinary failure result.

A null value may be accepted silently only when the operation explicitly
documents that null is a valid input and defines the corresponding behavior.

A numeric value may be converted, clamped, truncated, or saturated only when
that behavior is explicitly part of the documented contract. Otherwise, an
out-of-range value is a contract violation.

Resource release operations shall not treat an unexpected null resource as a
successful release unless that behavior is part of the documented contract.

ABI boundary code shall preserve contract failures instead of hiding them.
When Cim converts values between Cim types and toolkit, plugin, or operating
system types, the conversion shall either be proven valid or report a contract
violation. Silent narrowing conversions are not allowed.

External system failures are different from internal contract violations.
External failures may require recovery handling, state invalidation, resource
teardown, retry behavior, or controlled failure reporting.

Examples of external failures include:

* plugin load failure
* plugin ABI mismatch
* plugin initialization failure
* operating system resource failure
* allocation failure
* toolkit bridge failure
* input method framework failure
* backend communication failure
* external protocol failure
* service disconnection
* display server disconnection

Internal contract violations indicate a bug in Cim, a bridge, a plugin, or the
caller. They should be detected early and reported clearly during development.
