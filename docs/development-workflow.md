# Development Workflow

## Normal Order Of Work

1. confirm the scope
2. define the public API shape
3. define lifecycle and thread-safety rules
4. implement the module
5. add the example
6. add the task guide and module reference
7. validate all supported targets

## Change Rules

- start from the smallest correct implementation
- keep the public surface smaller than the backend surface
- avoid target-specific public APIs unless there is a clear need
- keep target differences internal

## Release Rules

- no undocumented public API
- no exampleless public module
- no module considered done without validation on all supported targets
