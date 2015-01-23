/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License v. 2.0 as published by Mozilla.

 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License v. 2.0 for more details.

 * You should have received a copy of the Mozilla Public License v. 2.0
 * along with the Game Closure SDK.  If not, see <http://mozilla.org/MPL/2.0/>.
 */

/**
 * @file	 rgba.c
 * @brief
 */
#include "core/rgba.h"
#include "core/log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "core/deps/uthash/uthash.h"

typedef struct html_color_t {
    const char *name;
    rgba color;
    UT_hash_handle hh;
} html_color;

static html_color *defaults = NULL;

#define ADD_COLOR(color) HASH_ADD_KEYPTR(hh, defaults, (color).name, strlen((color).name), (&color))

static html_color colors[] = {
    {"aliceblue",            {240, 248, 255, 1}},
    {"antiquewhite",         {250, 235, 215, 1}},
    {"aqua",                 {  0, 255, 255, 1}},
    {"aquamarine",           {127, 255, 212, 1}},
    {"azure",                {240, 255, 255, 1}},
    {"beige",                {245, 245, 220, 1}},
    {"bisque",               {255, 228, 196, 1}},
    {"black",                {  0,   0,   0, 1}},
    {"blanchedalmond",       {255, 235, 205, 1}},
    {"blue",                 {  0,   0, 255, 1}},
    {"blueviolet",           {138,  43, 226, 1}},
    {"brown",                {165,  42,  42, 1}},
    {"burlywood",            {222, 184, 135, 1}},
    {"cadetblue",            { 95, 158, 160, 1}},
    {"chartreuse",           {127, 255,   0, 1}},
    {"chocolate",            {210, 105,  30, 1}},
    {"coral",                {255, 127,  80, 1}},
    {"cornflowerblue",       {100, 149, 237, 1}},
    {"cornsilk",             {255, 248, 220, 1}},
    {"crimson",              {220,  20,  60, 1}},
    {"cyan",                 {  0, 255, 255, 1}},
    {"darkblue",             {  0,   0, 139, 1}},
    {"darkcyan",             {  0, 139, 139, 1}},
    {"darkgoldenrod",        {184, 134,  11, 1}},
    {"darkgray",             {169, 169, 169, 1}},
    {"darkgreen",            {  0, 100,   0, 1}},
    {"darkgrey",             {169, 169, 169, 1}},
    {"darkkhaki",            {189, 183, 107, 1}},
    {"darkmagenta",          {139,   0, 139, 1}},
    {"darkolivegreen",       { 85, 107,  47, 1}},
    {"darkorange",           {255, 140,   0, 1}},
    {"darkorchid",           {153,  50, 204, 1}},
    {"darkred",              {139,   0,   0, 1}},
    {"darksalmon",           {233, 150, 122, 1}},
    {"darkseagreen",         {143, 188, 143, 1}},
    {"darkslateblue",        { 72,  61, 139, 1}},
    {"darkslategray",        { 47,  79,  79, 1}},
    {"darkslategrey",        { 47,  79,  79, 1}},
    {"darkturquoise",        {  0, 206, 209, 1}},
    {"darkviolet",           {148,   0, 211, 1}},
    {"deeppink",             {255,  20, 147, 1}},
    {"deepskyblue",          {  0, 191, 255, 1}},
    {"dimgray",              {105, 105, 105, 1}},
    {"dimgrey",              {105, 105, 105, 1}},
    {"dodgerblue",           { 30, 144, 255, 1}},
    {"firebrick",            {178,  34,  34, 1}},
    {"floralwhite",          {255, 250, 240, 1}},
    {"forestgreen",          { 34, 139,  34, 1}},
    {"fuchsia",              {255,   0, 255, 1}},
    {"gainsboro",            {220, 220, 220, 1}},
    {"ghostwhite",           {248, 248, 255, 1}},
    {"gold",                 {255, 215,   0, 1}},
    {"goldenrod",            {218, 165,  32, 1}},
    {"gray",                 {128, 128, 128, 1}},
    {"grey",                 {128, 128, 128, 1}},
    {"green",                {  0, 128,   0, 1}},
    {"greenyellow",          {173, 255,  47, 1}},
    {"honeydew",             {240, 255, 240, 1}},
    {"hotpink",              {255, 105, 180, 1}},
    {"indianred",            {205,  92,  92, 1}},
    {"indigo",               { 75,   0, 130, 1}},
    {"ivory",                {255, 255, 240, 1}},
    {"khaki",                {240, 230, 140, 1}},
    {"lavender",             {230, 230, 250, 1}},
    {"lavenderblush",        {255, 240, 245, 1}},
    {"lawngreen",            {124, 252,   0, 1}},
    {"lemonchiffon",         {255, 250, 205, 1}},
    {"lightblue",            {173, 216, 230, 1}},
    {"lightcoral",           {240, 128, 128, 1}},
    {"lightcyan",            {224, 255, 255, 1}},
    {"lightgoldenrodyellow", {250, 250, 210, 1}},
    {"lightgray",            {211, 211, 211, 1}},
    {"lightgreen",           {144, 238, 144, 1}},
    {"lightgrey",            {211, 211, 211, 1}},
    {"lightpink",            {255, 182, 193, 1}},
    {"lightsalmon",          {255, 160, 122, 1}},
    {"lightseagreen",        { 32, 178, 170, 1}},
    {"lightskyblue",         {135, 206, 250, 1}},
    {"lightslategray",       {119, 136, 153, 1}},
    {"lightslategrey",       {119, 136, 153, 1}},
    {"lightsteelblue",       {176, 196, 222, 1}},
    {"lightyellow",          {255, 255, 224, 1}},
    {"lime",                 {  0, 255,   0, 1}},
    {"limegreen",            { 50, 205,  50, 1}},
    {"linen",                {250, 240, 230, 1}},
    {"magenta",              {255,   0, 255, 1}},
    {"maroon",               {128,   0,   0, 1}},
    {"mediumaquamarine",     {102, 205, 170, 1}},
    {"mediumblue",           {  0,   0, 205, 1}},
    {"mediumorchid",         {186,  85, 211, 1}},
    {"mediumpurple",         {147, 112, 219, 1}},
    {"mediumseagreen",       { 60, 179, 113, 1}},
    {"mediumslateblue",      {123, 104, 238, 1}},
    {"mediumspringgreen",    {  0, 250, 154, 1}},
    {"mediumturquoise",      { 72, 209, 204, 1}},
    {"mediumvioletred",      {199,  21, 133, 1}},
    {"midnightblue",         { 25,  25, 112, 1}},
    {"mintcream",            {245, 255, 250, 1}},
    {"mistyrose",            {255, 228, 225, 1}},
    {"moccasin",             {255, 228, 181, 1}},
    {"navajowhite",          {255, 222, 173, 1}},
    {"navy",                 {  0,   0, 128, 1}},
    {"oldlace",              {253, 245, 230, 1}},
    {"olive",                {128, 128,   0, 1}},
    {"olivedrab",            {107, 142,  35, 1}},
    {"orange",               {255, 165,   0, 1}},
    {"orangered",            {255,  69,   0, 1}},
    {"orchid",               {218, 112, 214, 1}},
    {"palegoldenrod",        {238, 232, 170, 1}},
    {"palegreen",            {152, 251, 152, 1}},
    {"paleturquoise",        {175, 238, 238, 1}},
    {"palevioletred",        {219, 112, 147, 1}},
    {"papayawhip",           {255, 239, 213, 1}},
    {"peachpuff",            {255, 218, 185, 1}},
    {"peru",                 {205, 133,  63, 1}},
    {"pink",                 {255, 192, 203, 1}},
    {"plum",                 {221, 160, 221, 1}},
    {"powderblue",           {176, 224, 230, 1}},
    {"purple",               {128,   0, 128, 1}},
    {"red",                  {255,   0,   0, 1}},
    {"rosybrown",            {188, 143, 143, 1}},
    {"royalblue",            { 65, 105, 225, 1}},
    {"saddlebrown",          {139,  69,  19, 1}},
    {"salmon",               {250, 128, 114, 1}},
    {"sandybrown",           {244, 164,  96, 1}},
    {"seagreen",             { 46, 139,  87, 1}},
    {"seashell",             {255, 245, 238, 1}},
    {"sienna",               {160,  82,  45, 1}},
    {"silver",               {192, 192, 192, 1}},
    {"skyblue",              {135, 206, 235, 1}},
    {"slateblue",            {106,  90, 205, 1}},
    {"slategray",            {112, 128, 144, 1}},
    {"slategrey",            {112, 128, 144, 1}},
    {"snow",                 {255, 250, 250, 1}},
    {"springgreen",          {  0, 255, 127, 1}},
    {"steelblue",            { 70, 130, 180, 1}},
    {"tan",                  {210, 180, 140, 1}},
    {"teal",                 {  0, 128, 128, 1}},
    {"thistle",              {216, 191, 216, 1}},
    {"tomato",               {255,  99,  71, 1}},
    {"transparent",          {  0,   0,   0, 0}},
    {"turquoise",            { 64, 224, 208, 1}},
    {"violet",               {238, 130, 238, 1}},
    {"wheat",                {245, 222, 179, 1}},
    {"white",                {255, 255, 255, 1}},
    {"whitesmoke",           {245, 245, 245, 1}},
    {"yellow",               {255, 255,   0, 1}},
    {"yellowgreen",          {154, 205,  50, 1}},
    {NULL}
};


/**
 * @name	rgba_init
 * @brief
 * @retval	NONE
 */
void rgba_init() {
    static bool done_rgba_init = false;

    if (!done_rgba_init) {
        done_rgba_init = true;
        int i = 0;

        for (; colors[i].name != NULL; i++) {
            ADD_COLOR(colors[i]);
        }
    }
}

/**
 * @name	rgba_equals
 * @brief	checks if the given rgba values are equal
 * @param	a - (rgba *) first rgba to compare
 * @param	b - (rgba *) second rgba to compare
 * @retval	bool - (true | false)
 */
bool rgba_equals(rgba *a, rgba *b) {
    return a->r == b->r && a->g == b->g && a->b == b->b && a->a == b->a;
}


/**
 * @name	rgba_parse
 * @brief	parses the given color string into the given rgba object
 * @param	color - (rgba *) rgba object which will hold the parsed color string
 * @param	src - (const char *) color string to be parsed
 * @retval	NONE
 */
void rgba_parse(rgba *color, const char *src) {
    int r = 0, g = 0, b = 0;
    float a = 1;
    size_t n = strlen(src);

    if (src[0] == '#') {
        char buf[3];
        buf[2] = '\0';

        if (n == 4) {
            buf[0] = src[1];
            buf[1] = src[1];
            r = (int)strtol(buf, NULL, 16);
            buf[0] = src[2];
            buf[1] = src[2];
            g = (int)strtol(buf, NULL, 16);
            buf[0] = src[3];
            buf[1] = src[3];
            b = (int)strtol(buf, NULL, 16);
        } else if (n >= 7) {
            buf[0] = src[1];
            buf[1] = src[2];
            r = (int)strtol(buf, NULL, 16);
            buf[0] = src[3];
            buf[1] = src[4];
            g = (int)strtol(buf, NULL, 16);
            buf[0] = src[5];
            buf[1] = src[6];
            b = (int)strtol(buf, NULL, 16);
        }

        if (n == 9) {
            buf[0] = src[7];
            buf[1] = src[8];
            a = strtol(buf, NULL, 16) / 255.0;
        }
    } else if (src[0] == 'r' && src[1] == 'g' && src[2] == 'b') {
        bool has_alpha = (src[3] == 'a');
        unsigned int i = 3;

        while (src[i++] != '(') {
            if (i == n) {
                return;
            }
        }

        r = atoi(src + i);

        while (src[i++] != ',') {
            if (i == n) {
                return;
            }
        }

        g = atoi(src + i);

        while (src[i++] != ',') {
            if (i == n) {
                return;
            }
        }

        b = atoi(src + i);

        if (has_alpha) {
            while (src[i++] != ',') {
                if (i == n) {
                    return;
                }
            }

            a = strtod(src + i, NULL);
        }
    } else {
        html_color *c;
        HASH_FIND_STR(defaults, src, c);

        if (c) {
            r = c->color.r;
            b = c->color.b;
            g = c->color.g;
            a = c->color.a;
        } else {
            a = 0;
        }
    }

    if (r < 0) {
        r = 0;
    }

    if (r > 255) {
        r = 255;
    }

    if (g < 0) {
        g = 0;
    }

    if (g > 255) {
        g = 255;
    }

    if (b < 0) {
        b = 0;
    }

    if (b > 255) {
        b = 255;
    }

    if (a < 0) {
        a = 0;
    }

    if (a > 1) {
        a = 1;
    }

    color->r = r / 255.0;
    color->g = g / 255.0;
    color->b = b / 255.0;
    color->a = a;
}

/**
 * @name	rgba_to_string
 * @brief	converts the given rgba color value into the given string
 * @param	color - (rgba *) rgba color to convert
 * @param	buf - (char *) string to be converted into
 * @retval	int -
 */
int rgba_to_string(rgba *color, char *buf) {
    LOGFN("rgba_to_string");
    return snprintf(buf, RGBA_MAX_STR_LEN, "rgba(%i, %i, %i, %f)", (int)(color->r * 255), (int)(color->g * 255), (int)(color->b * 255), color->a);
}

/**
 * @name	rgba_print
 * @brief	pretty print the given rgba color
 * @param	color - (rgba *)
 * @retval	NONE
 */
void rgba_print(rgba *color) {
    char buf[RGBA_MAX_STR_LEN];
    rgba_to_string(color, buf);
    LOG("%s", buf);
}
