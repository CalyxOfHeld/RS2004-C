# Test the current Windows navigation checkpoint — October 21, 2026

Launch play_client.bat in this folder. It uses the configured local server on43595 and preserves account progress. If no server is running, start_local_server.bat checks for port conflicts before starting one. Reuse the existing server when it is already available. Close/reopen the client to load the new executable.

Installed native SHA-256: `487B85739750A5456CD5D0DB1BBCAC4ACC8360FA9BA9C7610377FA3197F94F9E`. Current cumulative overlay: navigation-verified-checkpoint.zip and its .sha256 sidecar. Extract into this existing rewrite checkout; existing engine/cache/content and build dependencies are required. No player saves or .env files are packaged. Preserve old packages and never restore the withdrawn save-purge launcher.

## Visible changes

1. Click reachable ground on the minimap, rotate with arrow keys and click again. The flag and route use the displayed camera direction. Movement comes from the server.
2. Click near the player centre to interrupt a route. Same-tile clicks now send the original movement packet instead of being discarded.
3. Longer winding routes retain their starting path before the original25-turn packet compression.
4. Distant entity dots respect the minimap border. Rotation uses the original integer trigonometry tables.

Modal interfaces, open menus and active item drags gate minimap input. Friend colouring, nonzero minimap zoom/anticheat offsets, full gameplay and broader original rendering parity remain incomplete. See MANUAL-TESTING.md for optional account selection and controls; preserve all accounts.

All39 programs pass with zero compiler warnings. Original Java comparisons cover20 route cases,4096 trigonometry entries and160 complete sprite framebuffers. The final executable also completed live region-crossing/cancellation checks. The separate optional login program skips without explicit opt-in; actual live sessions are documented independently. Exact evidence and remaining gaps: client-c/verification/completion/runs/2026-10-21-navigation-sol/README.md and HANDOFF.md.
