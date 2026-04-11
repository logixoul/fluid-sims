# Copilot Instructions

## Project Guidelines
- User prefers simple implementations over heavily optimized ones when requesting code changes.
- FFT storage in this codebase is unshifted; frequency-distance calculations should use wraparound distance from the DC bin rather than assuming a centered spectrum.
- References to symbols moved under the new `lx` namespace should be written with an explicit `lx::` prefix, even when the reference appears inside the `lx` namespace.