import { readFileSync } from 'fs';

const db = JSON.parse(readFileSync('database.json', 'utf8'));

function xp_to_reach_level(level) {
    if (level <= 60)
        return (level + 5) * Math.pow(1.175, level);
    let base = (level + 5) * Math.pow(1.175, 60);
    for (let i = 60; i < level; ++i)
        base *= Math.min(Math.max(1.18 - 0.0075 * (i - 60), 1.1), 1.18);
    return base;
}

function level_from_xp(xp) {
    let level = 1;
    while (xp >= xp_to_reach_level(level + 1))
        xp -= xp_to_reach_level(++level);
    return level;
}

function xp_from_level(level) {
    let xp = 0;
    for (let lvl = 2; lvl <= level; ++lvl)
        xp += xp_to_reach_level(lvl);
    return xp;
}

const arr = [];
for (const uuid in db) {
    if (uuid === 'accounts' || uuid === 'links') continue;
    db[uuid].exos = 0;
    db[uuid].aatts = 0;
    db[uuid].mobs = 0;
    for (const item in db[uuid].petals)
        if (item.endsWith(':6')) db[uuid].exos += db[uuid].petals[item];
    for (const item in db[uuid].failed_crafts)
        if (item.endsWith(':6')) db[uuid].aatts += db[uuid].failed_crafts[item];
    for (const item in db[uuid].mob_gallery)
        db[uuid].mobs += db[uuid].mob_gallery[item];
    db[uuid].level = level_from_xp(db[uuid].xp);
    db[uuid].id = uuid.split('-')[0];
    arr.push(db[uuid]);
}

console.log(new Date());
console.log('#', 'uuid', 'xp', 'lvl', 'exos', 'aatts', 'mobs');

console.log('sorted by xp');
arr.sort((a, b) => b.xp - a.xp);
let i = 0;
for (const acc of arr.slice(0, 50)) console.log(++i, acc.id, acc.xp, acc.level, acc.exos, acc.aatts, acc.mobs);

console.log('sorted by exos');
arr.sort((a, b) => b.exos - a.exos);
i = 0;
for (const acc of arr.slice(0, 50)) console.log(++i, acc.id, acc.xp, acc.level, acc.exos, acc.aatts, acc.mobs);

console.log('sorted by mobs');
arr.sort((a, b) => b.mobs - a.mobs);
i = 0;
for (const acc of arr.slice(0, 50)) console.log(++i, acc.id, acc.xp, acc.level, acc.exos, acc.aatts, acc.mobs);
