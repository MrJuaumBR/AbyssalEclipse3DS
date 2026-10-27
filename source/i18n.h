// Data-driven localization with English fallback (design doc: "Reliability Rules").
#pragma once
#include "data.h"
extern int g_lang;                         // Lang
const char* T(StrId id);                   // UI string (falls back to English, then to "")
const char* Tn(StrId id, int n);           // UI string with {n} / {v} replaced (static buffer, ring of 4)
const char* spellName(int idx);
const char* spellDesc(int idx);
const char* spellStats(int idx);
const char* spellLore(int idx);            // "" if none in any language
const char* enemyName(int idx);
const char* enemyDesc(int idx);
const char* enemySpecial(int idx);
const char* enemyLore(int idx);
