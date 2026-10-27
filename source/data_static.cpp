#include "data.h"
const LevelDef LEVELS[3] = {{"1", 10}, {"2", 15}, {"3", 20}};   // waves per level (design doc: 10/15/20)
const u32 SEASON_COLOR[SEA_COUNT] = {C_FOREST, C_DESERT, C_SNOW, C_MOUNTAIN, C_ECLIPSE};
const u8 SEASON_ELEM[SEA_COUNT][2] = {{EL_EARTH, EL_AIR}, {EL_FIRE, EL_EARTH}, {EL_WATER, EL_AIR}, {EL_EARTH, EL_AIR}, {EL_BLOOD, EL_LIGHTNING}};
const u32 ELEM_COLOR[EL_COUNT] = {0x4A9FD9, 0xF08030, 0x9B7A4A, 0xBFE3F0, 0xC43D3D, 0xF0E050};
const u32 RARITY_COLOR[R_COUNT] = {C_TEXT, C_RARE, C_EPIC, C_LEG};
const u32 HAT_COLORS[6] = {0xC43D3D, 0x4A7FC1, 0x4FA85A, 0xE8B54B, 0x6B6480, 0xF0EAD6};
const StrId SEASON_NAME[SEA_COUNT] = {S_s_forest, S_s_desert, S_s_snow, S_s_mountain, S_s_eclipse};
// Only Forest level names exist in the design doc; other seasons reuse the doc's three names as PLACEHOLDERS.
const StrId LEVEL_NAME[3] = {S_lvl_1_1, S_lvl_1_2, S_lvl_1_3};
const StrId ELEM_NAME[EL_COUNT] = {S_el_water, S_el_fire, S_el_earth, S_el_air, S_el_blood, S_el_lightning};
const StrId RARITY_NAME[R_COUNT] = {S_r_common, S_r_rare, S_r_epic, S_r_legendary};
const StrId TYPE_NAME[TY_COUNT] = {S_ty_damage, S_ty_status, S_ty_weapon};
const StrId ACT_NAME[ACT_COUNT] = {S_act_aoe, S_act_self, S_act_touch};
const StrId WEAPON_NAME[W_COUNT] = {S_w_dagger, S_w_bow, S_w_axe, S_w_staff};
const StrId WEAPON_FIRE[W_COUNT] = {S_fire_dagger, S_fire_bow, S_fire_axe, S_fire_staff};
const StrId HAT_NAME[10] = {S_cloth_default, S_cloth_wizard, S_cloth_leaf, S_cloth_crown, S_cloth_none, S_cloth_batman, S_cloth_joker, S_cloth_terraria, S_cloth_mario, S_cloth_ironman};
const StrId FACE_NAME[6] = {S_face_beard, S_face_glasses, S_face_scar, S_face_freckles, S_face_none, S_face_dwarf_beard};
const StrId COLOR_NAME[6] = {S_col_red, S_col_blue, S_col_green, S_col_gold, S_col_gray, S_col_white};
