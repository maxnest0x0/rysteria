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

class GameClient
{
    constructor(user, server, nonce) 
    {
        this.user = user;
        this.server = server;
        this.nonce = nonce;
        this.needs_gameserver_update = false;
        this.needs_database_update = false;
    }

    write(encoder) 
    {
        const user = this.user;
        encoder.WriteVarUint(this.nonce);
        encoder.WriteStringNT(user.username);
        encoder.WriteStringNT(user.password);
        encoder.WriteStringNT(user.discord_id);
        encoder.WriteFloat64(user.xp);
        encoder.WriteUint8(user.checkpoint);
        for (const petal of Object.keys(user.petals))
        {
            if (!(user.petals[petal] > 0))
                continue;
            const [id, rarity] = petal.split(":");
            encoder.WriteUint8(id);
            encoder.WriteUint8(rarity);
            encoder.WriteVarUint(user.petals[petal]);
        }
        encoder.WriteUint8(0);
        for (const petal of Object.keys(user.failed_crafts))
        {
            if (!(user.failed_crafts[petal] > 0))
                continue;
            const [id, rarity] = petal.split(":");
            encoder.WriteUint8(id);
            encoder.WriteUint8(rarity);
            encoder.WriteVarUint(user.failed_crafts[petal]);
        }
        encoder.WriteUint8(0);
        for (const mob of Object.keys(user.mob_gallery))
        {
            if (!(user.mob_gallery[mob] > 0))
                continue;
            const [id, rarity] = mob.split(":");
            encoder.WriteUint8(+id + 1);
            encoder.WriteUint8(rarity);
            encoder.WriteVarUint(user.mob_gallery[mob]);
        }
        encoder.WriteUint8(0);
        this.needs_gameserver_update = false;
    }
}

module.exports = GameClient;