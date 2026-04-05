# Documentation Standards

## Goals

- user docs should be task-first
- reference docs should be short and precise
- examples must match shipped behavior

## Required Per Public Module

- one example
- one task guide
- one module reference page
- documented target support, thread-safety notes, and limitations

## Writing Rules

- write for application developers first
- explain the task before listing APIs
- prefer short examples over long theory
- avoid repeating setup, build, and flash instructions across many pages
- use ESP-IDF docs only as secondary reference

## Page Shape

Module reference pages should include:

- purpose
- supported targets
- quick example
- lifecycle
- blocking and async behavior
- thread safety
- ISR notes when relevant
- limitations and error behavior

Task guides should include:

- problem statement
- prerequisites
- minimal example
- APIs used
- expected behavior
- common mistakes
