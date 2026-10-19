// Verifies the user token the CrazyGames SDK hands out (SDK.user.getUserToken),
// a JWT signed with RS256, and returns the CrazyGames user id inside it.
// https://docs.crazygames.com/sdk/user/

const crypto = require("crypto");

const KEY_URL = "https://sdk.crazygames.com/publicKey.json";
const KEY_MAX_AGE = 10 * 60 * 1000;

// The SDK's local test mode (?useLocalSdk=true) hands out demo tokens for a fake
// game, "yourGameId". They are genuinely signed by CrazyGames and the same
// token is in the public SDK file, so anyone could present one. They are only
// accepted with CG_ALLOW_DEMO=1, for testing.
const DEMO_GAME_ID = "yourGameId";
const ALLOW_DEMO = process.env["CG_ALLOW_DEMO"] === "1";
// Once known (it is the gameId claim of a real token, the server logs it),
// tokens issued for any other CrazyGames game are rejected.
const GAME_ID = process.env["CG_GAME_ID"] || "";

let cached_key = null;
let cached_at = 0;

async function fetch_public_key(force)
{
    if (!force && cached_key && Date.now() - cached_at < KEY_MAX_AGE)
        return cached_key;
    const res = await fetch(KEY_URL);
    if (!res.ok)
        throw new Error("could not fetch the crazygames public key: " + res.status);
    cached_key = crypto.createPublicKey((await res.json())["publicKey"]);
    cached_at = Date.now();
    return cached_key;
}

function decode_part(part)
{
    return JSON.parse(Buffer.from(part, "base64url").toString("utf8"));
}

function signature_valid(parts, key)
{
    return crypto.verify("RSA-SHA256", Buffer.from(parts[0] + "." + parts[1]),
                         key, Buffer.from(parts[2], "base64url"));
}

// Checks everything about a token except where the key comes from. get_key(force)
// resolves to a public key, force asks for a fresh one.
async function verify_token(token, get_key, options)
{
    const parts = token.split(".");
    if (parts.length !== 3)
        throw new Error("not a jwt");
    const header = decode_part(parts[0]);
    const payload = decode_part(parts[1]);
    if (header.alg !== "RS256")
        throw new Error("unexpected algorithm " + header.alg);
    // the key can rotate, so a failure is retried once with a fresh one
    if (!signature_valid(parts, await get_key(false)) &&
        !signature_valid(parts, await get_key(true)))
        throw new Error("bad signature");
    if (typeof payload.exp !== "number" || payload.exp * 1000 < Date.now())
        throw new Error("token expired");
    if (typeof payload.userId !== "string" || !payload.userId ||
        payload.userId.length > 128)
        throw new Error("token has no usable userId");
    if (payload.gameId === DEMO_GAME_ID)
    {
        if (!options.allow_demo)
            throw new Error("demo token, set CG_ALLOW_DEMO=1 to test with them");
    }
    else if (options.game_id && payload.gameId !== options.game_id)
        throw new Error("token is for game " + payload.gameId);
    return payload;
}

// Resolves to the CrazyGames user id, throws if the token is not valid.
async function user_id_from_token(token)
{
    const payload = await verify_token(token, fetch_public_key,
                                       {allow_demo: ALLOW_DEMO, game_id: GAME_ID});
    console.log("crazygames token ok:", payload.userId, payload.username,
                "gameId", payload.gameId);
    return payload.userId;
}

if (ALLOW_DEMO)
    console.warn("CG_ALLOW_DEMO=1: crazygames demo tokens are accepted, testing only");

// ---- leaderboard -----------------------------------------------------------
// Scores are sent from here, not from the browser: the server knows the real
// score, and a browser submission needs the leaderboard's encryption key in
// public code, which would let anyone post any score.
// https://docs.crazygames.com/sdk/leaderboard-api/
// The leaderboard is only available to games CrazyGames has invited, and the
// api key comes from the Leaderboard tab of the game in the Developer Portal.
// Without CG_LEADERBOARD_API_KEY scores are dropped.
const LEADERBOARD_URL = process.env["CG_LEADERBOARD_URL"] ||
    "https://leaderboard.crazygames.com/leaderboard/scores";
const LEADERBOARD_KEY = process.env["CG_LEADERBOARD_API_KEY"] || "";
const pending_scores = [];
let warned_about_key = false;

function submit_score(user_id, score)
{
    if (!LEADERBOARD_KEY)
    {
        if (!warned_about_key)
            console.warn("no CG_LEADERBOARD_API_KEY, leaderboard scores are dropped");
        warned_about_key = true;
        return false;
    }
    if (!Number.isFinite(score) || score <= 0)
        return false;
    // timestamps may not be in the future, so allow for clock drift
    pending_scores.push({userId: user_id, score: score,
                         timestamp: new Date(Date.now() - 2000).toISOString()});
    return true;
}

async function flush_scores()
{
    if (pending_scores.length === 0)
        return;
    const batch = pending_scores.splice(0, 100);
    try
    {
        const res = await fetch(LEADERBOARD_URL, {
            method: "POST",
            headers: {"Content-Type": "application/json", "X-API-Key": LEADERBOARD_KEY},
            body: JSON.stringify({scores: batch}),
        });
        if (res.status === 429)
        {
            // rate limited, try again on the next round
            pending_scores.unshift(...batch);
            return;
        }
        const body = await res.json().catch(() => ({}));
        if (!res.ok)
            console.error("leaderboard rejected scores:", res.status, JSON.stringify(body).slice(0, 300));
        else if (body.errors && body.errors.length)
            console.error("leaderboard errors:", JSON.stringify(body.errors).slice(0, 300));
    }
    catch (e)
    {
        console.error("could not reach the leaderboard:", e.message);
    }
}

if (LEADERBOARD_KEY)
    setInterval(flush_scores, 2000).unref();

module.exports = {user_id_from_token, verify_token, submit_score, flush_scores};
