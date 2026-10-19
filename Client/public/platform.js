// Works out where the game is running and, on CrazyGames, sets up their SDK.
// The game reads window.rrolfPlatform, always through brackets because the
// compiled client is minified.
//
//   mode "web"         a normal page, no SDK is loaded
//   mode "crazygames"  embedded in an iframe on CrazyGames
//   mode "local"       any page opened with ?useLocalSdk=true, the SDK's own
//                      test mode with demo users and a fake ad overlay
//
// The SDK's init() waits 7 seconds on a page that is not on CrazyGames, so it
// is only called when this page is framed (CrazyGames is the only site allowed
// to frame it, see the CSP in scripts/serve-client.js) or in local mode.
//
// The game (Client/Platform.c) polls this object once per frame, so most of
// what the SDK tells us is stored here and picked up from there.
(function () {
    const SDK_URL = "https://sdk.crazygames.com/crazygames-sdk-v3.js";
    // squad codes look like "hc-abcdef": the server's name, then six letters
    const ROOM_CODE = /^[a-z]{1,8}-[a-z]{6}$/;
    const platform = window.rrolfPlatform = {
        mode: "web",
        disableChat: false,
        sdk: null,
        // the logged in CrazyGames user, or null for a guest
        user: null,
        userVersion: 0,
        // set when the player logs in during this visit
        loginEvent: false,
        // a squad code to join, from an invite link or a friend joining us
        joinCode: "",
        // CrazyGames wants the player in a joinable room straight away
        instant: false,
        // logging in as part of linking must not offer to link a second time
        linkingUntil: 0,
    };

    function loadScript(src, timeoutMs) {
        return new Promise(function (resolve, reject) {
            const script = document.createElement("script");
            const timer = setTimeout(function () { reject(new Error("timeout loading " + src)); }, timeoutMs);
            script.src = src;
            script.onload = function () { clearTimeout(timer); resolve(); };
            script.onerror = function () { clearTimeout(timer); reject(new Error("could not load " + src)); };
            document.head.appendChild(script);
        });
    }

    function withTimeout(promise, timeoutMs, what) {
        return Promise.race([
            promise,
            new Promise(function (resolve, reject) {
                setTimeout(function () { reject(new Error(what + " timed out")); }, timeoutMs);
            }),
        ]);
    }

    function setUser(user) {
        platform.user = user ? {
            username: String(user.username || ""),
            avatar: String(user.profilePictureUrl || ""),
        } : null;
        ++platform.userVersion;
    }

    function acceptJoinCode(code) {
        if (typeof code === "string" && ROOM_CODE.test(code))
            platform.joinCode = code;
    }

    // Everything the game can ask the SDK for after init. Each call is allowed
    // to fail, the game must keep working without the SDK.
    function setUpSdk(sdk) {
        try {
            platform.disableChat = !!sdk.game.settings.disableChat;
            sdk.game.addSettingsChangeListener(function (settings) {
                platform.disableChat = !!(settings && settings.disableChat);
            });
        } catch (e) {
            console.warn("could not read the CrazyGames game settings", e);
        }
        try {
            sdk.game.loadingStart();
        } catch (e) {}
        try {
            platform.instant = !!sdk.game.isInstantMultiplayer;
            acceptJoinCode(sdk.game.getInviteParam("code"));
            // a friend joining while the game is already running
            sdk.game.addJoinRoomListener(function (params) {
                acceptJoinCode(params && params["code"]);
            });
        } catch (e) {
            console.warn("could not set up the CrazyGames room features", e);
        }
        try {
            // guests have no user, so getUser resolves to null for them
            sdk.user.getUser().then(setUser).catch(function () {});
            sdk.user.addAuthListener(function (user) {
                setUser(user);
                // logging in as part of linking is handled by the link itself
                if (Date.now() >= platform.linkingUntil)
                    platform.loginEvent = true;
            });
        } catch (e) {
            console.warn("could not set up CrazyGames accounts", e);
        }
    }

    // Resolves once the mode is known. Never rejects: any failure leaves the
    // game in plain web mode.
    platform.init = async function () {
        const query = new URLSearchParams(window.location.search);
        const local = query.get("useLocalSdk") === "true";
        // an invite link made by the "Copy Link" button on a plain web page
        const invited = query.get("code");
        if (invited) {
            acceptJoinCode(invited);
            try {
                query.delete("code");
                const search = query.toString();
                window.history.replaceState(null, "", window.location.pathname + (search ? "?" + search : ""));
            } catch (e) {}
        }
        let framed = false;
        try { framed = window.self !== window.top; } catch (e) { framed = true; }
        if (!local && !framed)
            return;
        try {
            await loadScript(SDK_URL, 10000);
            const sdk = window.CrazyGames.SDK;
            await withTimeout(sdk.init(), 15000, "SDK init");
            if (sdk.environment !== "crazygames" && sdk.environment !== "local")
                return;
            platform.sdk = sdk;
            platform.mode = sdk.environment;
            setUpSdk(sdk);
        } catch (e) {
            console.warn("CrazyGames SDK unavailable, continuing as a plain web page", e);
            platform.mode = "web";
            platform.sdk = null;
        }
    };

    // The helpers below swallow SDK errors and report whether they worked.
    function guarded(fn) {
        return function () {
            if (!platform.sdk)
                return 0;
            try {
                fn.apply(null, arguments);
                return 1;
            } catch (e) {
                return 0;
            }
        };
    }

    // the game started or stopped being played, not menus or the death screen
    platform.gameplay = guarded(function (active) {
        if (active)
            platform.sdk.game.gameplayStart();
        else
            platform.sdk.game.gameplayStop();
    });

    platform.loadingStop = guarded(function () {
        platform.sdk.game.loadingStop();
    });

    // the squad the player is in, so friends can join it from CrazyGames
    platform.room = guarded(function (code, joinable) {
        platform.sdk.game.updateRoom({
            roomId: code,
            isJoinable: !!joinable,
            inviteParams: joinable ? {code: code} : undefined,
        });
    });

    platform.leftRoom = guarded(function () {
        platform.sdk.game.leftRoom();
    });

    // A link that takes whoever opens it into the squad with this code. On
    // CrazyGames it is their link to the game with the code attached.
    platform.inviteLink = function (code) {
        if (platform.sdk) {
            try {
                return String(platform.sdk.game.inviteLink({code: code}));
            } catch (e) {}
        }
        const url = new URL(window.location.href);
        url.search = "";
        url.hash = "";
        url.searchParams.set("code", code);
        return url.toString();
    };

    // Puts text on the clipboard. Called from a click, where the async
    // clipboard may be blocked inside an iframe, so there is a fallback.
    platform.copy = function (text) {
        const fallback = function () {
            const area = document.createElement("textarea");
            area.value = text;
            area.style.position = "fixed";
            area.style.opacity = "0";
            document.body.appendChild(area);
            area.focus();
            area.select();
            try { document.execCommand("copy"); } catch (e) {}
            document.body.removeChild(area);
        };
        try {
            navigator.clipboard.writeText(text).catch(fallback);
        } catch (e) {
            fallback();
        }
    };

    // returns the pending squad code to join, once
    platform.takeJoinCode = function () {
        const code = platform.joinCode;
        platform.joinCode = "";
        return code;
    };

    platform.takeLoginEvent = function () {
        const event = platform.loginEvent;
        platform.loginEvent = false;
        return event ? 1 : 0;
    };

    // the "Login with CrazyGames" button: only logs in, linking the account is
    // a separate step the player asks for
    platform.login = async function () {
        if (!platform.sdk)
            return;
        platform.linkingUntil = Date.now() + 3000;
        try {
            if (!platform.sdk.user.isUserAccountAvailable) {
                alert("CrazyGames accounts are not available here.");
                return;
            }
            setUser(await platform.sdk.user.showAuthPrompt());
            platform.loginEvent = true;
        } catch (e) {
            console.warn("CrazyGames login did not complete", e);
        }
    };

    // Asks the player to log in to CrazyGames if they are not yet, then
    // resolves to a user token the master server can verify, or "" if they
    // cancel or accounts are unavailable.
    platform.linkToken = async function () {
        const sdk = platform.sdk;
        if (!sdk)
            return "";
        platform.linkingUntil = Date.now() + 30000;
        try {
            if (!sdk.user.isUserAccountAvailable) {
                alert("CrazyGames accounts are not available here.");
                return "";
            }
            let user = null;
            try { user = await sdk.user.getUser(); } catch (e) {}
            if (!user)
                user = await sdk.user.showAuthPrompt();
            setUser(user);
            return await sdk.user.getUserToken();
        } catch (e) {
            console.warn("CrazyGames login did not complete", e);
            return "";
        } finally {
            platform.linkingUntil = Date.now() + 3000;
        }
    };

    // After a fresh login, CrazyGames' own "link your account?" question. Like
    // linkToken, but only when the player answers yes.
    platform.linkTokenAfterPrompt = async function () {
        const sdk = platform.sdk;
        if (!sdk)
            return "";
        try {
            const answer = await sdk.user.showAccountLinkPrompt();
            if (!answer || answer.response !== "yes")
                return "";
            return await sdk.user.getUserToken();
        } catch (e) {
            return "";
        }
    };
})();
