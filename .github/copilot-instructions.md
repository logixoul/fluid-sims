# Copilot Instructions

## Project Guidelines
- User prefers simple implementations over heavily optimized ones when requesting code changes.
- FFT storage in this codebase is unshifted; frequency-distance calculations should use wraparound distance from the DC bin rather than assuming a centered spectrum.