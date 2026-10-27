#include "i18n.h"
#include <stdio.h>
#include <string.h>
int g_lang = LANG_EN;

static const char* pick(const char* loc, const char* en) { return (loc && loc[0]) ? loc : (en ? en : ""); }
const char* T(StrId id) {
  if ((int)id < 0 || id >= S_COUNT) return "";
  return pick(UI_TABLE[g_lang][id], UI_TABLE[LANG_EN][id]);
}
const char* Tn(StrId id, int n) {
  static char buf[4][96];
  static int k = 0;
  char* out = buf[k = (k + 1) & 3];
  const char* s = T(id);
  size_t o = 0;
  for (; *s && o < 90; ++s) {
    if (s[0] == '{' && (s[1] == 'n' || s[1] == 'v') && s[2] == '}') {
      o += (size_t)snprintf(out + o, 95 - o, "%d", n);
      s += 2;
    } else out[o++] = *s;
  }
  out[o] = 0;
  return out;
}
#define SPELL_FIELD(fn, field) \
  const char* fn(int i) { if (i < 0 || i >= NUM_SPELLS) return ""; return pick(SPELL_TEXT[g_lang][i].field, SPELL_TEXT[LANG_EN][i].field); }
SPELL_FIELD(spellName, name)
SPELL_FIELD(spellDesc, desc)
SPELL_FIELD(spellStats, stats)
#define ENEMY_FIELD(fn, field) \
  const char* fn(int i) { if (i < 0 || i >= NUM_ENEMIES) return ""; return pick(ENEMY_TEXT[g_lang][i].field, ENEMY_TEXT[LANG_EN][i].field); }
ENEMY_FIELD(enemyName, name)
ENEMY_FIELD(enemyDesc, desc)
ENEMY_FIELD(enemySpecial, special)
const char* spellLore(int i) { if (i < 0 || i >= NUM_SPELLS) return ""; return pick(SPELL_LORE[g_lang][i], SPELL_LORE[LANG_EN][i]); }
const char* enemyLore(int i) { if (i < 0 || i >= NUM_ENEMIES) return ""; return pick(ENEMY_LORE[g_lang][i], ENEMY_LORE[LANG_EN][i]); }
