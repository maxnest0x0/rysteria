// Copyright (C) 2024 Paul Johnson
// Copyright (C) 2024-2025 Maxim Nesterov

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

require("dotenv").config({ quiet: true });
const express = require("express");
const cors = require("cors");
const path = require("path");
const fs = require("fs");
const WSS = require("ws");
const http = require("http")
const crypto = require("crypto");
const protocol = require("./protocol");
const GameServer = require("./gameserver");
const GameClient = require("./client");
const crazygames = require("./crazygames");
const app = express();
const port = 55554;
const namespace = "/api";

const DIRECTORY_SECRET = "a92pd3nf29d38tny9pr34dn3d908ntgb";
const CLOUD_TOKEN = "cloud.eyJ0eXAiOiJKV1QiLCJhbGciOiJFZERTQSJ9.CKOM_-XtMRCjtLqo-DAaEgoQjUflOmYrT3Sv5ckk4-Nk0yIWOhQKEgoQ6l7BiqssS-iYCw6PaqKKnA.Pgw_qDBaugBIFd7ilYcbbm_6yPNDeqreiDi1VBkKX84ER7CXvS-8abNuRhKtU_hDtgT9Sd4a7JWN68fdLnEKCA";
const NAMESPACE_ID = "04cfba67-e965-4899-bcb9-b7497cc6863b";
const SERVER_SECRET = "ad904nf3adrgnariwpanyf3qap8unri4t9b384wna3g34ytgdr4bwtvd4y";

const REDIRECT_URI = "https://rysteria.pro/";
const CLIENT_ID = "1453525695228678349";
const CLIENT_SECRET = process.env["CLIENT_SECRET"] || "";
const BOT_TOKEN = process.env["BOT_TOKEN"] || "";
const PASSWORD_SALT = process.env["PASSWORD_SALT"] || "";

if (!PASSWORD_SALT || !CLIENT_SECRET || !BOT_TOKEN)
    console.warn("missing .env");

let database = {accounts: [], links: {}};
let changed = false;
const databaseFilePath = path.join(__dirname, "database.json");

if (fs.existsSync(databaseFilePath))
{
    const databaseData = fs.readFileSync(databaseFilePath, "utf8");
    try {
        database = JSON.parse(databaseData);
        if (Array.isArray(database)) database = {accounts: [], links: {}}; // remove after first run
    } catch(e) {
        database = {};
    }
}
if (!Array.isArray(database.accounts)) database.accounts = [];
if (!database.links || Array.isArray(database.links)) database.links = {};

const hash = s => crypto.createHash("sha512").update(s, "utf8").digest("hex");

app.use(cors());

app.use(function (req, res, next) {
    res.setHeader("X-Powered-By", "custom rrolf http server written in c")
    next();
});

function is_valid_uuid(uuid)
{
    return uuid.length === 36 && uuid.match(/[0-9a-z]{8}-([0-9a-z]{4}-){3}[0-9a-z]{12}/) !== null;
}

function log(type, args, color = 31)
{
    // todo: save to some sort of log file
    console.log(`\u001b[${color}[${new Date().toJSON()}::[${type}]:\t${args.join("\t")}\u001b[0m`);
}

function get_rivet_url(key)
{
    return `https://kv.api.rivet.gg/v1/entries?namespace_id=${NAMESPACE_ID}&key=${key}`;
}

async function request(method, key, body)
{
    const url = get_rivet_url(key);
    const data_j = await fetch(url, {
        method,
        headers: {
            Authorization: `Bearer ${CLOUD_TOKEN}`
        },
        body: (() => {
            if (method !== "GET")
            {
                return JSON.stringify({
                    namespace_id: NAMESPACE_ID,
                    key,
                    value: body
                });
            }
        })()
    });
    if (method === "POST")
        return;
    try {
        const data = await data_j.json();
        return data;
    }
    catch (e) {
        console.log(e, data_j);
        return {};
    }
}

function apply_missing_defaults(account)
{
    const defaults = {
        password: "",
        username: "",
        xp: 0,
        petals: {"1:0": 5},
        failed_crafts: {},
        mob_gallery: {},
        checkpoint: { hell_creek: 0, pvp: 0 },
        score: 0,
        discord_id: "",
        cg_id: "",
        // inflated_up_to: 1,
    };

    // Fill in any missing defaults
    for (let prop in defaults) {
        if (!account.hasOwnProperty(prop)) {
            account[prop] = defaults[prop];
        }
    }

    // Remove any extra properties
    for (let prop in account) {
        if (!defaults.hasOwnProperty(prop)) {
            delete account[prop];
        }
    }

    if (typeof account.checkpoint === "number") {
        account.checkpoint = { hell_creek: account.checkpoint, pvp: 0 };
    }
    if (!account.checkpoint.hasOwnProperty("hell_creek"))
        account.checkpoint.hell_creek = 0;
    if (!account.checkpoint.hasOwnProperty("pvp"))
        account.checkpoint.pvp = 0;

    // if (account.inflated_up_to < MAX_PETAL_COUNT)
    // {
    //     log("inflated acc", [account.username]);
    //     for (let i = account.inflated_up_to; i < MAX_PETAL_COUNT; i++)
    //     {
    //         account.failed_crafts[`${i}:0`] = Math.max(4, account.failed_crafts[`${i}:0`] || 0); //  guarantee
    //         account.failed_crafts[`${i}:1`] = Math.max(4, account.failed_crafts[`${i}:1`] || 0); //  47.5%
    //         account.failed_crafts[`${i}:2`] = Math.max(6, account.failed_crafts[`${i}:2`] || 0); //  19.3%
    //         account.failed_crafts[`${i}:3`] = Math.max(8, account.failed_crafts[`${i}:3`] || 0); //  3.04%
    //         account.failed_crafts[`${i}:4`] = Math.max(10, account.failed_crafts[`${i}:4`] || 0); // 1.38%
    //     }
    //     account.inflated_up_to = MAX_PETAL_COUNT;
    // }
    return account;
}

async function write_db_entry(username, data)
{
    if (!database.accounts.includes(username))
        database.accounts.push(username);
    // try {
    changed = true;
    database[username] = data;
    //     await request("PUT", `${DIRECTORY_SECRET}/game/players/${username}`, data);
    // } catch(e) {
    //     console.log(e);
    // }
}

async function db_read_user(username, password)
{
    if (!database.accounts.includes(username))
        database.accounts.push(username);

    if (connected_clients[username] && (connected_clients[username].password === password || password === SERVER_SECRET))
        return connected_clients[username].user;
    const user = {value: database[username]}
    // const user = await request("GET", `${DIRECTORY_SECRET}/game/players/${username}`);
    if (!user.value)
    {
        return null;
    }
    if (password !== SERVER_SECRET && password !== user.value.password)
        return null;
    apply_missing_defaults(user.value);
    return user.value;
}

async function db_read_or_create_user(username, password)
{
    if (!database.accounts.includes(username))
        database.accounts.push(username);

    if (connected_clients[username] && (connected_clients[username].password === password || password === SERVER_SECRET))
        return connected_clients[username].user;
    const user = {value: database[username]}
    // const user = await request("GET", `${DIRECTORY_SECRET}/game/players/${username}`);
    if (!user.value)
    {
        log("account create", [username]);
        const user = apply_missing_defaults({})
        user.password = hash(username + PASSWORD_SALT);
        user.username = username;
        await write_db_entry(username, user);
        return user;
    }
    apply_missing_defaults(user.value);
    return user.value;
}

// Octobober 19, 2023 = 102923
function get_today()
{
    const date = new Date();
    
    const day = String(date.getDate()).padStart(2, '0'); // Day with leading zeros
    const month = String(date.getMonth() + 1).padStart(2, '0'); // Month is 0-based
    const year = String(date.getFullYear()).slice(-2); // Last two digits of year

    return `${month}${day}${year}`;
}

function get_unique_petals(petals)
{
    petals = structuredClone(petals);
    for (key in petals)
        petals[key] = 1;
    return petals;
}

async function db_append_petals_to_logs(petals)
{
    // const today = get_today();
    // let entry = (await request("GET", `${DIRECTORY_SECRET}/game/logs/${today}`)).value;
    // if (!entry)
    // {
    //     console.log("new day");
    //     const data = {
    //         games_played: 1,
    //         build_contains: get_unique_petals(petals),
    //         build_sum: structuredClone(petals) 
    //     };
    //     await request("PUT", `${DIRECTORY_SECRET}/game/logs/${today}`, data);
    //     return;
    // }

    // if (entry["7:3"] && today == "102923")
    // {
    //     console.log("wipe");
    //     entry = {games_played: 0, build_sum: {}, build_contains: {}};
    // }

    // entry.games_played++;

    // merge_petals(entry.build_sum, petals);
    // merge_petals(entry.build_contains, get_unique_petals(petals));
    // await request("PUT", `${DIRECTORY_SECRET}/game/logs/${today}`, entry);
}

function merge_petals(obj1, obj2) {
    for (let key in obj2) {
        if (obj1[key]) {
            obj1[key] += obj2[key];
        } else {
            obj1[key] = obj2[key];
        }
    }
}

async function handle_error(res, cb)
{
    try
    {
        res.end(await cb());
    }
    catch (e)
    {
        console.log(e);
        res.end("\x00" + e.stack);
    }
}

async function discord_oauth2(code) {
    let res = await fetch("https://discord.com/api/oauth2/token", {
        method: "POST",
        body: new URLSearchParams({
            "grant_type": "authorization_code",
            "code": code,
            "redirect_uri": REDIRECT_URI,
            "client_id": CLIENT_ID,
            "client_secret": CLIENT_SECRET
        })
    });
    if (!res.ok) throw new Error([res.url, res.status, res.statusText, await res.text()].join(" "));
    const token = (await res.json())["access_token"];
    res = await fetch("https://discord.com/api/v10/users/@me", {
        headers: {
            "Authorization": "Bearer " + token
        }
    });
    if (!res.ok) throw new Error([res.url, res.status, res.statusText, await res.text()].join(" "));
    const id = (await res.json())["id"];
    try {
        // hehehehaw
        res = await fetch('https://discord.com/api/v10/guilds/1229302334396563596/members/' + id, {
            method: "PUT",
            headers: {
                "Authorization": "Bot " + BOT_TOKEN,
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                "access_token": token
            })
        });
        if (!res.ok) throw new Error([res.url, res.status, res.statusText, await res.text()].join(" "));
    } catch (e) {
        console.error(e);
    }
    return id;
}

async function discord_name(id) {
    if (!id)
        return "";
    try {
        const res = await fetch("https://discord.com/api/v10/users/" + id, {
            headers: {
                "Authorization": "Bot " + BOT_TOKEN
            }
        });
        if (!res.ok) throw new Error([res.url, res.status, res.statusText, await res.text()].join(" "));
        return (await res.json())["username"];
    } catch (e) {
        console.error(e);
        return id;
    }
}

app.get(`${namespace}/user_get_password/:password`, async (req, res) => {
    const {password} = req.params;
    handle_error(res, async () => {
        // const d = await fetch("https://identity.api.rivet.gg/v1/identities/self/profile", {
        //     headers: {
        //         Authorization: "Bearer " + password
        //     }
        // });
        // if (d.status != 200)
        //     throw new Error(JSON.stringify(await d.text()));
        // const j = await d.json();
        return hash("fucking shit");
    });
});

app.get(`${namespace}/user_get_server_alias/:alias`, async (req, res) => {
    const {alias} = req.params;
    handle_error(res, async () => {
        if (game_servers[alias])
            return game_servers[alias].rivet_server_id;
        else
            return '';
    });
});

app.use((req, res) => {
    res.status(404).send("404 Not Found\n");
});

// ---- account requests from the game servers --------------------------------
// The master server is never reached by browsers. A player asks their game
// server to link a Discord / CrazyGames account or to make / use a transfer
// code, the game server forwards it over its websocket (message 5 below) and
// the answer goes back the same way (message 3), see handle_account_request.

const ACCOUNT_DISCORD = 0;
const ACCOUNT_CRAZYGAMES = 1;
const ACCOUNT_CODE_CREATE = 2;
const ACCOUNT_CODE_REDEEM = 3;

const TRANSFER_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
const TRANSFER_TTL = 10 * 60 * 1000;
const transfer_codes = new Map();

// requests per account per minute
const account_hits = new Map();
setInterval(() => account_hits.clear(), 60 * 1000).unref();

function fail(message)
{
    return {ok: false, message};
}

// Moves everything from the absorbed account into the surviving one. The
// absorbed account is deleted afterwards, so there are no duplicates.
function merge_accounts(survivor, absorbed)
{
    apply_missing_defaults(survivor);
    apply_missing_defaults(absorbed);
    survivor.xp = (Number(survivor.xp) || 0) + (Number(absorbed.xp) || 0);
    for (const biome of ["hell_creek", "pvp"])
        survivor.checkpoint[biome] = Math.max(survivor.checkpoint[biome], absorbed.checkpoint[biome]);
    merge_petals(survivor.petals, absorbed.petals);
    merge_petals(survivor.mob_gallery, absorbed.mob_gallery);
    for (const key in absorbed.failed_crafts)
        survivor.failed_crafts[key] = Math.max(survivor.failed_crafts[key] || 0, absorbed.failed_crafts[key]);
    if (!survivor.discord_id)
        survivor.discord_id = absorbed.discord_id;
    if (!survivor.cg_id)
        survivor.cg_id = absorbed.cg_id;
    for (const key in database.links)
        if (database.links[key] === absorbed.username)
            database.links[key] = survivor.username;
    for (const [code, entry] of transfer_codes)
        if (entry.username === absorbed.username)
            transfer_codes.delete(code);
    database.accounts = database.accounts.filter(name => name !== absorbed.username);
    delete database[absorbed.username];
    delete connected_clients[absorbed.username];
    for (const alias in game_servers)
    {
        const slot = game_servers[alias].clients.indexOf(absorbed.username);
        if (slot !== -1)
            game_servers[alias].clients[slot] = 0;
    }
    changed = true;
    log("account merge", [absorbed.username, survivor.username]);
}

// Two accounts that both have a Discord, or both have a CrazyGames account
// linked are never merged: that would link two external accounts together.
// Only CrazyGames with Discord is allowed. Answers with the reason, or null.
function merge_conflict(a, b)
{
    if (a.username === b.username)
        return null;
    if (a.discord_id && b.discord_id)
        return "Both accounts already have a Discord account linked.";
    if (a.cg_id && b.cg_id)
        return "Both accounts already have a CrazyGames account linked.";
    return null;
}

// The player is now the account `survivor`: the one they were on is merged
// into it unless it is the same one. Answers with that account's uuid and
// token, which the client reconnects with.
function switch_account(client, survivor, message)
{
    if (survivor.username !== client.user.username)
    {
        const conflict = merge_conflict(survivor, client.user);
        if (conflict)
            return fail(conflict);
        kick_client(survivor.username, null);
        merge_accounts(survivor, client.user);
    }
    survivor.password = hash(survivor.username + PASSWORD_SALT);
    write_db_entry(survivor.username, survivor);
    return {ok: true, message, uuid: survivor.username, token: survivor.password, code: ""};
}

// the account that owns a Discord / CrazyGames link, if it still exists
function link_owner(link)
{
    const owner = database.links[link];
    if (!owner)
        return null;
    if (!database[owner])
    {
        delete database.links[link];
        return null;
    }
    return database[owner];
}

// Links the player's account to an external account (user[field] = id, found
// again through database.links[link]). If that external account is already
// linked to another account, the player is moved onto that one.
function link_provider(client, field, id, link, name)
{
    const user = client.user;
    const owner = link_owner(link);
    if (owner)
        return switch_account(client, owner, `${name} account linked.`);
    if (user[field])
        return fail(`This account already has a ${name} account linked.`);
    user[field] = id;
    database.links[link] = user.username;
    log(`${name} link`, [user.username, id]);
    return switch_account(client, user, `${name} account linked.`);
}

async function handle_account_request(client, kind, payload, on_crazygames)
{
    const user = client.user;
    const hits = (account_hits.get(user.username) || 0) + 1;
    account_hits.set(user.username, hits);
    if (hits > 12)
        return fail("Too many requests, try again in a minute.");
    const now = Date.now();
    switch (kind)
    {
    case ACCOUNT_DISCORD:
    {
        if (!payload || payload.length > 100)
            return fail("Bad request.");
        let discord;
        try {
            discord = await discord_oauth2(payload);
        } catch (e) {
            console.error(e);
            return fail("Discord login failed, try again.");
        }
        return link_provider(client, "discord_id", discord, discord, "Discord");
    }
    case ACCOUNT_CRAZYGAMES:
    {
        if (!payload || payload.length > 4000)
            return fail("Bad request.");
        let cg_id;
        try {
            cg_id = await crazygames.user_id_from_token(payload);
        } catch (e) {
            console.error("crazygames token rejected:", e.message);
            return fail("CrazyGames login could not be verified.");
        }
        return link_provider(client, "cg_id", cg_id, "cg:" + cg_id, "CrazyGames");
    }
    case ACCOUNT_CODE_CREATE:
    {
        for (const [code, entry] of transfer_codes)
            if (entry.expires < now || entry.username === user.username)
                transfer_codes.delete(code);
        if (transfer_codes.size >= 5000)
            return fail("Try again later.");
        let code = "";
        for (let i = 0; i < 8; ++i)
            code += TRANSFER_ALPHABET[crypto.randomInt(TRANSFER_ALPHABET.length)];
        transfer_codes.set(code, {username: user.username, expires: now + TRANSFER_TTL, on_crazygames});
        log("transfer code created", [user.username]);
        return {ok: true, message: "Code valid for 10 minutes.", uuid: "", token: "",
                code: code.slice(0, 4) + "-" + code.slice(4)};
    }
    case ACCOUNT_CODE_REDEEM:
    {
        const code = payload.toUpperCase().replace(/[^A-Z0-9]/g, "");
        const entry = transfer_codes.get(code);
        if (!entry || entry.expires < now || !database[entry.username])
            return fail("That code is wrong or has expired.");
        if (entry.username === user.username)
            return fail("That code is from this account, enter it on the other one.");
        if (entry.on_crazygames === on_crazygames)
            return fail(on_crazygames ? "Use a code made on the web version, not on CrazyGames."
                                      : "Use a code made on CrazyGames, not on the web version.");
        const conflict = merge_conflict(database[entry.username], user);
        if (conflict)
            return fail(conflict);
        transfer_codes.delete(code);
        log("transfer code redeemed", [entry.username, user.username]);
        return switch_account(client, database[entry.username], "Accounts merged.");
    }
    }
    return fail("Bad request.");
}

const saveDatabaseToFile = () => {
    if (changed)
    {
        changed = false;
        console.log("saving database to file:", databaseFilePath);
        const databaseData = JSON.stringify(database);
        fs.writeFileSync(databaseFilePath, databaseData, "utf8");
    }
};

    const server = http.createServer(app);

server.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
    });

const wss = new WSS.Server({server});
const game_servers = {};
const connected_clients = {};

// Force-disconnects the player from the game server they are on. Leaves them
// alone when that is the server `keep_alias` (null kicks them wherever they are).
function kick_client(username, keep_alias)
{
    const previous = connected_clients[username];
    if (!previous || previous.server === keep_alias)
        return;
    const old_server = game_servers[previous.server];
    if (!old_server)
        return;
    const pos = old_server.clients.indexOf(username);
    if (pos === -1)
        return; // already left that server
    log("kick from old server", [username, old_server.alias, keep_alias]);
    const encoder = new protocol.BinaryWriter();
    encoder.WriteUint8(2); // force disconnect: slot, then the nonce to make sure it's the same connection
    encoder.WriteUint8(pos);
    encoder.WriteVarUint(previous.nonce);
    old_server.ws.send(encoder.data.subarray(0, encoder.at));
    old_server.clients[pos] = 0;
}

function kick_from_other_servers(username, new_server)
{
    kick_client(username, new_server.alias);
}

wss.on("connection", (ws, req) => {
    if (req.url !== `/api/${SERVER_SECRET}`)
       return ws.close();
    const game_server = new GameServer();
    game_server.ws = ws;
    game_server[game_server.alias] = game_server;
    ws.on('message', async (message) => {
        const data = new Uint8Array(message);
        const decoder = new protocol.BinaryReader(data);
        switch(decoder.ReadUint8())
        {
            case 0:
            {
                const uuid = decoder.ReadStringNT();
                const token = decoder.ReadStringNT();
                const code = decoder.ReadStringNT();
                const nonce = decoder.ReadVarUint();
                const pos = decoder.ReadUint8();
                log("attempt init", [uuid, token, code]);
                try {
                    let user = await db_read_user(uuid, SERVER_SECRET);
                    if (!user || !is_valid_uuid(uuid) ||
                        (user.password === hash(user.username + PASSWORD_SALT) && token !== user.password)) {
                        user = await db_read_or_create_user(crypto.randomUUID(), SERVER_SECRET);
                        // log("player force disconnect", [uuid]);
                        // const encoder = new protocol.BinaryWriter();
                        // encoder.WriteUint8(2);
                        // encoder.WriteUint8(pos);
                        // encoder.WriteVarUint(nonce);
                        // ws.send(encoder.data.subarray(0, encoder.at));
                        // break;
                    }
                    user.password = hash(user.username + PASSWORD_SALT);
                    write_db_entry(user.username, user);
                    kick_from_other_servers(user.username, game_server);
                    connected_clients[user.username] = new GameClient(user, game_server.alias, nonce, await discord_name(user.discord_id));
                    game_server.clients[pos] = user.username;
                    const encoder = new protocol.BinaryWriter();
                    encoder.WriteUint8(1);
                    encoder.WriteUint8(pos);
                    connected_clients[user.username].write(encoder);
                    ws.send(encoder.data.subarray(0, encoder.at));
                } catch(e) {
                    console.log(e);
                }
                break;
            }
            case 1:
            {
                const uuid = decoder.ReadStringNT();
                if (connected_clients[uuid] && connected_clients[uuid].server !== game_server.alias)
                    break;
                const pos = game_server.clients.indexOf(uuid);
                if (pos !== -1)
                {
                    log("client delete", [uuid]);
                    const client = connected_clients[uuid];
                    if (!client)
                        break;
                    write_db_entry(client.user.username, client.user);
                    game_server.clients[pos] = 0;
                }
                // delete connected_clients[uuid];
                break;
            }
            case 2:
            {
                const uuid = decoder.ReadStringNT();
                if (!connected_clients[uuid])
                    break;
                if (connected_clients[uuid] && connected_clients[uuid].server !== game_server.alias)
                    break;
                const user = connected_clients[uuid].user;
                user.xp = decoder.ReadFloat64();
                user.score = decoder.ReadVarUint();
                user.checkpoint = {
                    hell_creek: decoder.ReadUint8(),
                    pvp: decoder.ReadUint8(),
                };
                user.petals = {};
                user.failed_crafts = {};
                user.mob_gallery = {};
                let id = decoder.ReadUint8();
                while (id)
                {
                    const rarity = decoder.ReadUint8();
                    const count = decoder.ReadVarUint();
                    user.petals[id+':'+rarity] = count;
                    id = decoder.ReadUint8();
                }
                id = decoder.ReadUint8();
                while (id)
                {
                    const rarity = decoder.ReadUint8();
                    const count = decoder.ReadVarUint();
                    user.failed_crafts[id+':'+rarity] = count;
                    id = decoder.ReadUint8();
                }
                id = decoder.ReadUint8();
                while (id)
                {
                    const rarity = decoder.ReadUint8();
                    const count = decoder.ReadVarUint();
                    user.mob_gallery[(id - 1)+':'+rarity] = count;
                    id = decoder.ReadUint8();
                }
                await write_db_entry(uuid, user);
                break;
            }
            case 3:
            {
                let petals = {};
                let id = decoder.ReadUint8();
                while (id)
                {
                    let rarity = decoder.ReadUint8();
                    petals[`${id}:${rarity}`] ||= 0;
                    petals[`${id}:${rarity}`]++;
                    id = decoder.ReadUint8();
                }

                db_append_petals_to_logs(petals);
                break;
            }
            case 5:
            {
                const pos = decoder.ReadUint8();
                const nonce = decoder.ReadVarUint();
                const kind = decoder.ReadUint8();
                const on_crazygames = decoder.ReadUint8() !== 0;
                const payload = decoder.ReadStringNT();
                const username = game_server.clients[pos];
                const client = username && connected_clients[username];
                if (!client || client.server !== game_server.alias || client.nonce !== nonce)
                    break;
                let result;
                try {
                    result = await handle_account_request(client, kind, payload, on_crazygames);
                } catch (e) {
                    console.error("account request failed:", e);
                    result = fail("Something went wrong, try again.");
                }
                const encoder = new protocol.BinaryWriter();
                encoder.WriteUint8(3);
                encoder.WriteUint8(pos);
                encoder.WriteVarUint(nonce);
                encoder.WriteUint8(result.ok ? 1 : 0);
                encoder.WriteUint8(kind);
                encoder.WriteStringNT(result.message || "");
                encoder.WriteStringNT(result.uuid || "");
                encoder.WriteStringNT(result.token || "");
                encoder.WriteStringNT(result.code || "");
                if (ws.readyState === WSS.OPEN)
                    ws.send(encoder.data.subarray(0, encoder.at));
                break;
            }
            case 4:
            {
                // a linked player died in pvp, their score goes to the
                // CrazyGames leaderboard
                const uuid = decoder.ReadStringNT();
                const score = decoder.ReadFloat64();
                const client = connected_clients[uuid];
                if (!client || client.server !== game_server.alias)
                    break;
                if (client.user.cg_id)
                    crazygames.submit_score(client.user.cg_id, score);
                break;
            }
            case 101:
                game_server.rivet_server_id = decoder.ReadStringNT();
                log("server id recv", [game_server.rivet_server_id]);
                break;
        }
    });
    log("game connect", [game_server.alias]);
    game_servers[game_server.alias] = game_server;
    const encoder = new protocol.BinaryWriter();
    encoder.WriteUint8(0);
    encoder.WriteStringNT(game_server.alias);
    ws.send(encoder.data.subarray(0, encoder.at));
    ws.on('close', async () => {
        log("game disconnect", [game_server.alias]);
        for (const uuid of game_server.clients)
        {
            if (connected_clients[uuid] && connected_clients[uuid].server === game_server.alias)
                await write_db_entry(uuid, connected_clients[uuid].user);
            delete connected_clients[uuid];
        }
        delete game_servers[game_server.alias];
    });
});

let quit = false;
const try_save_exit = () =>
{
   if (!quit)
   {
       quit = true;
       saveDatabaseToFile();
   }
   process.exit();
}

for (const error of ["beforeExit", "exit", "SIGTERM", "SIGINT", "uncaughtException"])
    process.on(error, args => { console.log(error, args); try_save_exit() });

setInterval(saveDatabaseToFile, 10000);

setInterval(() =>  {
    // log("player count", [Object.keys(connected_clients).length]);
}, 15000);
