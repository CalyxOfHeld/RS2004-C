# Scenery and texture update — October 20, 2026

Extract the update ZIP into the existing `2004scape C Rewrite` folder, preserving
its layout. Keep the existing engine cache in place. Launch `play_client.bat --offline`
for Lumbridge, or launch without `--offline` with your server running.

The updated native client renders depth-correct opaque scenery and cache textures.
Four camera views, walking, rebuild identity, and live server login/map rebuilding
were verified in the actual SDL client. Details and remaining work: `HANDOFF.md`.

Executable SHA-256: `D66AAC9B608F01184AC303C9160C5F2759663220793433988DF0CF2D0E97E3AA`
