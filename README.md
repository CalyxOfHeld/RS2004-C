# RS2004-C

A native C99 rewrite of the RuneScape revision-225 client (May 2004), built on a checkout of [LostCity's Server](https://github.com/LostCityRS/Server) and checked against the original Java client frame for frame. Written in October 2026 by the repository owner with Claude and Codex.

- **The client:** [`client-c/`](client-c/) holds the source, tests, the Java parity oracle and the verification evidence. [RELEASE-CURRENT.md](RELEASE-CURRENT.md) describes the current Windows build. To run it, use `start_local_server.bat`, wait for World ready, then run `test_client.bat`.
- **History:** [HANDOFF.md](HANDOFF.md), [WORK-QUEUE.md](WORK-QUEUE.md) and [`archive/`](archive/) record each step. The checkpoints from before the first commit (October 20-21) are attached to the [pre-git-checkpoints release](../../releases/tag/pre-git-checkpoints), and the `pre-git-history` branch replays them as commits.
- **Wii U:** a port reached 9.8 fps on hardware (handoff steps 1-14). The Wii U work then moved to [Client3](https://github.com/CalyxOfHeld/Client3/tree/wiiu), which ran at about 20 fps, and its first-person fork [client3-fps](https://github.com/CalyxOfHeld/client3-fps).

The LostCity README follows.

---

<div align="center">
    <h1>Lost City</h1>
</div>

> [!NOTE]
> Learn about our history and ethos on our forum: https://lostcity.rs/t/faq-what-is-lost-city/16

This is a higher-level repository that links our other projects. You'll notice it's like home-rolled submodules (without commit references).  
Github won't include submodules in web downloads, and we have a lot of users who end up clicking download zip.

## Getting Started

> [!IMPORTANT]
> If you run into issues, please see our [common issues](#common-issues).

1. Download and extract this repo somewhere on your computer.
2. Install our [dependencies](#dependencies).
3. Open the folder you downloaded: **Run the start script and follow the on-screen prompts.** You may disregard any severity warnings you see.

Once your setup process has completed, wait for it to tell you the world has started before trying to play at: http://localhost/rs2.cgi

You can press `ctrl + c` to cancel/quit out of a terminal process.

## Dependencies

- Git CLI - Windows users: [git-scm](https://git-scm.com/)
- [NodeJS 24+](https://nodejs.org/)

> [!TIP]
> If you're using VS Code (recommended), [we have an extension to install on the marketplace.](https://marketplace.visualstudio.com/items?itemName=2004scape.runescriptlanguage)

## Workflow

**Use the start script provided** - it handles a lot of common use cases. We're trying to reduce the barrier to entry by providing an all-inclusive script.

## License
This project is licensed under the [MIT License](https://opensource.org/licenses/MIT). See the [LICENSE](LICENSE) file for details.
