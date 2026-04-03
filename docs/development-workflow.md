# Development Workflow

## Working Order

For each module, work in this order:

1. confirm module scope
2. finalize public API shape
3. define thread-safety and lifecycle rules
4. implement internal port binding
5. add blocking behavior first
6. add async behavior if required for that module
7. add example
8. add task-first documentation
9. validate both targets

## Change Rules

- start from the smallest correct implementation
- keep public surface smaller than the backend surface
- do not add options just because ESP-IDF has them
- avoid introducing target-specific public APIs unless clearly necessary

## Public API Review Checklist

- is the common use case short and readable?
- does this API save the user from reading ESP-IDF docs?
- does it avoid leaking backend types?
- is lifecycle ownership obvious?
- are thread-safety expectations explicit?
- is there a task-first example for it?

## Internal Implementation Rules

- localize target differences
- centralize error translation
- centralize concurrency policy where possible
- document any backend limitation that affects behavior

## Release Discipline

- no undocumented public API
- no exampleless public module
- no public module considered done without both target builds
