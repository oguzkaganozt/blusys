# Adding a capability

Add a capability when the shared catalog needs a new runtime module.

## The shape

Every capability is:

1. one header
2. one host stub
3. one device implementation

## Steps

1. Reserve a new event range in `components/blusys/include/blusys/framework/capabilities/capability.hpp`.
2. Add the capability header with config, status, and event types.
3. Add `*_device.cpp` for real hardware work.
4. Add `*_host.cpp` so host builds reach steady state.
5. Register the capability in `app_spec` and map its events to actions.

## Keep it small

- Use declarative config only.
- Emit `capability_ready` when startup is done.
- Keep `poll()` non-blocking.
- Update status before posting events.

## Verify

- Host build compiles without hardware headers.
- Validation example covers the event sequence.
- `scripts/check-capability-shape.sh` passes.

## See also

- [Authoring a capability](capability-authoring.md)
- [Capability composition](capability-composition.md)
- [Product-custom capabilities](custom-capabilities.md)
