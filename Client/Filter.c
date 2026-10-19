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

#include <Client/Filter.h>

#include <string.h>

// Matched anywhere inside a word, so "fucking" and "motherfucker" are caught.
// Only for words that are never part of an innocent one.
static char const *const BLOCKED_ANYWHERE[] = {
    "fuck", "shit", "bitch", "nigger", "nigga", "faggot", "whore", "pussy",
    "dickhead", "asshole", "bastard", "cocksucker", "motherfuck", "wanker",
    "retard", "twat", "blowjob", "slut",
};

// Matched only as a whole word, for short words that sit inside innocent ones
// (ass in class, cock in peacock, rape in grape, tit in title).
static char const *const BLOCKED_WHOLE[] = {
    "ass", "asses", "arse", "cock", "cocks", "cunt", "cunts", "dick", "dicks",
    "dildo", "douche", "douchebag", "fag", "fags", "kike", "kikes", "chink",
    "chinks", "gook", "gooks", "spic", "spics", "coon", "coons", "paki",
    "tranny", "penis", "vagina", "rape", "raped", "rapist", "rapists",
    "porn", "porno", "tit", "tits", "wank", "jerkoff", "kys", "slag",
};

// what a character stands for when letters are swapped for look-alikes
static char fold(unsigned char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    switch (c)
    {
    case '0': return 'o';
    case '1': return 'i';
    case '3': return 'e';
    case '4': return 'a';
    case '@': return 'a';
    case '5': return 's';
    case '$': return 's';
    case '7': return 't';
    case '8': return 'b';
    }
    return c;
}

static int is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

void rr_filter_censor(char *text)
{
    size_t length = strlen(text);
    if (length == 0 || length > 1023)
        return;
    char folded[1024];
    char hit[1024] = {0};
    for (size_t i = 0; i < length; ++i)
        folded[i] = fold((unsigned char)text[i]);
    folded[length] = 0;

    for (size_t w = 0; w < sizeof BLOCKED_ANYWHERE / sizeof *BLOCKED_ANYWHERE;
         ++w)
    {
        size_t word_length = strlen(BLOCKED_ANYWHERE[w]);
        for (char *at = strstr(folded, BLOCKED_ANYWHERE[w]); at != NULL;
             at = strstr(at + 1, BLOCKED_ANYWHERE[w]))
        {
            // hide the whole word, so "fucking" does not leave "ing" behind
            size_t start = at - folded;
            size_t end = start + word_length;
            while (start > 0 && is_word_char(folded[start - 1]))
                --start;
            while (end < length && is_word_char(folded[end]))
                ++end;
            memset(hit + start, 1, end - start);
        }
    }

    // words are runs of letters and digits, "ass" in "ass!" is a word and in
    // "class" is not. Folding turns symbols like $ into letters first, so a
    // run is split on what is left after folding.
    for (size_t start = 0; start < length;)
    {
        if (!is_word_char(folded[start]))
        {
            ++start;
            continue;
        }
        size_t end = start;
        while (end < length && is_word_char(folded[end]))
            ++end;
        for (size_t w = 0; w < sizeof BLOCKED_WHOLE / sizeof *BLOCKED_WHOLE;
             ++w)
            if (strlen(BLOCKED_WHOLE[w]) == end - start &&
                memcmp(folded + start, BLOCKED_WHOLE[w], end - start) == 0)
                memset(hit + start, 1, end - start);
        start = end;
    }

    for (size_t i = 0; i < length; ++i)
        if (hit[i])
            text[i] = '*';
}
