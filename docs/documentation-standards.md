# Documentation Standards

## Documentation Goals

- users should start with Blusys docs, not ESP-IDF docs
- docs should be task-first
- reference material should be short and precise
- examples should match the documented API exactly

## Required Documentation Per Module

Every public module must have:
- one task-first guide
- one API reference page
- one example application
- target support notes
- thread-safety notes
- blocking and async behavior notes
- major limitations and failure cases

## Writing Rules

- write for application developers first
- explain what the user is trying to achieve before describing functions
- prefer short examples over long theory
- do not copy ESP-IDF documentation structure blindly
- link to upstream ESP-IDF docs only as secondary reference

## Example Rules

- examples must compile against the documented public API
- examples should be minimal and realistic
- examples should prefer one clear task per example

## API Page Template

Each module page should include:
- purpose
- supported targets
- quick start example
- lifecycle model
- blocking APIs
- async APIs
- thread safety
- ISR notes
- limitations
- error behavior

## Guide Page Template

Each guide should include:
- problem statement
- prerequisites
- minimal example
- explanation of the few APIs used
- expected runtime behavior
- common mistakes
