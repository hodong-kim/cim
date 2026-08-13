# Clair Coding Style Guide

The Clair coding style adapts common conventions from mainstream programming
languages to Ada. Variables and subprograms use `snake_case`. Types,
subtypes, packages, exceptions, protected objects, loop names, and labels use
`Mixed_Case`. Symbolic constants use `UPPER_CASE_WITH_UNDERSCORES`.

The goal is consistency, readability, and predictable formatting across the
codebase.

-----

## File Headers

The following file-header form is recommended, but not required:

```ada
-- ============================================================================
-- cim.gpr
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
```

Use the file's actual name and applicable copyright year.

-----

## Indentation

Use **2 spaces** for indentation. Do not use tabs.

**Rationale**: Ada code can become deeply nested. A 2-space indent keeps line
width under control while still making block structure visible.

-----

## Line Length And Wrapping

### Common Rules

Keep each code line at or below **80 columns**.

Before wrapping a line, identify why it exceeds 80 columns. If the excess is
caused by a trailing comment, apply the comment placement rule first. Variable
declarations, subprogram declarations, subprogram calls, and type conversions
have separate wrapping rules.

**Rationale**: 80-column code remains readable in split panes, terminals, and
review tools without horizontal scrolling.

### Variable Declaration Wrapping

Declare one object per variable declaration. Do not combine multiple object
names before one type separator.

```ada
px : Coord;
py : Coord;
pw : Coord;
ph : Coord;
```

Keep a variable declaration on one line when it fits within 80 columns and the
initializer is not a multi-line aggregate.

When a declaration does not fit on one line, or when the initializer is a
multi-line aggregate, wrap before the assignment operator (`:=`). Align the
assignment operator with the type separator (`:`).

```ada
options : constant Formatter.Options
        := formatter.default_options;
```

For a multi-line aggregate initializer, keep `:= (` on the assignment
continuation line. Align aggregate component associations with the first
component and align association operators (`=>`) within the aggregate.

```ada
range_config : constant Range_Config
             := (minimum => 0,
                 maximum => limit,
                 step    => 1);
```

If a declaration block aligns several type separators (`:`), additional spaces
may be used for that alignment.

```ada
req_name      : Ada.Strings.Unbounded.Unbounded_String;
req_max_count : Natural := 1000;
```

If the line exceeds 80 columns only because of a trailing comment, do not wrap
the declaration. Move the comment above the declaration instead.

### Type Declaration Wrapping

Keep a type declaration on one line when it fits within 80 columns.

When an array type declaration exceeds 80 columns, keep the declaration head
and the array index constraint on the first line when they fit. Break before
`of` and place the component type on the next line.

```ada
type Small_Array is array (1 .. 10) of Integer;
```

```ada
type External_Library_Handle_Table is array (0 .. MAX_REGISTERED_ITEMS)
  of External.Library.Handle;
```

Do not break immediately after `is` when the type constructor still fits on
the same line.

Avoid:

```ada
type External_Library_Handle_Table is
  array (0 .. MAX_REGISTERED_ITEMS) of External.Library.Handle;
```

### Comment Placement And Wrapping

Comments must be written in English to keep code review, maintenance, and
tool-assisted analysis accessible to a common working language in software
development.

Comments should describe the concrete behavior or intent of the code.

Use one space between `--` and the comment text.

If a trailing comment makes a line exceed 80 columns, move the comment to the
line immediately above the code it describes.

```ada
-- A negative value indicates unconstrained width.
req_max_width : Clair.Coord := -1;
```

Avoid splitting code only to keep a trailing comment on the same logical line.

```ada
req_max_width : Clair.Coord
  := -1; -- A negative value indicates unconstrained width.
```

Multiple spaces in comments are allowed only when they serve a specific
formatting purpose, such as aligning explanatory text.

```ada
-- Initialize the subsystem.
-- Fields:
--   req_width
```

Avoid comments without a space after `--` or with unnecessary extra spacing.

```ada
--No space after the marker.
--  Unnecessary extra spacing.
```

### Subprogram Specification Wrapping

This rule applies to subprogram declarations and to the subprogram
specification that begins a subprogram body. It does not apply to subprogram
calls. The `is` placement rules apply only to subprogram bodies.

Choose the wrapping form in this order:

1. If the subprogram has two or more parameters, place the opening parenthesis
   on the next line and use a vertical parameter layout, even when the full
   specification would fit within 80 columns.
2. In a wrapped parameter list, put each complete parameter declaration on its
   own line when it fits within 80 columns.
3. Within the same subprogram specification, align parameter names and type
   separators (`:`).
4. Do not combine multiple parameter names before one type separator in a
   wrapped subprogram specification.
5. For a function with a wrapped parameter list, place the `return ...` clause
   on a separate line aligned with the start of the declaration.
6. If only the `return ...` clause makes a function specification too long,
   move the `return` clause to the next line.
7. For a subprogram body with an empty declarative part, keep `is` on the final
   specification line when it fits within 80 columns. For a function body with
   a separate `return ...` clause, keep `return ... is` on the same line when
   it fits.
8. For a subprogram body with a non-empty declarative part, place `is` on a
   separate line aligned with the subprogram keyword and `begin`.

```ada
procedure close_file (handle : File_Handle);
```

```ada
procedure enqueue
  (self    : in out Context;
   item    : in Element_Type;
   success : out Boolean);
```

```ada
procedure update_record_state
  (target : in out Record_State;
   code   : Status_Code;
   flags  : Update_Flags)
is
begin
  null;
end update_record_state;
```

```ada
function find_matching_record
  (table : Record_Table;
   key   : Record_Key)
return Record_Access;
```

```ada
function make_token
  (kind     : Token_Kind;
   text     : String := "";
   position : Adac.Source.Position)
return Token is
begin
  null;
end make_token;
```

When the `return` clause is placed on a separate line, align `return` with the
start of the declaration.

```ada
function xcb_intern_atom
  (conn           : Connection;
   only_if_exists : Interfaces.C.unsigned_char;
   name_len       : Interfaces.C.unsigned_short;
   name           : System.Address)
return intern_atom_cookie_t;
```

### Subprogram Call Wrapping

This rule applies only to subprogram calls. It does not apply to subprogram
declarations.

Keep a subprogram call on one line when it fits within 80 columns.

```ada
process_record (target, Record_Kind, record_data);
```

When a subprogram call exceeds 80 columns, break after the subprogram name and
put the full argument list on the next line. The opening parenthesis (`(`)
starts that next line.

```ada
logger.write
  ("connection failed: " & Clair.Error.get_error_message (errno_code));
```

### Type Conversion Wrapping

Do not put a space between a type name and the opening parenthesis (`(`) in a
type conversion when they appear on the same line.

```ada
return Token_Kind(current_token.kind);
```

When a type conversion appears inside an aggregate association and the
converted expression would make the line too long, the type name may remain on
the association line and the converted expression may be placed on the next
line. Align the continuation under the type conversion expression.

```ada
timeout_ts : constant Clair.Time.Timespec :=
  (tv_sec  => Clair.Time.time_t(actual_timeout / 1000),
   tv_nsec => Interfaces.C.long
                ((actual_timeout rem 1000) * 1_000_000));
```

For one-line conversions, keep the type name and opening parenthesis together.

```ada
timeout_ms := Integer(remaining_ts.tv_sec) * 1000;
```

### Return Statements

Keep `return` and the returned expression on one line when the full return
statement fits within 80 columns.

For a multi-line aggregate return value, keep `return (` and the first
component association on the same line when that line fits within 80 columns.
Align the remaining component associations with the first component and align
association operators (`=>`) within the aggregate.

```ada
return (kind     => kind,
        text     => Ada.Strings.Unbounded.to_unbounded_string (text),
        position => position);
```

Do not break immediately after `return` only to place the aggregate on the next
line when `return (` and the first component fit on the same line.

Avoid:

```ada
return
  (kind     => kind,
   text     => Ada.Strings.Unbounded.to_unbounded_string (text),
   position => position);
```

### Binary Expressions

When a binary expression is wrapped across multiple lines, place the operator
at the end of the continued line.

Good:

```ada
HEADER_BAR : constant String :=
  "======================================" &
  "======================================";
```

Avoid:

```ada
HEADER_BAR : constant String :=
  "======================================"
  & "======================================";
```

### Assignment Statements

In a contiguous block of two or more assignment statements at the same nesting
level, align assignment operators (`:=`) vertically. Do not align assignments
across blank lines, comments, or nested constructs.

```ada
self.req_state := state;
self.is_active := True;
```

-----

## Spacing

### Subprogram Calls And Declarations

Use one space between a subprogram name and the opening parenthesis (`(`).

```ada
Clair.Error.get_error_message (errno_code);
procedure exit_process (status : Integer := EXIT_SUCCESS);
```

**Rationale**: The space visually separates the subprogram name from its
parameter list. It also makes exact searching easier when a prefix is shared by
several identifiers.

### Attribute Calls

Use one space between an attribute name and the opening parenthesis (`(`) when
the attribute takes an argument.

```ada
Interfaces.C.int'image (fd);
```

Attributes without an argument are written directly after the object.

```ada
errmsg'length
```

### Array Indexing

Do not put a space between an array name and the opening parenthesis (`(`).

```ada
all_bids(n)
my_matrix(row, col)
```

### Range Operator

Use one space on both sides of the range operator (`..`).

```ada
range 0.0 .. 100.0
for i in 1 .. 10 loop
```

-----

## Naming

Use a name shape that matches the role of the identifier.

When code mirrors an external interface, preserve external names or
abbreviations only when they improve traceability to that interface.

Identifiers introduced locally should follow the normal naming rules. Prefer
complete words over ad-hoc abbreviations when the full word is clear and
reasonably short.

### `snake_case`

Use lowercase words separated by underscores.

Apply to:

- Reserved words
- Aspects
- Pragmas
- Variables
- Parameters
- Subprograms
- Entries
- Attributes
- Local constants that hold computed values within a narrow scope

Status-code return variables should be named `retval` to avoid ambiguity with
Ada `Result` usage. For data values, use a name that describes the value, such
as `bytes_written`.

```ada
pragma import (c, my_func)
with convention => c
my_variable
get_item
errmsg'length
```

Ada aspect identifiers and convention identifiers follow the normal
`snake_case` rule, even when the Ada Reference Manual or compiler
documentation shows them in mixed case.

### `Mixed_Case`

Capitalize each word and separate words with underscores.

Apply to:

- Types
- Subtypes
- Enumeration literals
- Exceptions
- Protected objects
- Packages
- Loop names
- `goto` labels

Do not repeat a package name in a type declared inside that package. For
example, use `File.Descriptor`, not `File.File_Descriptor`.

Use all capitals for specific abbreviations when mixed case would be
misleading. For example, use `Clair.DL`, not `Clair.Dl`.

```ada
Library_Load_Error
Main_Process_Loop
Clair.Process
Clair.DL
```

### `UPPER_CASE_WITH_UNDERSCORES`

Use all caps with underscores.

Apply to:

- Symbolic constants whose values are fixed by the program text and used as
  named constants
- Standard library constants

```ada
EXIT_SUCCESS
NULL_HANDLE : constant Handle := Handle(System.NULL_ADDRESS);
System.NULL_ADDRESS
```

-----

## `use` Clauses

Avoid broad or unnecessary `use` clauses.

A `use` clause may be used when it improves readability and the imported
identifiers remain obvious from the local context. This is usually acceptable
for a package body or a small local scope that is centered around one specific
abstraction.

```ada
use Adac.Frontend.Tokens;
```

Avoid using several broad packages together when doing so makes it unclear
where identifiers come from.

```ada
use Ada.Text_IO;
use Ada.Strings.Unbounded;
use Ada.Characters.Handling;
```

Prefer explicit qualification when the source package is not obvious or when
multiple packages define similar names.

-----

## API Design

### Primary Type Names

Choose the primary type name according to the abstraction level and role.

Use `Object` for high-level, object-oriented entities that have active
behavior. GUI widgets are the typical case.

```ada
Window.Object
Button.Object
```

Use `Context` or `Handle` for low-level resource management or execution
environment objects. These names fit opaque implementation details or system
resources.

```ada
Clair.Event_Loop.Context
Clair.DL.Handle
```

### Dot Notation

Ada 2012 supports `obj.method` calls for tagged types. Design primitive
operations with the receiver as the first parameter, usually named `self`, so
dot notation remains natural for callers.

-----

## Control Flow And Nesting

### Guard Clauses

Use guard clauses to avoid deeply nested `if` statements. Check exceptional or
failure conditions first, then leave early with `return` or `goto Next_Iter;`.

```ada
if not is_valid then
  goto Next_Item;
end if;

-- Main logic remains flat.
```

**Rationale**: Ada does not have a `continue` statement. Early exits keep loop
and conditional logic flatter and easier to scan.

## API Comments

Public API declarations should use `--!` comments when the contract,
ownership, outputs, or return status requires clarification.

API comments should be concise and should describe the public API contract, not
the implementation. Do not document private implementation details such as
internal reference counts, backend-specific cleanup paths, or garbage queues.

Use the following fields when applicable:

- `summary`
- `contract`
- `ownership`
- `outputs`
- `returns`
- `notes`

Status names and code symbols should be written using backticks, such as `OK`,
`INVALID_STATE`, `remove`, or `NULL_HANDLE`.

Prefer documenting API-level obligations and effects:

- whether a context must be initialized or uninitialized;
- whether a handle must be non-null;
- whether ownership is transferred or consumed;
- whether an output parameter is initialized on success or failure;
- which status codes are expected for normal recoverable failures.

Do not repeat obvious type information. Do not describe how the implementation
achieves the behavior unless that detail is part of the API contract.

For overloaded APIs with the same semantics, document the first overload unless
the overloads differ in contract, ownership, outputs, or return behavior.
