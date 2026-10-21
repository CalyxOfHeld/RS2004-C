# Manual testing on Windows

Open the `2004scape C Rewrite` folder in this existing checkout. The checkpoint ZIP is an update for this checkout, not a standalone server distribution.

1. Double-click `start_local_server.bat`. Wait for **World ready** and keep its window open. This runs the existing production engine with game port **43595**, website **8889**, and management port **8899**. It does not install dependencies or pull repositories. If startup fails, read the error in that window before launching a client.
2. Double-click `play_client.bat`. It automatically uses the existing local test account (`antigravity` / `antigravity`); no credential entry is required. Use `test_client.bat` to compile first; it requires the existing compiler setup. Both launchers use **127.0.0.1:43595**. An explicit later `--port` argument overrides that selection.
3. Close the client when done. In the server window use **Ctrl+C** to request normal shutdown; wait for it to finish so player saves can be written.

The client opens on the original RuneScape title screen. Both Windows launchers automatically supply the existing local test account when neither `RS2_USERNAME` nor `RS2_PASSWORD` is set; the client fills the original login box with them and presses Login once, so no typing is needed. The account keeps its existing progress. After logging out (Logout tab, *Click here to logout*), the client returns to the title screen: click **Existing User**, type a username, press Tab, type the password and click **Login** to sign in again, with any local account. Login failures show the original two-line message in the login box (for example *Your account is already logged in. / Try again in 60 secs...* while a previous logout is still pending).

To optionally use a different account, supply both environment variables. For example, open PowerShell in this folder and enter:

```powershell
$login = Get-Credential -Message 'Enter your local game account'
$env:RS2_USERNAME = $login.UserName
$env:RS2_PASSWORD = $login.GetNetworkCredential().Password
.\play_client.bat
Remove-Item Env:RS2_USERNAME, Env:RS2_PASSWORD
$login = $null
```

The launchers supply the default pair only when neither variable is set. If you previously set just one variable, remove both to restore automatic default login, or supply both for another account. Without the variables, the raw executable shows the title screen and waits for you to log in there. Explicit `--offline` previews require no credentials.

Credentials apply to the client launched from that PowerShell window. Do not put passwords in bug reports. Use a separate test account for experiments; no test scripts or save resets are needed for ordinary play.

## What to check

- Confirm the console says **Connected to LIVE SERVER**, then that the scene and actual inventory appear.
- Left-click an accessible ground tile to walk. Arrow keys rotate the camera. WASD moves only in the explicit offline preview.
- Select sidebar tabs with the mouse (F2 also selects Stats). Right-click an inventory item that has a Wield operation; choose it, then check the equipment tab. Select a combat style for an equipped weapon and check the resulting selection. These depend on the actual account having suitable items.
- Right-click an available ordinary door, choose Open/Close, and watch its state change. Talk to an available NPC and use Continue/choices if its real script presents them. NPC availability and dialogue depend on actual server state.
- Escape closes an active interface/menu first; otherwise it selects the inventory tab. Close the window to exit.

See TEST-THIS-CHECKPOINT.md for bank X/drag, shop buy/sell, ground quantity/reveal and stair checks. These have bounded recorded live scenarios. Live public chat and full gameplay coverage remain unfinished. Do not treat failure of those as a login problem. Previous Cook dialogue verification used an explicitly prepared developer fixture; this guide does not promise a naturally available Cook scenario or completed quests/tutorial. Never use `--offline` to validate live gameplay.

## Connection failures

Login reply 5 (title message *Your account is already logged in.*, console `[LOGIN] login reply=5`) means the account is already logged in or its logout is still pending. Close another client for that account, wait for the server to finish logout, and retry. A separate valid local account is also an option. It does not mean the password is wrong. Code3 indicates the server rejected the username/password. Reply -1 (*Error connecting to server.*) is a connection/handshake failure, not an invalid-password result. The client now reports whether connection, server seed, send, or login response failed, together with the endpoint. Check that this project's server reached World ready and has no port-conflict errors. Port 43594 was already occupied by a different game server during diagnosis; the updated launchers use 43595 to avoid that conflict. The raw executable still defaults to 43594 unless given `--port 43595`.

The server launcher checks ports before starting. If all three configured ports already listen in one process, it skips starting another server and directs you to the existing server window; wait for World ready there. If only some ports are occupied, it stops with a port-specific message. Ports 8889 and 8899 must also be available for startup. If the server returns another login code, check the specific message and your chosen account. The native client now explains codes5 and3 separately. A recently disconnected account may need time to log out before another attempt.

`play_client.bat --offline` is only an explicit scene preview with fixture state. It needs no server and cannot establish inventory, dialogue, movement or combat functionality against the server.
