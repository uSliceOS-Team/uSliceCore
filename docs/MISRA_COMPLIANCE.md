# MISRA Compliance Matrix

**Status: template, not yet populated.** No rule-by-rule evaluation has been
run against this codebase yet. This file exists so that when it is run, the
result has a place to live in a form that's actually useful (and safe to
publish): see [why the format looks like this](#why-no-rule-text) below.

This matrix will never contain MISRA rule text. Rule *numbers* are not
copyrighted; the *wording* of each rule is, and MISRA's license does not
permit reproducing it here. If you're filling this in from a purchased copy
of the standard, only bring the rule ID and your own compliance
determination (not the rule's text) into this file.

## Matrix

| Rule ID | Category | Status | Deviation / Justification | Verified by |
|---|---|---|---|---|
| _(pending)_ | | | | |

**Status values:**
- `Compliant`: no violation found, either by tool or manual review (state which, in the last column)
- `Non-compliant`: known violation, not yet addressed
- `Deviation`: intentionally not followed, with recorded justification (required by MISRA process for any deviation)
- `N/A`: rule does not apply to this codebase (e.g. a rule about exception handling in a codebase that never throws)
- `Not evaluated`: placeholder, not yet checked

## How to populate this

1. Run `cppcheck --addon=misra` (see `tools/cppcheck.sh`) against `tasks/`, `time/`, and `uSliceCore.h`.
2. If you have a purchased MISRA standard, cppcheck's MISRA addon can load a
   local rule-texts file to get descriptive output; see
   `tools/misra_rule_texts.example.txt` for the expected format. **Do not
   commit your populated rule-texts file.** It's covered by
   `.gitignore` (`tools/misra_rule_texts.txt`) because it would contain
   copyrighted MISRA text.
3. For every violation cppcheck reports, add a row: the rule ID it names, and
   either fix the code (`Compliant` once fixed) or record a `Deviation` with
   a real justification, not "will fix later."
4. Static analysis alone doesn't cover every MISRA rule (some are only
   checkable by manual review, e.g. rules about comment clarity or naming
   conventions). Tool coverage vs. manual-review coverage should eventually
   be tracked as a separate column here rather than assumed.

## Why no rule text?

MISRA rule wording is licensed content sold by MISRA Ltd. Reproducing it in
a public repository (even a handful of rules, even with attribution) is
outside what that license permits. Rule IDs and pass/fail/deviation status
are not the copyrighted part, so that's what this file tracks.
