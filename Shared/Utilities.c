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

#include <Shared/Utilities.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void rr_log_hex(uint8_t *start, uint8_t *end)
{
    while (start != end)
        printf("%02X ", *start++);
    puts("");
}

float rr_lerp(float start, float end, float t)
{
    t = rr_fclamp(t, 0, 1);

    return (end - start) * t + start;
}

float rr_angle_lerp(float start, float end, float t)
{
    t = rr_fclamp(t, 0, 1);

    start = fmod(start, 2 * M_PI);
    end = fmod(end, 2 * M_PI);
    if (fabs(end - start) < M_PI)
        return (end - start) * t + start;
    else
    {
        if (end > start)
            start += 2 * M_PI;
        else
            end += 2 * M_PI;
        return fmod((end - start) * t + start + 2 * M_PI, 2 * M_PI);
    }
}

float rr_frand() { return (double)rand() / ((double)RAND_MAX + 1.0); }

float rr_fclamp(float v, float s, float e)
{
    if (v < s)
        v = s;
    else if (v > e)
        v = e;
    return v;
}

int rr_angle_within(float a1, float a2, float within)
{
    float diff = fmod(fmod(a1 - a2, 2 * M_PI) + 2 * M_PI, 2 * M_PI);
    return diff < within || diff > 2 * M_PI - within;
}

/* aaaack but it's fast and const should make it shared text page. */
static const unsigned char pr2six[256] = {
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64,
    64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64};

static int Base64decode_len(const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register int nprbytes;

    bufin = (const unsigned char *)bufcoded;
    while (pr2six[*(bufin++)] <= 63)
        ;

    nprbytes = (bufin - (const unsigned char *)bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}

int rr_base_64_decode(char *bufplain, const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register unsigned char *bufout;
    register int nprbytes;

    bufin = (const unsigned char *)bufcoded;
    while (pr2six[*(bufin++)] <= 63)
        ;
    nprbytes = (bufin - (const unsigned char *)bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (unsigned char *)bufplain;
    bufin = (const unsigned char *)bufcoded;

    while (nprbytes > 4)
    {
        *(bufout++) =
            (unsigned char)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) = (unsigned char)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1)
    {
        *(bufout++) =
            (unsigned char)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    }
    if (nprbytes > 2)
    {
        *(bufout++) =
            (unsigned char)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    }
    if (nprbytes > 3)
    {
        *(bufout++) = (unsigned char)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    }

    *(bufout++) = '\0';
    nbytesdecoded -= (4 - nprbytes) & 3;
    return nbytesdecoded;
}

static const char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int Base64encode_len(int len) { return ((len + 2) / 3 * 4) + 1; }

int rr_base_64_encode(char *encoded, const char *string, int len)
{
    int i;
    char *p;

    p = encoded;
    for (i = 0; i < len - 2; i += 3)
    {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int)(string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                        ((int)(string[i + 2] & 0xC0) >> 6)];
        *p++ = basis_64[string[i + 2] & 0x3F];
    }
    if (i < len)
    {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        if (i == (len - 1))
        {
            *p++ = basis_64[((string[i] & 0x3) << 4)];
            *p++ = '=';
        }
        else
        {
            *p++ = basis_64[((string[i] & 0x3) << 4) |
                            ((int)(string[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    *p++ = '\0';
    return p - encoded;
}

char *rr_sprintf(char *buf, double i)
{
    if (fabs(i) < 1e3)
        sprintf(buf, "%.0f", floor(i));
    else if (fabs(i) < 1e6)
        sprintf(buf, "%.1f K", floor(i / 1e2) / 1e1);
    else if (fabs(i) < 1e9)
        sprintf(buf, "%.2f M", floor(i / 1e4) / 1e2);
    else if (fabs(i) < 1e12)
        sprintf(buf, "%.2f B", floor(i / 1e6) / 1e3);
    else if (fabs(i) < 1e15)
        sprintf(buf, "%.2f T", floor(i / 1e8) / 1e4);
    else if (fabs(i) < 1e18)
        sprintf(buf, "%.2f Qa", floor(i / 1e10) / 1e5);
    else if (fabs(i) < 1e21)
        sprintf(buf, "%.2f Qi", floor(i / 1e12) / 1e6);
    else if (fabs(i) < 1e24)
        sprintf(buf, "%.2f Sx", floor(i / 1e14) / 1e7);
    else if (fabs(i) < 1e27)
        sprintf(buf, "%.2f Sp", floor(i / 1e16) / 1e8);
    else if (fabs(i) < 1e30)
        sprintf(buf, "%.2f Oc", floor(i / 1e18) / 1e9);
    else if (fabs(i) < 1e33)
        sprintf(buf, "%.2f No", floor(i / 1e20) / 1e10);
    else if (fabs(i) < 1e36)
        sprintf(buf, "%.2f Dc", floor(i / 1e22) / 1e11);
    else if (fabs(i) < 1e39)
        sprintf(buf, "%.2f UDc", floor(i / 1e24) / 1e12);
    else if (fabs(i) < 1e42)
        sprintf(buf, "%.2f DDc", floor(i / 1e26) / 1e13);
    else if (fabs(i) < 1e45)
        sprintf(buf, "%.2f TDc", floor(i / 1e28) / 1e14);
    else if (fabs(i) < 1e48)
        sprintf(buf, "%.2f QaDc", floor(i / 1e30) / 1e15);
    else if (fabs(i) < 1e51)
        sprintf(buf, "%.2f QiDc", floor(i / 1e32) / 1e16);
    else if (fabs(i) < 1e54)
        sprintf(buf, "%.2f SxDc", floor(i / 1e34) / 1e17);
    else if (fabs(i) < 1e57)
        sprintf(buf, "%.2f SpDc", floor(i / 1e36) / 1e18);
    else if (fabs(i) < 1e60)
        sprintf(buf, "%.2f OcDc", floor(i / 1e38) / 1e19);
    else if (fabs(i) < 1e63)
        sprintf(buf, "%.2f NoDc", floor(i / 1e40) / 1e20);
    else if (fabs(i) < 1e66)
        sprintf(buf, "%.2f Vg", floor(i / 1e42) / 1e21);
    else if (fabs(i) < 1e69)
        sprintf(buf, "%.2f UVg", floor(i / 1e44) / 1e22);
    else if (fabs(i) < 1e72)
        sprintf(buf, "%.2f DVg", floor(i / 1e46) / 1e23);
    else if (fabs(i) < 1e75)
        sprintf(buf, "%.2f TVg", floor(i / 1e48) / 1e24);
    else if (fabs(i) < 1e78)
        sprintf(buf, "%.2f QaVg", floor(i / 1e50) / 1e25);
    else if (fabs(i) < 1e81)
        sprintf(buf, "%.2f QiVg", floor(i / 1e52) / 1e26);
    else if (fabs(i) < 1e84)
        sprintf(buf, "%.2f SxVg", floor(i / 1e54) / 1e27);
    else if (fabs(i) < 1e87)
        sprintf(buf, "%.2f SpVg", floor(i / 1e56) / 1e28);
    else if (fabs(i) < 1e90)
        sprintf(buf, "%.2f OcVg", floor(i / 1e58) / 1e29);
    else if (fabs(i) < 1e93)
        sprintf(buf, "%.2f NoVg", floor(i / 1e60) / 1e30);
    else if (fabs(i) < 1e96)
        sprintf(buf, "%.2f Tg", floor(i / 1e62) / 1e31);
    else if (fabs(i) < 1e99)
        sprintf(buf, "%.2f UTg", floor(i / 1e64) / 1e32);
    else if (fabs(i) < 1e102)
        sprintf(buf, "%.2f DTg", floor(i / 1e66) / 1e33);
    else if (fabs(i) < 1e105)
        sprintf(buf, "%.2f TTg", floor(i / 1e68) / 1e34);
    else if (fabs(i) < 1e108)
        sprintf(buf, "%.2f QaTg", floor(i / 1e70) / 1e35);
    else if (fabs(i) < 1e111)
        sprintf(buf, "%.2f QiTg", floor(i / 1e72) / 1e36);
    else if (fabs(i) < 1e114)
        sprintf(buf, "%.2f SxTg", floor(i / 1e74) / 1e37);
    else if (fabs(i) < 1e117)
        sprintf(buf, "%.2f SpTg", floor(i / 1e76) / 1e38);
    else if (fabs(i) < 1e120)
        sprintf(buf, "%.2f OcTg", floor(i / 1e78) / 1e39);
    else if (fabs(i) < 1e123)
        sprintf(buf, "%.2f NoTg", floor(i / 1e80) / 1e40);
    else if (fabs(i) < 1e126)
        sprintf(buf, "%.2f Qd", floor(i / 1e82) / 1e41);
    else if (fabs(i) < 1e129)
        sprintf(buf, "%.2f UQd", floor(i / 1e84) / 1e42);
    else if (fabs(i) < 1e132)
        sprintf(buf, "%.2f DQd", floor(i / 1e86) / 1e43);
    else if (fabs(i) < 1e135)
        sprintf(buf, "%.2f TQd", floor(i / 1e88) / 1e44);
    else if (fabs(i) < 1e138)
        sprintf(buf, "%.2f QaQd", floor(i / 1e90) / 1e45);
    else if (fabs(i) < 1e141)
        sprintf(buf, "%.2f QiQd", floor(i / 1e92) / 1e46);
    else if (fabs(i) < 1e144)
        sprintf(buf, "%.2f SxQd", floor(i / 1e94) / 1e47);
    else if (fabs(i) < 1e147)
        sprintf(buf, "%.2f SpQd", floor(i / 1e96) / 1e48);
    else if (fabs(i) < 1e150)
        sprintf(buf, "%.2f OcQd", floor(i / 1e98) / 1e49);
    else if (fabs(i) < 1e153)
        sprintf(buf, "%.2f NoQd", floor(i / 1e100) / 1e50);
    else if (fabs(i) < 1e156)
        sprintf(buf, "%.2f Qt", floor(i / 1e102) / 1e51);
    else if (fabs(i) < 1e159)
        sprintf(buf, "%.2f UQt", floor(i / 1e104) / 1e52);
    else if (fabs(i) < 1e162)
        sprintf(buf, "%.2f DQt", floor(i / 1e106) / 1e53);
    else if (fabs(i) < 1e165)
        sprintf(buf, "%.2f TQt", floor(i / 1e108) / 1e54);
    else if (fabs(i) < 1e168)
        sprintf(buf, "%.2f QaQt", floor(i / 1e110) / 1e5);
    else if (fabs(i) < 1e171)
        sprintf(buf, "%.2f QiQt", floor(i / 1e112) / 1e56);
    else if (fabs(i) < 1e174)
        sprintf(buf, "%.2f SxQt", floor(i / 1e114) / 1e57);
    else if (fabs(i) < 1e177)
        sprintf(buf, "%.2f SpQt", floor(i / 1e116) / 1e58);
    else if (fabs(i) < 1e180)
        sprintf(buf, "%.2f OcQt", floor(i / 1e118) / 1e59);
    else if (fabs(i) < 1e183)
        sprintf(buf, "%.2f NoQt", floor(i / 1e120) / 1e60);
    else if (fabs(i) < 1e186)
        sprintf(buf, "%.2f Se", floor(i / 1e122) / 1e61);
    else if (fabs(i) < 1e189)
        sprintf(buf, "%.2f USe", floor(i / 1e124) / 1e62);
    else if (fabs(i) < 1e192)
        sprintf(buf, "%.2f DSe", floor(i / 1e126) / 1e63);
    else if (fabs(i) < 1e195)
        sprintf(buf, "%.2f TSe", floor(i / 1e128) / 1e64);
    else if (fabs(i) < 1e198)
        sprintf(buf, "%.2f QaSe", floor(i / 1e130) / 1e65);
    else if (fabs(i) < 1e201)
        sprintf(buf, "%.2f QiSe", floor(i / 1e132) / 1e66);
    else if (fabs(i) < 1e204)
        sprintf(buf, "%.2f SxSe", floor(i / 1e134) / 1e67);
    else if (fabs(i) < 1e207)
        sprintf(buf, "%.2f SpSe", floor(i / 1e136) / 1e68);
    else if (fabs(i) < 1e210)
        sprintf(buf, "%.2f OcSe", floor(i / 1e138) / 1e69);
    else if (fabs(i) < 1e213)
        sprintf(buf, "%.2f NoSe", floor(i / 1e140) / 1e70);
    else if (fabs(i) < 1e216)
        sprintf(buf, "%.2f St", floor(i / 1e142) / 1e71);
    else if (fabs(i) < 1e219)
        sprintf(buf, "%.2f USt", floor(i / 1e146) / 1e72);
    else if (fabs(i) < 1e222)
        sprintf(buf, "%.2f DSt", floor(i / 1e148) / 1e73);
    else if (fabs(i) < 1e225)
        sprintf(buf, "%.2f TSt", floor(i / 1e150) / 1e74);
    else if (fabs(i) < 1e228)
        sprintf(buf, "%.2f QaSt", floor(i / 1e152) / 1e75);
    else if (fabs(i) < 1e231)
        sprintf(buf, "%.2f QiSt", floor(i / 1e154) / 1e76);
    else if (fabs(i) < 1e234)
        sprintf(buf, "%.2f SxSt", floor(i / 1e156) / 1e77);
    else if (fabs(i) < 1e237)
        sprintf(buf, "%.2f SpSt", floor(i / 1e158) / 1e78);
    else if (fabs(i) < 1e240)
        sprintf(buf, "%.2f OcSt", floor(i / 1e160) / 1e79);
    else if (fabs(i) < 1e243)
        sprintf(buf, "%.2f NoSt", floor(i / 1e162) / 1e80);
    else if (fabs(i) < 1e246)
        sprintf(buf, "%.2f Og", floor(i / 1e164) / 1e81);
    else if (fabs(i) < 1e249)
        sprintf(buf, "%.2f UOg", floor(i / 1e166) / 1e82);
    else if (fabs(i) < 1e252)
        sprintf(buf, "%.2f DOg", floor(i / 1e168) / 1e83);
    else if (fabs(i) < 1e255)
        sprintf(buf, "%.2f TOg", floor(i / 1e170) / 1e84);
    else if (fabs(i) < 1e258)
        sprintf(buf, "%.2f QaOg", floor(i / 1e172) / 1e85);
    else if (fabs(i) < 1e261)
        sprintf(buf, "%.2f QiOg", floor(i / 1e174) / 1e86);
    else if (fabs(i) < 1e264)
        sprintf(buf, "%.2f SxOg", floor(i / 1e176) / 1e87);
    else if (fabs(i) < 1e267)
        sprintf(buf, "%.2f SpOg", floor(i / 1e178) / 1e88);
    else if (fabs(i) < 1e270)
        sprintf(buf, "%.2f OOg", floor(i / 1e180) / 1e89);
    else if (fabs(i) < 1e273)
        sprintf(buf, "%.2f NOg", floor(i / 1e182) / 1e90);
    else if (fabs(i) < 1e276)
        sprintf(buf, "%.2f Nn", floor(i / 1e184) / 1e91);
    else if (fabs(i) < 1e279)
        sprintf(buf, "%.2f UNn", floor(i / 1e186) / 1e92);
    else if (fabs(i) < 1e282)
        sprintf(buf, "%.2f DNn", floor(i / 1e188) / 1e93);
    else if (fabs(i) < 1e285)
        sprintf(buf, "%.2f TNn", floor(i / 1e190) / 1e94);
    else if (fabs(i) < 1e288)
        sprintf(buf, "%.2f QaNn", floor(i / 1e192) / 1e95);
    else if (fabs(i) < 1e291)
        sprintf(buf, "%.2f QiNn", floor(i / 1e194) / 1e96);
    else if (fabs(i) < 1e294)
        sprintf(buf, "%.2f SxNn", floor(i / 1e196) / 1e97);
    else if (fabs(i) < 1e297)
        sprintf(buf, "%.2f SpNn", floor(i / 1e198) / 1e98);
    else if (fabs(i) < 1e300)
        sprintf(buf, "%.2f ONn", floor(i / 1e200) / 1e99);
    else if (fabs(i) < 1e303)
        sprintf(buf, "%.2f NNn", floor(i / 1e202) / 1e100);
    else if (fabs(i) < 1e306)
        sprintf(buf, "%.2fe303", floor(i / 1e204) / 1e101);
    else if (fabs(i) < 1e307)
        sprintf(buf, "%.2fe304", floor(i / 1e205) / 1e102);
    else if (fabs(i) < 1e308)
        sprintf(buf, "%.2fe305", floor(i / 1e206) / 1e103);
    else if (i > 0)
        sprintf(buf, "∞");
    else
        sprintf(buf, "-∞");
    return buf;
}

uint8_t rr_validate_user_string(char *str)
{
    return strstr(str, u8"\ufdfd") == NULL &&
           strstr(str, u8"\U0001242b") == NULL &&
           strstr(str, u8"\U00012219") == NULL &&
           strstr(str, u8"\u2e3b") == NULL &&
           strstr(str, u8"\ua9c5") == NULL &&
           strstr(str, u8"\u102a") == NULL &&
           strstr(str, u8"\u0bf5") == NULL &&
           strstr(str, u8"\u0bf8") == NULL &&
           strstr(str, u8"\u2031") == NULL &&
           strstr(str, u8"\u00ad") != str &&
           strstr(str, u8"\u200b") != str &&
           strstr(str, u8"\u200c") != str &&
           strstr(str, u8"\u200d") != str &&
           strstr(str, u8"\u2060") != str;
}

char *rr_trim_string(char *str)
{
    char *end;
    while (isspace(*str))
        ++str;
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        --end;
    end[1] = 0;
    return str;
}
