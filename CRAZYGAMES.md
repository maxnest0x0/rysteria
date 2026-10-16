# CrazyGames launch checklist

Tick a box when the task is done. Work happens on the `crazygames` branch, cut from `pvp`.

Requirement sources: [requirements](https://docs.crazygames.com/requirements/intro/), [SDK](https://docs.crazygames.com/sdk/intro/).

Launch order on CrazyGames: **Basic Launch** first (7-21 days, ads off), then **Full Launch** by invitation once the metrics pass. The game is embedded as an iframe of `https://game.parshwa.blog`.

## 0. Done

- [x] `crazygames` branch created from `pvp`
- [x] `scripts/` for build, local run, tunnel and server deploy
- [x] `MasterServer/.env` created with a random `PASSWORD_SALT` (gitignored)
- [x] Full local build works (`scripts/build.sh`)
- [x] Local stack runs and both game servers answer a websocket handshake (`scripts/dev-up.sh`)
- [x] Server host and user saved in `scripts/remote.env` (gitignored, no password)
- [x] Commit the above

## 1. Infrastructure

- [x] `scripts/tunnel-setup.sh` run (creates `rysteria-cg`, repoints `game.`, `default-game.` and `pvp-game.parshwa.blog`)
- [x] On the server `c1`: Remote Login is on and Homebrew is installed
- [x] On `c1`: `brew update && brew upgrade libwebsockets curl` (the binaries need `libwebsockets.22.dylib`, `c1` had `.19`)
- [x] `scripts/remote-setup.sh` passes
- [x] `scripts/remote-deploy.sh` runs and all four ports report up
- [x] Only one machine runs the `rysteria-cg` tunnel at a time
- [x] `https://game.parshwa.blog` loads in a browser through the tunnel
- [x] Switch to an ssh key (`~/.ssh/id_c1`, installed with `ssh-copy-id -i ~/.ssh/id_c1.pub tejashshah@c1.local`; the scripts use it automatically) and change the server password

## 2. Needed for Basic Launch

### Client connects through the tunnel
- [x] Websocket address uses `wss://default-game.parshwa.blog` and `wss://pvp-game.parshwa.blog` instead of being built from the script URL (`Client/DOM.c`, `rr_dom_get_socket_url`); localhost dev still works
- [x] A game can be played end to end through `https://game.parshwa.blog`

### SDK and platform detection
- [x] CrazyGames SDK script loaded and `await SDK.init()` run before the wasm starts (`Client/public/platform.js`, only when framed or with `?useLocalSdk=true`, because init waits 7 seconds on other pages)
- [x] Runtime detection of three modes: `crazygames`, `local` (`?useLocalSdk=true`) and plain web
- [x] Game works when the SDK environment is `disabled` (plain web)

### Hide what CrazyGames bans
- [x] Discord link-account stays on CrazyGames by your decision; a Link CrazyGames button sits next to it and Discord stays even after CrazyGames is linked (`Client/Ui/TitleScreen/Account.c`). Still a rule risk, see section 5
- [x] Github button kept visible on CrazyGames by your decision (`Client/Ui/TitleScreen/Github.c`)
- [x] Gardn button hidden, it promotes another game (`Client/Ui/TitleScreen/Gardn.c`)
- [x] Custom fullscreen button hidden (`Client/Ui/TitleScreen/Fullscreen.c`)
- [x] "Are you sure?" `onbeforeunload` prompt removed (`Client/Main.c`)
- [x] Discord community invite kept on the menu

### Chat
- [x] Client-side profanity filter on chat and nicknames, censored with `*` (`Client/Filter.c`). A server-side one is still open
- [x] Chat hidden and not sent when the SDK `disableChat` setting is on
- [x] Nicknames filtered too

### Gameplay and page behavior
- [x] The "Hide UI" setting and its [I] hotkey are disabled on CrazyGames (`Client/Platform.c`, `Client/Ui/TitleScreen/Settings.c`)
- [ ] Escape no longer leaves the squad by default on CrazyGames (it exits fullscreen there)
- [ ] Arrow keys, space and mouse wheel do not scroll the page
- [ ] Context menu blocked on the whole page
- [ ] `user-select: none` set on `body` for mobile
- [ ] UI readable at 907x510, 1077x606, 1216x684, 1366x768, 1920x1080 and mobile 800x450
- [ ] Mobile touch controls work in the CrazyGames app, including safe areas
- [ ] Physics identical at 60, 144 and 165 Hz
- [ ] Player lands in gameplay with at most one click
- [ ] English text only, checked for typos

### Loading
- [x] Ubuntu font bundled instead of loaded from Google Fonts (`Client/public/fonts/`, with its licence)
- [ ] Time from page load to gameplay is under 20 seconds
- [ ] Initial download is 50 MB or less (20 MB for the mobile homepage)

### Portal submission
- [ ] Game created with "Submit a game" in the Developer Portal
- [ ] Iframe URL `https://game.parshwa.blog` entered (find where the portal asks for it, or ask developer support)
- [ ] Description and controls written
- [ ] Lobby size entered (max players per server)
- [ ] Covers made: landscape 1920x1080, portrait 800x1200, square 800x800 (title only, no borders)
- [ ] Preview videos made: landscape and portrait 1080p, 15-20 seconds, no sound, no cursor
- [ ] Game works in `crazygames.com/preview` with the real SDK
- [ ] Sitelock header confirmed working (page embeds from CrazyGames, not from other sites)
- [ ] Working in Chrome and Edge, and checked in Safari and Firefox

## 3. Needed for Full Launch

### Gameplay events
- [x] `loadingStart` after the SDK starts and `loadingStop` once the first server answers (`Client/Platform.c`)
- [x] `gameplayStart` while the player is alive in the arena, `gameplayStop` on death, back to the menu or a lost connection

### CrazyGames login
- [x] "Link CrazyGames" button in the account panel (manual, never automatic) and a "Login with CrazyGames" button top right of the title screen that only logs in. Once logged in, the CrazyGames name shows there (the avatar is not shown)
- [x] Guests can always play without logging in
- [x] User token sent with the connection; the 100-byte code field widened in client, game server and master server (`Client/Game.c`, `Server/Server.c`)
- [x] Master server verifies the token (RS256, public key from `sdk.crazygames.com/publicKey.json`, expiry) and links `cg:<userId>` to an account
- [x] `cg_id` field added to the account defaults in `MasterServer/main.js` (unknown fields are deleted there)
- [x] A guest who logs in is offered CrazyGames' own "link your account?" question, the game reconnects with their token only if they say yes
- [x] The CrazyGames username fills the nickname when it is empty, and shows top right of the title screen
- [x] Env flag `CG_ALLOW_DEMO=1` accepts the SDK's local demo tokens (they are validly signed, so the signature is still checked)
- [ ] Real `gameId` read from a real token and `CG_GAME_ID` set in `MasterServer/.env`
- [x] CrazyGames' account link prompt is shown right after a fresh login when the account is not linked yet

### Multiplayer sharing
- [x] `updateRoom` and `leftRoom` report the squad code (it already starts with the server name, `hc-` or `pvp-`) and whether the squad has room
- [x] Invite parameter `code` is read on startup and moves the player to that server and squad
- [x] Join-room listener joins the squad, switching servers if needed, without reloading
- [x] `isInstantMultiplayer` joins the invited squad or makes a new private one, then presses ready

- [x] "Copy Link" button next to "Copy Code" in the squad panel, on every page. On CrazyGames it makes the CrazyGames invite link, elsewhere a link to the game with the squad code attached. Opening such a link joins the squad
- [x] PvP leaderboard: when a player with a linked CrazyGames account dies in PvP, the game server reports their score and the master server sends it to the CrazyGames leaderboard API (`MasterServer/crazygames.js`)
- [ ] Leaderboard enabled by CrazyGames (invite only) and `CG_LEADERBOARD_API_KEY` set in `MasterServer/.env`

- [x] "Terms & Privacy" button at the bottom of the settings opens an in-game panel with the terms, what data is kept and the source code notice (`Client/Ui/TitleScreen/Legal.c`). Read the text and adjust it if it is not true for you

### Storage
- [x] Data module is not used, by decision. Guest identity stays in localStorage
- [x] "Progress Save" toggle stays off since the Data module is not used

## 4. Hardening before real traffic

- [ ] `SERVER_SECRET` moved out of the source into the environment (`MasterServer/main.js`, `Shared/Api.h`)
- [ ] Old Rivet cloud token, ngrok token and Discord webhook in the repo rotated or removed
- [ ] Master server stays off the tunnel
- [ ] Optional: game servers check the websocket `Origin`
- [ ] The server `c1` stays on and connected for the whole review

## 5. Decisions only you can make

- [ ] Discord login on CrazyGames: the popup login now works inside an iframe, but CrazyGames' account rules ban external login options in both Basic and Full implementations. Decide whether to hide it there (planned, see section 2) or risk a rejection
- [ ] Originality: CrazyGames requires original names and content, and this is a florr-style game
- [ ] License: the repo is AGPL, so players must be offered the source; decide where the link goes
- [ ] Where servers run once real players arrive (home Mac or a hosted server)
- [ ] Merge plan: `crazygames` branch goes on top of `pvp` after the main dev finishes reviewing

## Handy commands

```
scripts/build.sh                  build client and both servers
scripts/dev-up.sh [--tunnel]      run everything on this Mac
scripts/tunnel-setup.sh           one-time tunnel setup
scripts/remote-setup.sh           one-time server setup
scripts/remote-deploy.sh          copy to the server and start, detached
scripts/remote-stop.sh            stop everything on the server
ssh tejashshah@c1.local 'tail -f rrolf-run/logs/*.log'
```
