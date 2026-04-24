# Getting started

Blusys starts with one decision: interactive or headless. From there you choose capabilities, optionally a profile, and run the same product on host before you touch hardware.

!!! note "Live catalog"
    `blusys create --list` shows the current interfaces, starter presets, capabilities, profiles, and policies.

<div class="grid cards" markdown>

-   **Interactive App**

    ---

    Local UI, screens, and encoder-friendly products.

    [Interactive Quickstart](quickstart-interactive.md)

-   **Headless App**

    ---

    Connected products with no local UI by default.

    [Headless Quickstart](quickstart-headless.md)

</div>

## The flow

1. Choose the product shape.
2. Scaffold it with `blusys create`.
3. Run it on host with `blusys host-build`.
4. Move to hardware or QEMU when you need it.
5. Add capabilities and profiles as the product grows.

## What each path is for

| Path | Use it when |
|------|-------------|
| Interactive | the product has a local UI, encoder, or screen |
| Headless | the product has no local UI and should stay reducer-first |

## Next steps

- [Product shape](product-shape.md)
- [App](../app/index.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)
- [Device, host & QEMU CLI](../app/cli-host-qemu.md)
