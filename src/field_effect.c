#include "global.h"
#include "gflib.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_camera.h"
#include "field_control_avatar.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_effect_scripts.h"
#include "field_fadetransition.h"
#include "field_player_avatar.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "help_system.h"
#include "metatile_behavior.h"
#include "new_menu_helpers.h"
#include "overworld.h"
#include "party_menu.h"
#include "quest_log.h"
#include "script.h"
#include "special_field_anim.h"
#include "task.h"
#include "trainer_pokemon_sprites.h"
#include "trig.h"
#include "util.h"
#include "constants/event_object_movement.h"
#include "constants/metatile_behaviors.h"
#include "constants/songs.h"

#define subsprite_table(ptr) {.subsprites = ptr, .subspriteCount = (sizeof ptr) / (sizeof(struct Subsprite))}
#define FIELD_EFFECT_COUNT 32

static u8 sFieldEffectActiveList[FIELD_EFFECT_COUNT];

void FieldEffectActiveListAdd(u8 fldeff);
bool8 FieldEffectCmd_loadtiles(const u8 **script, u32 *result);
bool8 FieldEffectCmd_loadfadedpal(const u8 **script, u32 *result);
bool8 FieldEffectCmd_loadpal(const u8 **script, u32 *result);
bool8 FieldEffectCmd_callnative(const u8 **script, u32 *result);
bool8 FieldEffectCmd_end(const u8 **script, u32 *result);
bool8 FieldEffectCmd_loadgfx_callnative(const u8 **script, u32 *result);
bool8 FieldEffectCmd_loadtiles_callnative(const u8 **script, u32 *result);
bool8 FieldEffectCmd_loadfadedpal_callnative(const u8 **script, u32 *result);
void FieldEffectScript_LoadTiles(const u8 **script);
void FieldEffectScript_LoadFadedPal(const u8 **script);
void FieldEffectScript_LoadPal(const u8 **script);
void FieldEffectScript_CallNative(const u8 **script, u32 *result);
void FieldEffectFreeTilesIfUnused(u16 tilesTag);
void FieldEffectFreePaletteIfUnused(u8 paletteNum);
void Task_PokecenterHeal(u8 taskId);
void SpriteCB_PokeballGlow(struct Sprite * sprite);
void SpriteCB_PokecenterMonitor(struct Sprite * sprite);
void SpriteCB_HallOfFameMonitor(struct Sprite * sprite);

const u16 sNewGameOakObjectSpriteTiles[] = INCBIN_U16("graphics/field_effects/unk_83CA770.4bpp");
const u16 sNewGameOakObjectPals[] = INCBIN_U16("graphics/field_effects/unk_83CAF70.gbapal");
const u16 gUnknown_83CAF90[] = INCBIN_U16("graphics/field_effects/unk_83CAF90.4bpp");
const u16 gUnknown_83CAFB0[] = INCBIN_U16("graphics/field_effects/unk_83CAFB0.gbapal");
const u16 gUnknown_83CAFD0[] = INCBIN_U16("graphics/field_effects/unk_83CAFD0.4bpp");
const u16 gUnknown_83CB3D0[] = INCBIN_U16("graphics/field_effects/unk_83CB3D0.gbapal");
const u16 gUnknown_83CB3F0[] = INCBIN_U16("graphics/field_effects/unk_83CB3F0.4bpp");
const u16 gFieldMoveStreaksTiles[] = INCBIN_U16("graphics/field_effects/unk_83CB5F0.4bpp");
const u16 gFieldMoveStreaksPalette[] = INCBIN_U16("graphics/field_effects/unk_83CB7F0.gbapal");
const u16 gFieldMoveStreaksTilemap[] = INCBIN_U16("graphics/field_effects/unk_83CB810.bin");
const u16 gDarknessFieldMoveStreaksTiles[] = INCBIN_U16("graphics/field_effects/unk_83CBA90.4bpp");
const u16 gDarknessFieldMoveStreaksPalette[] = INCBIN_U16("graphics/field_effects/unk_83CBB10.gbapal");
const u16 gDarknessFieldMoveStreaksTilemap[] = INCBIN_U16("graphics/field_effects/unk_83CBB30.bin");
const u16 gUnknown_83CBDB0[] = INCBIN_U16("graphics/field_effects/unk_83CBDB0.4bpp");

bool8 (*const sFldEffScrcmdTable[])(const u8 **script, u32 *result) = {
    FieldEffectCmd_loadtiles,
    FieldEffectCmd_loadfadedpal,
    FieldEffectCmd_loadpal,
    FieldEffectCmd_callnative,
    FieldEffectCmd_end,
    FieldEffectCmd_loadgfx_callnative,
    FieldEffectCmd_loadtiles_callnative,
    FieldEffectCmd_loadfadedpal_callnative
};

const struct OamData gNewGameOakOamAttributes = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0x000,
    .priority = 0,
    .paletteNum = 0x0,
    .affineParam = 0
};

const struct OamData gOamData_83CBE58 = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0x000,
    .priority = 0,
    .paletteNum = 0x0,
    .affineParam = 0
};

const struct OamData gOamData_83CBE60 = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0x000,
    .priority = 0,
    .paletteNum = 0x0,
    .affineParam = 0
};

const struct SpriteFrameImage gNewGameOakObjectSpriteFrames[] = {
    {sNewGameOakObjectSpriteTiles, 0x800}
};

const struct SpritePalette gNewGameOakObjectPaletteInfo = {
    sNewGameOakObjectPals, 4102
};

const union AnimCmd gNewGameOakAnim[] = {
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};

const union AnimCmd *const gNewGameOakAnimTable[] = {
    gNewGameOakAnim
};

const struct SpriteTemplate gNewGameOakObjectTemplate = {
    .tileTag = 0xFFFF,
    .paletteTag = 4102,
    .oam = &gNewGameOakOamAttributes,
    .anims = gNewGameOakAnimTable,
    .images = gNewGameOakObjectSpriteFrames,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

const struct SpritePalette gUnknown_83CBE9C = {
    gUnknown_83CAFB0, 4103
};

const struct SpritePalette gUnknown_83CBEA4 = {
    gUnknown_83CB3D0, 4112
};

const struct OamData gOamData_83CBEAC = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0x000,
    .priority = 0,
    .paletteNum = 0x0,
    .affineParam = 0
};

const struct SpriteFrameImage gUnknown_83CBEB4[] = {
    {gUnknown_83CAF90, 0x20}
};

const struct SpriteFrameImage gUnknown_83CBEBC[] = {
    {gUnknown_83CAFD0 + 0x000, 0x100},
    {gUnknown_83CAFD0 + 0x080, 0x100},
    {gUnknown_83CAFD0 + 0x100, 0x100},
    {gUnknown_83CAFD0 + 0x180, 0x100}
};

const struct SpriteFrameImage gUnknown_83CBEDC[] = {
    {gUnknown_83CB3F0 + 0x00, 0x80},
    {gUnknown_83CB3F0 + 0x40, 0x80},
    {gUnknown_83CB3F0 + 0x80, 0x80},
    {gUnknown_83CB3F0 + 0xC0, 0x80}
};

const struct Subsprite gUnknown_83CBEFC[] =
{
    {
        .x = -12,
        .y =  -8,
        .shape = SPRITE_SHAPE(16x8),
        .size = SPRITE_SIZE(16x8),
        .tileOffset = 0,
        .priority = 2
    }, {
        .x =  4,
        .y = -8,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 2,
        .priority = 2
    }, {
        .x = -12,
        .y =   0,
        .shape = SPRITE_SHAPE(16x8),
        .size = SPRITE_SIZE(16x8),
        .tileOffset = 3,
        .priority = 2
    }, {
        .x = 4,
        .y = 0,
        .shape = SPRITE_SHAPE(8x8),
        .size = SPRITE_SIZE(8x8),
        .tileOffset = 5,
        .priority = 2
    }
};

const struct SubspriteTable gUnknown_83CBF0C = subsprite_table(gUnknown_83CBEFC);

const struct Subsprite gUnknown_83CBF14[] =
{
    {
        .x = -32,
        .y = -8,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 0,
        .priority = 2
    }, {
        .x =  0,
        .y = -8,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 4,
        .priority = 2
    }, {
        .x = -32,
        .y =  0,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 8,
        .priority = 2
    }, {
        .x =   0,
        .y =  0,
        .shape = SPRITE_SHAPE(32x8),
        .size = SPRITE_SIZE(32x8),
        .tileOffset = 12,
        .priority = 2
    }
};

const struct SubspriteTable gUnknown_83CBF24 = subsprite_table(gUnknown_83CBF14);

const union AnimCmd gUnknown_83CBF2C[] = {
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_JUMP(0)
};

const union AnimCmd gUnknown_83CBF34[] = {
    ANIMCMD_FRAME(1, 5),
    ANIMCMD_FRAME(2, 5),
    ANIMCMD_FRAME(3, 7),
    ANIMCMD_FRAME(2, 5),
    ANIMCMD_FRAME(1, 5),
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_LOOP(3),
    ANIMCMD_END
};

const union AnimCmd *const gUnknown_83CBF54[] = {
    gUnknown_83CBF2C,
    gUnknown_83CBF34
};

const union AnimCmd gUnknown_83CBF5C[] = {
    ANIMCMD_FRAME(3, 8),
    ANIMCMD_FRAME(2, 8),
    ANIMCMD_FRAME(1, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(1, 8),
    ANIMCMD_FRAME(2, 8),
    ANIMCMD_LOOP(2),
    ANIMCMD_FRAME(1, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END
};

const union AnimCmd *const gUnknown_83CBF84[] = {
    gUnknown_83CBF5C
};

const struct SpriteTemplate gUnknown_83CBF88 = {
    .tileTag = 65535,
    .paletteTag = 4103,
    .oam = &gOamData_83CBE58,
    .anims = gUnknown_83CBF54,
    .images = gUnknown_83CBEB4,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_PokeballGlow
};

const struct SpriteTemplate gUnknown_83CBFA0 = {
    .tileTag = 65535,
    .paletteTag = 4103,
    .oam = &gOamData_83CBEAC,
    .anims = gUnknown_83CBF54,
    .images = gUnknown_83CBEBC,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_PokecenterMonitor
};

const struct SpriteTemplate gUnknown_83CBFB8 = {
    .tileTag = 65535,
    .paletteTag = 4112,
    .oam = &gOamData_83CBE60,
    .anims = gUnknown_83CBF84,
    .images = gUnknown_83CBEDC,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_HallOfFameMonitor
};


u32 FieldEffectStart(u8 fldeff)
{
    const u8 *script;
    u32 result;
    FieldEffectActiveListAdd(fldeff);
    script = gFieldEffectScriptPointers[fldeff];
    while (sFldEffScrcmdTable[*script](&script, &result))
        ;
    return result;
}

bool8 FieldEffectCmd_loadtiles(const u8 **script, u32 *result)
{
    (*script)++;
    FieldEffectScript_LoadTiles(script);
    return TRUE;
}

bool8 FieldEffectCmd_loadfadedpal(const u8 **script, u32 *result)
{
    (*script)++;
    FieldEffectScript_LoadFadedPal(script);
    return TRUE;
}

bool8 FieldEffectCmd_loadpal(const u8 **script, u32 *result)
{
    (*script)++;
    FieldEffectScript_LoadPal(script);
    return TRUE;
}
bool8 FieldEffectCmd_callnative(const u8 **script, u32 *result)
{
    (*script)++;
    FieldEffectScript_CallNative(script, result);
    return TRUE;
}

bool8 FieldEffectCmd_end(const u8 **script, u32 *result)
{
    return FALSE;
}

bool8 FieldEffectCmd_loadgfx_callnative(const u8 **script, u32 *result)
{
    (*script)++;
    FieldEffectScript_LoadTiles(script);
    FieldEffectScript_LoadFadedPal(script);
    FieldEffectScript_CallNative(script, result);
    return TRUE;
}

bool8 FieldEffectCmd_loadtiles_callnative(const u8 **script, u32 *result)
{
    (*script)++;
    FieldEffectScript_LoadTiles(script);
    FieldEffectScript_CallNative(script, result);
    return TRUE;
}

bool8 FieldEffectCmd_loadfadedpal_callnative(const u8 **script, u32 *result)
{
    (*script)++;
    FieldEffectScript_LoadFadedPal(script);
    FieldEffectScript_CallNative(script, result);
    return TRUE;
}

u32 FieldEffectScript_ReadWord(const u8 **script)
{
    return T2_READ_32(*script);
}

void FieldEffectScript_LoadTiles(const u8 **script)
{
    const struct SpriteSheet * spriteSheet = (const struct SpriteSheet * )FieldEffectScript_ReadWord(script);
    if (GetSpriteTileStartByTag(spriteSheet->tag) == 0xFFFF)
        LoadSpriteSheet(spriteSheet);
    *script += sizeof(u32);
}

void sub_8083598(u8 paletteIdx)
{
    switch (gUnknown_2036E28)
    {
    case 0:
        return;
    case 1:
        TintPalette_GrayScale(&gPlttBufferUnfaded[(paletteIdx + 16) * 16], 0x10);
        break;
    case 2:
        TintPalette_SepiaTone(&gPlttBufferUnfaded[(paletteIdx + 16) * 16], 0x10);
        break;
    case 3:
        sub_8111F38((paletteIdx + 16) * 16, 0x10);
        TintPalette_GrayScale(&gPlttBufferUnfaded[(paletteIdx + 16) * 16], 0x10);
        break;
    default:
        return;
    }
    CpuFastCopy(&gPlttBufferUnfaded[(paletteIdx + 16) * 16], &gPlttBufferFaded[(paletteIdx + 16) * 16], 0x20);
}

void FieldEffectScript_LoadFadedPal(const u8 **script)
{
    const struct SpritePalette * spritePalette = (const struct SpritePalette * )FieldEffectScript_ReadWord(script);
    u8 idx = IndexOfSpritePaletteTag(spritePalette->tag);
    LoadSpritePalette(spritePalette);
    if (idx == 0xFF)
        sub_8083598(IndexOfSpritePaletteTag(spritePalette->tag));
    sub_807AA8C(IndexOfSpritePaletteTag(spritePalette->tag));
    *script += sizeof(u32);
}

void FieldEffectScript_LoadPal(const u8 **script)
{
    const struct SpritePalette * spritePalette = (const struct SpritePalette * )FieldEffectScript_ReadWord(script);
    u8 idx = IndexOfSpritePaletteTag(spritePalette->tag);
    LoadSpritePalette(spritePalette);
    if (idx != 0xFF)
        sub_8083598(IndexOfSpritePaletteTag(spritePalette->tag));
    *script += sizeof(u32);
}

void FieldEffectScript_CallNative(const u8 **script, u32 *result)
{
    u32 (*func)(void) = (u32 (*)(void))FieldEffectScript_ReadWord(script);
    *result = func();
    *script += sizeof(u32);
}

void FieldEffectFreeGraphicsResources(struct Sprite * sprite)
{
    u16 tileStart = sprite->sheetTileStart;
    u8 paletteNum = sprite->oam.paletteNum;
    DestroySprite(sprite);
    FieldEffectFreeTilesIfUnused(tileStart);
    FieldEffectFreePaletteIfUnused(paletteNum);
}

void FieldEffectStop(struct Sprite * sprite, u8 fldeff)
{
    FieldEffectFreeGraphicsResources(sprite);
    FieldEffectActiveListRemove(fldeff);
}

void FieldEffectFreeTilesIfUnused(u16 tileStart)
{
    u8 i;
    u16 tileTag = GetSpriteTileTagByTileStart(tileStart);
    if (tileTag == 0xFFFF)
        return;
    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse && gSprites[i].usingSheet && tileStart == gSprites[i].sheetTileStart)
            return;
    }
    FreeSpriteTilesByTag(tileTag);
}

void FieldEffectFreePaletteIfUnused(u8 paletteNum)
{
    u8 i;
    u16 paletteTag = GetSpritePaletteTagByPaletteNum(paletteNum);
    if (paletteTag == 0xFFFF)
        return;
    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse && gSprites[i].oam.paletteNum == paletteNum)
            return;
    }
    FreeSpritePaletteByTag(paletteTag);
}

void FieldEffectActiveListClear(void)
{
    u8 i;
    for (i = 0; i < FIELD_EFFECT_COUNT; i++)
    {
        sFieldEffectActiveList[i] = 0xFF;
    }
}

void FieldEffectActiveListAdd(u8 fldeff)
{
    u8 i;
    for (i = 0; i < FIELD_EFFECT_COUNT; i++)
    {
        if (sFieldEffectActiveList[i] == 0xFF)
        {
            sFieldEffectActiveList[i] = fldeff;
            return;
        }
    }
}

void FieldEffectActiveListRemove(u8 fldeff)
{
    u8 i;
    for (i = 0; i < FIELD_EFFECT_COUNT; i++)
    {
        if (sFieldEffectActiveList[i] == fldeff)
        {
            sFieldEffectActiveList[i] = 0xFF;
            return;
        }
    }
}

bool8 FieldEffectActiveListContains(u8 fldeff)
{
    u8 i;
    for (i = 0; i < FIELD_EFFECT_COUNT; i++)
    {
        if (sFieldEffectActiveList[i] == fldeff)
        {
            return TRUE;
        }
    }
    return FALSE;
}

u8 CreateTrainerSprite(u8 trainerSpriteID, s16 x, s16 y, u8 subpriority, u8 *buffer)
{
    struct SpriteTemplate spriteTemplate;
    LoadCompressedSpritePaletteOverrideBuffer(&gTrainerFrontPicPaletteTable[trainerSpriteID], buffer);
    LoadCompressedSpriteSheetOverrideBuffer(&gTrainerFrontPicTable[trainerSpriteID], buffer);
    spriteTemplate.tileTag = gTrainerFrontPicTable[trainerSpriteID].tag;
    spriteTemplate.paletteTag = gTrainerFrontPicPaletteTable[trainerSpriteID].tag;
    spriteTemplate.oam = &gNewGameOakOamAttributes;
    spriteTemplate.anims = gDummySpriteAnimTable;
    spriteTemplate.images = NULL;
    spriteTemplate.affineAnims = gDummySpriteAffineAnimTable;
    spriteTemplate.callback = SpriteCallbackDummy;
    return CreateSprite(&spriteTemplate, x, y, subpriority);
}

void LoadTrainerGfx_TrainerCard(u8 gender, u16 palOffset, u8 *dest)
{
    LZDecompressVram(gTrainerFrontPicTable[gender].data, dest);
    LoadCompressedPalette(gTrainerFrontPicPaletteTable[gender].data, palOffset, 0x20);
}

u8 AddNewGameBirchObject(s16 x, s16 y, u8 subpriority)
{
    LoadSpritePalette(&gNewGameOakObjectPaletteInfo);
    return CreateSprite(&gNewGameOakObjectTemplate, x, y, subpriority);
}

u8 CreateMonSprite_PicBox(u16 species, s16 x, s16 y, u8 subpriority)
{
    u16 spriteId = CreateMonPicSprite_HandleDeoxys(species, 0, 0x8000, TRUE, x, y, 0, gMonPaletteTable[species].tag);
    PreservePaletteInWeather(IndexOfSpritePaletteTag(gMonPaletteTable[species].tag) + 0x10);
    if (spriteId == 0xFFFF)
        return MAX_SPRITES;
    else
        return spriteId;
}

u8 CreateMonSprite_FieldMove(u16 species, u32 otId, u32 personality, s16 x, s16 y, u8 subpriority)
{
    const struct CompressedSpritePalette * spritePalette = GetMonSpritePalStructFromOtIdPersonality(species, otId, personality);
    u16 spriteId = CreateMonPicSprite_HandleDeoxys(species, otId, personality, 1, x, y, 0, spritePalette->tag);
    PreservePaletteInWeather(IndexOfSpritePaletteTag(spritePalette->tag) + 0x10);
    if (spriteId == 0xFFFF)
        return MAX_SPRITES;
    else
        return spriteId;
}

void FreeResourcesAndDestroySprite(struct Sprite * sprite, u8 spriteId)
{
    ResetPreservedPalettesInWeather();
    if (sprite->oam.affineMode != ST_OAM_AFFINE_OFF)
    {
        FreeOamMatrix(sprite->oam.matrixNum);
    }
    FreeAndDestroyMonPicSprite(spriteId);
}

// r, g, b are between 0 and 16
void MultiplyInvertedPaletteRGBComponents(u16 i, u8 r, u8 g, u8 b)
{
    int curRed;
    int curGreen;
    int curBlue;
    u16 outPal;

    outPal = gPlttBufferUnfaded[i];
    curRed = outPal & 0x1f;
    curGreen = (outPal & (0x1f << 5)) >> 5;
    curBlue = (outPal & (0x1f << 10)) >> 10;
    curRed += (((0x1f - curRed) * r) >> 4);
    curGreen += (((0x1f - curGreen) * g) >> 4);
    curBlue += (((0x1f - curBlue) * b) >> 4);
    outPal = curRed;
    outPal |= curGreen << 5;
    outPal |= curBlue << 10;
    gPlttBufferFaded[i] = outPal;
}

// r, g, b are between 0 and 16
void MultiplyPaletteRGBComponents(u16 i, u8 r, u8 g, u8 b)
{
    int curRed;
    int curGreen;
    int curBlue;
    u16 outPal;

    outPal = gPlttBufferUnfaded[i];
    curRed = outPal & 0x1f;
    curGreen = (outPal & (0x1f << 5)) >> 5;
    curBlue = (outPal & (0x1f << 10)) >> 10;
    curRed -= ((curRed * r) >> 4);
    curGreen -= ((curGreen * g) >> 4);
    curBlue -= ((curBlue * b) >> 4);
    outPal = curRed;
    outPal |= curGreen << 5;
    outPal |= curBlue << 10;
    gPlttBufferFaded[i] = outPal;
}

void PokecenterHealEffect_0(struct Task * task);
void PokecenterHealEffect_1(struct Task * task);
void PokecenterHealEffect_2(struct Task * task);
void PokecenterHealEffect_3(struct Task * task);
void HallOfFameRecordEffect_0(struct Task * task);
void HallOfFameRecordEffect_1(struct Task * task);
void HallOfFameRecordEffect_2(struct Task * task);
void HallOfFameRecordEffect_3(struct Task * task);
void Task_HallOfFameRecord(u8 taskId);
u8 CreatePokeballGlowSprite(s16 duration, s16 x, s16 y, bool16 fanfare);
void SpriteCB_PokeballGlowEffect(struct Sprite * sprite);
void PokeballGlowEffect_0(struct Sprite * sprite);
void PokeballGlowEffect_1(struct Sprite * sprite);
void PokeballGlowEffect_2(struct Sprite * sprite);
void PokeballGlowEffect_3(struct Sprite * sprite);
void PokeballGlowEffect_4(struct Sprite * sprite);
void PokeballGlowEffect_5(struct Sprite * sprite);
void PokeballGlowEffect_6(struct Sprite * sprite);
void PokeballGlowEffect_7(struct Sprite * sprite);
u8 PokecenterHealEffectHelper(s32 x, s32 y);
void HallOfFameRecordEffectHelper(s32 x, s32 y);

void (*const sPokecenterHealTaskCBTable[])(struct Task * ) = {
    PokecenterHealEffect_0,
    PokecenterHealEffect_1,
    PokecenterHealEffect_2,
    PokecenterHealEffect_3
};

void (*const sHallOfFameRecordTaskCBTable[])(struct Task * ) = {
    HallOfFameRecordEffect_0,
    HallOfFameRecordEffect_1,
    HallOfFameRecordEffect_2,
    HallOfFameRecordEffect_3
};

void (*const sPokeballGlowSpriteCBTable[])(struct Sprite * ) = {
    PokeballGlowEffect_0,
    PokeballGlowEffect_1,
    PokeballGlowEffect_2,
    PokeballGlowEffect_3,
    PokeballGlowEffect_4,
    PokeballGlowEffect_5,
    PokeballGlowEffect_6,
    PokeballGlowEffect_7
};

bool8 FldEff_PokecenterHeal(void)
{
    u8 nPokemon;
    struct Task * task;

    nPokemon = CalculatePlayerPartyCount();
    task = &gTasks[CreateTask(Task_PokecenterHeal, 0xff)];
    task->data[1] = nPokemon;
    task->data[2] = 0x5d;
    task->data[3] = 0x24;
    task->data[4] = 0x80;
    task->data[5] = 0x18;
    return FALSE;
}

void Task_PokecenterHeal(u8 taskId)
{
    struct Task * task = &gTasks[taskId];
    sPokecenterHealTaskCBTable[task->data[0]](task);
}

void PokecenterHealEffect_0(struct Task * task)
{
    task->data[0]++;
    task->data[6] = CreatePokeballGlowSprite(task->data[1], task->data[2], task->data[3], TRUE);
    task->data[7] = PokecenterHealEffectHelper(task->data[4], task->data[5]);
}

void PokecenterHealEffect_1(struct Task * task)
{
    if (gSprites[task->data[6]].data[0] > 1)
    {
        gSprites[task->data[7]].data[0]++;
        task->data[0]++;
    }
}

void PokecenterHealEffect_2(struct Task * task)
{
    if (gSprites[task->data[6]].data[0] > 4)
    {
        task->data[0]++;
    }
}

void PokecenterHealEffect_3(struct Task * task)
{
    if (gSprites[task->data[6]].data[0] > 6)
    {
        DestroySprite(&gSprites[task->data[6]]);
        FieldEffectActiveListRemove(FLDEFF_POKECENTER_HEAL);
        DestroyTask(FindTaskIdByFunc(Task_PokecenterHeal));
    }
}


bool8 FldEff_HallOfFameRecord(void)
{
    u8 nPokemon;
    struct Task * task;

    nPokemon = CalculatePlayerPartyCount();
    task = &gTasks[CreateTask(Task_HallOfFameRecord, 0xff)];
    task->data[1] = nPokemon;
    task->data[2] = 0x75;
    task->data[3] = 0x3C;
    return FALSE;
}

void Task_HallOfFameRecord(u8 taskId)
{
    struct Task * task;
    task = &gTasks[taskId];
    sHallOfFameRecordTaskCBTable[task->data[0]](task);
}

void HallOfFameRecordEffect_0(struct Task * task)
{
    u8 taskId;
    task->data[0]++;
    task->data[6] = CreatePokeballGlowSprite(task->data[1], task->data[2], task->data[3], FALSE);
}

void HallOfFameRecordEffect_1(struct Task * task)
{
    if (gSprites[task->data[6]].data[0] > 1)
    {
        HallOfFameRecordEffectHelper(0x78, 0x19);
        task->data[15]++; // was this ever initialized? is this ever used?
        task->data[0]++;
    }
}

void HallOfFameRecordEffect_2(struct Task * task)
{
    if (gSprites[task->data[6]].data[0] > 4)
    {
        task->data[0]++;
    }
}

void HallOfFameRecordEffect_3(struct Task * task)
{
    if (gSprites[task->data[6]].data[0] > 6)
    {
        DestroySprite(&gSprites[task->data[6]]);
        FieldEffectActiveListRemove(FLDEFF_HALL_OF_FAME_RECORD);
        DestroyTask(FindTaskIdByFunc(Task_HallOfFameRecord));
    }
}

u8 CreatePokeballGlowSprite(s16 duration, s16 x, s16 y, bool16 fanfare)
{
    u8 spriteId;
    struct Sprite * sprite;
    spriteId = CreateInvisibleSprite(SpriteCB_PokeballGlowEffect);
    sprite = &gSprites[spriteId];
    sprite->pos2.x = x;
    sprite->pos2.y = y;
    sprite->subpriority = 0xFF;
    sprite->data[5] = fanfare;
    sprite->data[6] = duration;
    sprite->data[7] = spriteId;
    return spriteId;
}

void SpriteCB_PokeballGlowEffect(struct Sprite * sprite)
{
    sPokeballGlowSpriteCBTable[sprite->data[0]](sprite);
}

const struct Coords16 gUnknown_83CC010[] = {
    {0, 0},
    {6, 0},
    {0, 4},
    {6, 4},
    {0, 8},
    {6, 8}
};

const u8 gUnknown_83CC028[] = {16, 12,  8,  0};
const u8 gUnknown_83CC02C[] = {16, 12,  8,  0};
const u8 gUnknown_83CC030[] = { 0,  0,  0,  0};

void PokeballGlowEffect_0(struct Sprite * sprite)
{
    u8 endSpriteId;
    if (sprite->data[1] == 0 || (--sprite->data[1]) == 0)
    {
        sprite->data[1] = 25;
        endSpriteId = CreateSpriteAtEnd(&gUnknown_83CBF88, gUnknown_83CC010[sprite->data[2]].x + sprite->pos2.x, gUnknown_83CC010[sprite->data[2]].y + sprite->pos2.y, 0xFF);
        gSprites[endSpriteId].oam.priority = 2;
        gSprites[endSpriteId].data[0] = sprite->data[7];
        sprite->data[2]++;
        sprite->data[6]--;
        PlaySE(SE_BOWA);
    }
    if (sprite->data[6] == 0)
    {
        sprite->data[1] = 32;
        sprite->data[0]++;
    }
}

void PokeballGlowEffect_1(struct Sprite * sprite)
{
    if ((--sprite->data[1]) == 0)
    {
        sprite->data[0]++;
        sprite->data[1] = 8;
        sprite->data[2] = 0;
        sprite->data[3] = 0;
        if (sprite->data[5])
        {
            PlayFanfare(MUS_ME_ASA);
        }
    }
}

void PokeballGlowEffect_2(struct Sprite * sprite)
{
    u8 phase;
    if ((--sprite->data[1]) == 0)
    {
        sprite->data[1] = 8;
        sprite->data[2]++;
        sprite->data[2] &= 3;
        if (sprite->data[2] == 0)
        {
            sprite->data[3]++;
        }
    }
    phase = (sprite->data[2] + 3) & 3;
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x108, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    phase = (sprite->data[2] + 2) & 3;
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x106, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    phase = (sprite->data[2] + 1) & 3;
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x102, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    phase = sprite->data[2];
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x105, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x103, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    if (sprite->data[3] > 2)
    {
        sprite->data[0]++;
        sprite->data[1] = 8;
        sprite->data[2] = 0;
    }
}

void PokeballGlowEffect_3(struct Sprite * sprite)
{
    u8 phase;
    if ((--sprite->data[1]) == 0)
    {
        sprite->data[1] = 8;
        sprite->data[2]++;
        sprite->data[2] &= 3;
        if (sprite->data[2] == 3)
        {
            sprite->data[0]++;
            sprite->data[1] = 30;
        }
    }
    phase = sprite->data[2];
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x108, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x106, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x102, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x105, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
    MultiplyInvertedPaletteRGBComponents((IndexOfSpritePaletteTag(0x1007) << 4) + 0x103, gUnknown_83CC028[phase], gUnknown_83CC02C[phase], gUnknown_83CC030[phase]);
}

void PokeballGlowEffect_4(struct Sprite * sprite)
{
    if ((--sprite->data[1]) == 0)
    {
        sprite->data[0]++;
    }
}

void PokeballGlowEffect_5(struct Sprite * sprite)
{
    sprite->data[0]++;
}

void PokeballGlowEffect_6(struct Sprite * sprite)
{
    if (sprite->data[5] == 0 || IsFanfareTaskInactive())
    {
        sprite->data[0]++;
    }
}

void PokeballGlowEffect_7(struct Sprite * sprite)
{
}

void SpriteCB_PokeballGlow(struct Sprite * sprite)
{
    if (gSprites[sprite->data[0]].data[0] > 4)
        FieldEffectFreeGraphicsResources(sprite);
}

u8 PokecenterHealEffectHelper(s32 x, s32 y)
{
    u8 spriteId;
    struct Sprite * sprite;
    spriteId = CreateSpriteAtEnd(&gUnknown_83CBFA0, x, y, 0);
    sprite = &gSprites[spriteId];
    sprite->oam.priority = 2;
    sprite->invisible = TRUE;
    return spriteId;
}

void SpriteCB_PokecenterMonitor(struct Sprite * sprite)
{
    if (sprite->data[0] != 0)
    {
        sprite->data[0] = 0;
        sprite->invisible = FALSE;
        StartSpriteAnim(sprite, 1);
    }
    if (sprite->animEnded)
        FieldEffectFreeGraphicsResources(sprite);
}

void HallOfFameRecordEffectHelper(s32 x, s32 y)
{
    CreateSpriteAtEnd(&gUnknown_83CBFB8, x, y, 0);
}

void SpriteCB_HallOfFameMonitor(struct Sprite * sprite)
{
    if (sprite->animEnded)
        FieldEffectFreeGraphicsResources(sprite);
}

void FieldCallback_Fly(void);
void Task_FlyOut(u8 taskId);
void FieldCallback_FlyArrive(void);
void Task_FlyIn(u8 taskId);

void ReturnToFieldFromFlyMapSelect(void)
{
    SetMainCallback2(CB2_ReturnToField);
    gFieldCallback = FieldCallback_Fly;
}

void FieldCallback_Fly(void)
{
    FadeInFromBlack();
    CreateTask(Task_FlyOut, 0);
    ScriptContext2_Enable();
    FreezeObjectEvents();
    gFieldCallback = NULL;
}

void Task_FlyOut(u8 taskId)
{
    struct Task * task;
    task = &gTasks[taskId];
    if (task->data[0] == 0)
    {
        if (!IsWeatherNotFadingIn())
            return;
        gFieldEffectArguments[0] = GetCursorSelectionMonId();
        if ((int)gFieldEffectArguments[0] >= PARTY_SIZE)
            gFieldEffectArguments[0] = 0;
        FieldEffectStart(FLDEFF_USE_FLY);
        task->data[0]++;
    }
    if (!FieldEffectActiveListContains(FLDEFF_USE_FLY))
    {
        Overworld_ResetStateAfterFly();
        WarpIntoMap();
        SetMainCallback2(CB2_LoadMap);
        gFieldCallback = FieldCallback_FlyArrive;
        DestroyTask(taskId);
    }
}

void FieldCallback_FlyArrive(void)
{
    Overworld_PlaySpecialMapMusic();
    FadeInFromBlack();
    CreateTask(Task_FlyIn, 0);
    gObjectEvents[gPlayerAvatar.objectEventId].invisible = TRUE;
    if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING)
    {
        ObjectEventTurn(&gObjectEvents[gPlayerAvatar.objectEventId], DIR_WEST);
    }
    ScriptContext2_Enable();
    FreezeObjectEvents();
    gFieldCallback = NULL;
}

void Task_FlyIn(u8 taskId)
{
    struct Task * task;
    task = &gTasks[taskId];
    if (task->data[0] == 0)
    {
        if (gPaletteFade.active)
        {
            return;
        }
        FieldEffectStart(FLDEFF_FLY_IN);
        task->data[0]++;
    }
    if (!FieldEffectActiveListContains(FLDEFF_FLY_IN))
    {
        ScriptContext2_Disable();
        UnfreezeObjectEvents();
        DestroyTask(taskId);
    }
}

void Task_FallWarpFieldEffect(u8 taskId);
bool8 FallWarpEffect_1(struct Task * task);
bool8 FallWarpEffect_2(struct Task * task);
bool8 FallWarpEffect_3(struct Task * task);
bool8 FallWarpEffect_4(struct Task * task);
bool8 FallWarpEffect_5(struct Task * task);
bool8 FallWarpEffect_6(struct Task * task);
bool8 FallWarpEffect_7(struct Task * task);

bool8 (*const sFallWarpEffectCBPtrs[])(struct Task * task) = {
    FallWarpEffect_1,
    FallWarpEffect_2,
    FallWarpEffect_3,
    FallWarpEffect_4,
    FallWarpEffect_5,
    FallWarpEffect_6,
    FallWarpEffect_7
};

void FieldCB_FallWarpExit(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    sub_8111CF0();
    ScriptContext2_Enable();
    FreezeObjectEvents();
    CreateTask(Task_FallWarpFieldEffect, 0);
    gFieldCallback = NULL;
}

void Task_FallWarpFieldEffect(u8 taskId)
{
    struct Task * task = &gTasks[taskId];
    while (sFallWarpEffectCBPtrs[task->data[0]](task))
        ;
}

bool8 FallWarpEffect_1(struct Task * task)
{
    struct ObjectEvent * playerObject;
    struct Sprite * playerSprite;
    playerObject = &gObjectEvents[gPlayerAvatar.objectEventId];
    playerSprite = &gSprites[gPlayerAvatar.spriteId];
    CameraObjectReset2();
    gObjectEvents[gPlayerAvatar.objectEventId].invisible = TRUE;
    gPlayerAvatar.preventStep = TRUE;
    ObjectEventSetHeldMovement(playerObject, GetFaceDirectionMovementAction(GetPlayerFacingDirection()));
    task->data[4] = playerSprite->subspriteMode;
    playerObject->fixedPriority = 1;
    playerSprite->oam.priority = 1;
    playerSprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
    task->data[0]++;
    return TRUE;
}

bool8 FallWarpEffect_2(struct Task * task)
{
    if (IsWeatherNotFadingIn())
    {
        task->data[0]++;
    }
    return FALSE;
}

bool8 FallWarpEffect_3(struct Task * task)
{
    struct Sprite * sprite;
    s16 centerToCornerVecY;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    centerToCornerVecY = -(sprite->centerToCornerVecY << 1);
    sprite->pos2.y = -(sprite->pos1.y + sprite->centerToCornerVecY + gSpriteCoordOffsetY + centerToCornerVecY);
    task->data[1] = 1;
    task->data[2] = 0;
    gObjectEvents[gPlayerAvatar.objectEventId].invisible = FALSE;
    PlaySE(SE_RU_HYUU);
    task->data[0]++;
    return FALSE;
}

bool8 FallWarpEffect_4(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    struct Sprite * sprite;

    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.y += task->data[1];
    if (task->data[1] < 8)
    {
        task->data[2] += task->data[1];
        if (task->data[2] & 0xf)
        {
            task->data[1] <<= 1;
        }
    }
    if (task->data[3] == 0 && sprite->pos2.y >= -16)
    {
        task->data[3]++;
        objectEvent->fixedPriority = 0;
        sprite->subspriteMode = task->data[4];
        objectEvent->triggerGroundEffectsOnMove = 1;
    }
    if (sprite->pos2.y >= 0)
    {
        PlaySE(SE_W070);
        objectEvent->triggerGroundEffectsOnStop = 1;
        objectEvent->landingJump = 1;
        sprite->pos2.y = 0;
        task->data[0]++;
    }
    return FALSE;
}

bool8 FallWarpEffect_5(struct Task * task)
{
    task->data[0]++;
    task->data[1] = 4;
    task->data[2] = 0;
    SetCameraPanningCallback(NULL);
    return TRUE;
}

bool8 FallWarpEffect_6(struct Task * task)
{
    SetCameraPanning(0, task->data[1]);
    task->data[1] = -task->data[1];
    task->data[2]++;
    if ((task->data[2] & 3) == 0)
    {
        task->data[1] >>= 1;
    }
    if (task->data[1] == 0)
    {
        task->data[0]++;
    }
    return FALSE;
}

bool8 FallWarpEffect_7(struct Task * task)
{
    s16 x, y;
    gPlayerAvatar.preventStep = FALSE;
    ScriptContext2_Disable();
    CameraObjectReset1();
    UnfreezeObjectEvents();
    InstallCameraPanAheadCallback();
    PlayerGetDestCoords(&x, &y);
    // Seafoam Islands
    if (sub_8055B38(MapGridGetMetatileBehaviorAt(x, y)) == TRUE)
    {
        VarSet(VAR_TEMP_1, 1);
        SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_SURFING);
        HelpSystem_SetSomeVariable2(22);
    }
    DestroyTask(FindTaskIdByFunc(Task_FallWarpFieldEffect));
    return FALSE;
}

void Task_EscalatorWarpFieldEffect(u8 taskId);
bool8 EscalatorWarpEffect_1(struct Task * task);
bool8 EscalatorWarpEffect_2(struct Task * task);
bool8 EscalatorWarpEffect_3(struct Task * task);
bool8 EscalatorWarpEffect_4(struct Task * task);
bool8 EscalatorWarpEffect_5(struct Task * task);
bool8 EscalatorWarpEffect_6(struct Task * task);
void Escalator_AnimatePlayerGoingDown(struct Task * task);
void Escalator_AnimatePlayerGoingUp(struct Task * task);
void Escalator_BeginFadeOutToNewMap(void);
void Escalator_TransitionToWarpInEffect(void);
void FieldCB_EscalatorWarpIn(void);
void Task_EscalatorWarpInFieldEffect(u8 taskId);
bool8 EscalatorWarpInEffect_1(struct Task * task);
bool8 EscalatorWarpInEffect_2(struct Task * task);
bool8 EscalatorWarpInEffect_3(struct Task * task);
bool8 EscalatorWarpInEffect_4(struct Task * task);
bool8 EscalatorWarpInEffect_5(struct Task * task);
bool8 EscalatorWarpInEffect_6(struct Task * task);
bool8 EscalatorWarpInEffect_7(struct Task * task);

bool8 (*const sEscalatorWarpFieldEffectFuncs[])(struct Task * task) = {
    EscalatorWarpEffect_1,
    EscalatorWarpEffect_2,
    EscalatorWarpEffect_3,
    EscalatorWarpEffect_4,
    EscalatorWarpEffect_5,
    EscalatorWarpEffect_6
};

void StartEscalatorWarp(u8 metatileBehavior, u8 priority)
{
    u8 taskId = CreateTask(Task_EscalatorWarpFieldEffect, priority);
    gTasks[taskId].data[1] = 0;
    if (metatileBehavior == MB_UP_ESCALATOR)
        gTasks[taskId].data[1] = 1;
}

void Task_EscalatorWarpFieldEffect(u8 taskId)
{
    struct Task * task = &gTasks[taskId];
    while (sEscalatorWarpFieldEffectFuncs[task->data[0]](task))
        ;
}

bool8 EscalatorWarpEffect_1(struct Task * task)
{
    FreezeObjectEvents();
    CameraObjectReset2();
    StartEscalator(task->data[1]);
    sub_81128BC(1);
    task->data[0]++;
    return FALSE;
}

bool8 EscalatorWarpEffect_2(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(objectEvent) || ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        ObjectEventSetHeldMovement(objectEvent, GetFaceDirectionMovementAction(GetPlayerFacingDirection()));
        task->data[0]++;
        task->data[2] = 0;
        task->data[3] = 0;
        if ((u8)task->data[1] == 0)
        {
            task->data[0] = 4;
        }
        PlaySE(SE_ESUKA);
    }
    return FALSE;
}

bool8 EscalatorWarpEffect_3(struct Task * task)
{
    Escalator_AnimatePlayerGoingDown(task);
    if (task->data[2] > 3)
    {
        Escalator_BeginFadeOutToNewMap();
        task->data[0]++;
    }
    return FALSE;
}

bool8 EscalatorWarpEffect_4(struct Task * task)
{
    Escalator_AnimatePlayerGoingDown(task);
    Escalator_TransitionToWarpInEffect();
    return FALSE;
}

bool8 EscalatorWarpEffect_5(struct Task * task)
{
    Escalator_AnimatePlayerGoingUp(task);
    if (task->data[2] > 3)
    {
        Escalator_BeginFadeOutToNewMap();
        task->data[0]++;
    }
    return FALSE;
}

bool8 EscalatorWarpEffect_6(struct Task * task)
{
    Escalator_AnimatePlayerGoingUp(task);
    Escalator_TransitionToWarpInEffect();
    return FALSE;
}


void Escalator_AnimatePlayerGoingDown(struct Task * task)
{
    struct Sprite * sprite;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.x = Cos(0x84, task->data[2]);
    sprite->pos2.y = Sin(0x94, task->data[2]);
    task->data[3]++;
    if (task->data[3] & 1)
    {
        task->data[2]++;
    }
}

void Escalator_AnimatePlayerGoingUp(struct Task * task)
{
    struct Sprite * sprite;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.x = Cos(0x7c, task->data[2]);
    sprite->pos2.y = Sin(0x76, task->data[2]);
    task->data[3]++;
    if (task->data[3] & 1)
    {
        task->data[2]++;
    }
}

void Escalator_BeginFadeOutToNewMap(void)
{
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
}

void Escalator_TransitionToWarpInEffect(void)
{
    if (!gPaletteFade.active && BGMusicStopped() == TRUE)
    {
        StopEscalator();
        WarpIntoMap();
        gFieldCallback = FieldCB_EscalatorWarpIn;
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(FindTaskIdByFunc(Task_EscalatorWarpFieldEffect));
    }
}

bool8 (*const sEscalatorWarpInFieldEffectFuncs[])(struct Task * task) = {
    EscalatorWarpInEffect_1,
    EscalatorWarpInEffect_2,
    EscalatorWarpInEffect_3,
    EscalatorWarpInEffect_4,
    EscalatorWarpInEffect_5,
    EscalatorWarpInEffect_6,
    EscalatorWarpInEffect_7
};

void FieldCB_EscalatorWarpIn(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    sub_8111CF0();
    ScriptContext2_Enable();
    FreezeObjectEvents();
    CreateTask(Task_EscalatorWarpInFieldEffect, 0);
    gFieldCallback = NULL;
}

void Task_EscalatorWarpInFieldEffect(u8 taskId)
{
    struct Task * task = &gTasks[taskId];
    while (sEscalatorWarpInFieldEffectFuncs[task->data[0]](task))
        ;
}

bool8 EscalatorWarpInEffect_1(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    s16 x;
    s16 y;
    u8 behavior;
    CameraObjectReset2();
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    ObjectEventSetHeldMovement(objectEvent, GetFaceDirectionMovementAction(DIR_EAST));
    PlayerGetDestCoords(&x, &y);
    behavior = MapGridGetMetatileBehaviorAt(x, y);
    task->data[0]++;
    task->data[1] = 16;
    if (behavior == MB_DOWN_ESCALATOR)
    {
        behavior = 1;
        task->data[0] = 3;
    } else
    {
        behavior = 0;
    }
    StartEscalator(behavior);
    return TRUE;
}

bool8 EscalatorWarpInEffect_2(struct Task * task)
{
    struct Sprite * sprite;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.x = Cos(0x84, task->data[1]);
    sprite->pos2.y = Sin(0x94, task->data[1]);
    task->data[0]++;
    return FALSE;
}

bool8 EscalatorWarpInEffect_3(struct Task * task)
{
    struct Sprite * sprite;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.x = Cos(0x84, task->data[1]);
    sprite->pos2.y = Sin(0x94, task->data[1]);
    task->data[2]++;
    if (task->data[2] & 1)
    {
        task->data[1]--;
    }
    if (task->data[1] == 0)
    {
        sprite->pos2.x = 0;
        sprite->pos2.y = 0;
        task->data[0] = 5;
    }
    return FALSE;
}


bool8 EscalatorWarpInEffect_4(struct Task * task)
{
    struct Sprite * sprite;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.x = Cos(0x7c, task->data[1]);
    sprite->pos2.y = Sin(0x76, task->data[1]);
    task->data[0]++;
    return FALSE;
}

bool8 EscalatorWarpInEffect_5(struct Task * task)
{
    struct Sprite * sprite;
    sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.x = Cos(0x7c, task->data[1]);
    sprite->pos2.y = Sin(0x76, task->data[1]);
    task->data[2]++;
    if (task->data[2] & 1)
    {
        task->data[1]--;
    }
    if (task->data[1] == 0)
    {
        sprite->pos2.x = 0;
        sprite->pos2.y = 0;
        task->data[0]++;
    }
    return FALSE;
}

bool8 EscalatorWarpInEffect_6(struct Task * task)
{
    if (IsEscalatorMoving())
    {
        return FALSE;
    }
    StopEscalator();
    task->data[0]++;
    return TRUE;
}

bool8 EscalatorWarpInEffect_7(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        CameraObjectReset1();
        ScriptContext2_Disable();
        UnfreezeObjectEvents();
        ObjectEventSetHeldMovement(objectEvent, GetWalkNormalMovementAction(DIR_EAST));
        DestroyTask(FindTaskIdByFunc(Task_EscalatorWarpInFieldEffect));
        sub_81128BC(2);
    }
    return FALSE;
}

void Task_UseWaterfall(u8 taskId);

bool8 waterfall_0_setup(struct Task * task, struct ObjectEvent * playerObj);
bool8 waterfall_1_do_anim_probably(struct Task * task, struct ObjectEvent * playerObj);
bool8 waterfall_2_wait_anim_finish_probably(struct Task * task, struct ObjectEvent * playerObj);
bool8 waterfall_3_move_player_probably(struct Task * task, struct ObjectEvent * playerObj);
bool8 waterfall_4_wait_player_move_probably(struct Task * task, struct ObjectEvent * playerObj);

bool8 (*const sUseWaterfallFieldEffectFuncs[])(struct Task * task, struct ObjectEvent * playerObj) = {
    waterfall_0_setup,
    waterfall_1_do_anim_probably,
    waterfall_2_wait_anim_finish_probably,
    waterfall_3_move_player_probably,
    waterfall_4_wait_player_move_probably
};

u32 FldEff_UseWaterfall(void)
{
    u8 taskId = CreateTask(Task_UseWaterfall, 0xFF);
    gTasks[taskId].data[1] = gFieldEffectArguments[0];
    Task_UseWaterfall(taskId);
    return 0;
}

void Task_UseWaterfall(u8 taskId)
{
    while (sUseWaterfallFieldEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId], &gObjectEvents[gPlayerAvatar.objectEventId]))
        ;
}

bool8 waterfall_0_setup(struct Task * task, struct ObjectEvent * playerObj)
{
    ScriptContext2_Enable();
    gPlayerAvatar.preventStep = TRUE;
    task->data[0]++;
    return FALSE;
}

bool8 waterfall_1_do_anim_probably(struct Task * task, struct ObjectEvent * playerObj)
{
    ScriptContext2_Enable();
    if (!ObjectEventIsMovementOverridden(playerObj))
    {
        ObjectEventClearHeldMovementIfFinished(playerObj);
        gFieldEffectArguments[0] = task->data[1];
        FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
        task->data[0]++;
    }
    return FALSE;
}

bool8 waterfall_2_wait_anim_finish_probably(struct Task * task, struct ObjectEvent * playerObj)
{
    if (FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON))
        return FALSE;
    task->data[0]++;
    return TRUE;
}

bool8 waterfall_3_move_player_probably(struct Task * task, struct ObjectEvent * playerObj)
{
    ObjectEventSetHeldMovement(playerObj, sub_8063F2C(DIR_NORTH));
    task->data[0]++;
    return FALSE;
}

bool8 waterfall_4_wait_player_move_probably(struct Task * task, struct ObjectEvent * playerObj)
{
    if (!ObjectEventClearHeldMovementIfFinished(playerObj))
        return FALSE;
    if (MetatileBehavior_IsWaterfall(playerObj->mapobj_unk_1E))
    {
        task->data[0] = 3;
        return TRUE;
    }
    ScriptContext2_Disable();
    gPlayerAvatar.preventStep = FALSE;
    DestroyTask(FindTaskIdByFunc(Task_UseWaterfall));
    FieldEffectActiveListRemove(FLDEFF_USE_WATERFALL);
    return FALSE;
}

void Task_Dive(u8 taskId);
bool8 dive_1_lock(struct Task * task);
bool8 dive_2_unknown(struct Task * task);
bool8 dive_3_unknown(struct Task * task);

bool8 (*const sDiveFieldEffectFuncPtrs[])(struct Task * task) = {
    dive_1_lock,
    dive_2_unknown,
    dive_3_unknown
};

u32 FldEff_UseDive(void)
{
    u8 taskId = CreateTask(Task_Dive, 0xFF);
    gTasks[taskId].data[15] = gFieldEffectArguments[0];
    gTasks[taskId].data[14] = gFieldEffectArguments[1];
    Task_Dive(taskId);
    return 0;
}

void Task_Dive(u8 taskId)
{
    while (sDiveFieldEffectFuncPtrs[gTasks[taskId].data[0]](&gTasks[taskId]))
        ;
}

bool8 dive_1_lock(struct Task * task)
{
    gPlayerAvatar.preventStep = TRUE;
    task->data[0]++;
    return FALSE;
}

bool8 dive_2_unknown(struct Task * task)
{
    ScriptContext2_Enable();
    gFieldEffectArguments[0] = task->data[15];
    FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    task->data[0]++;
    return FALSE;
}

bool8 dive_3_unknown(struct Task * task)
{
    struct MapPosition pos;
    PlayerGetDestCoords(&pos.x, &pos.y);
    if (!FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON))
    {
        dive_warp(&pos, gObjectEvents[gPlayerAvatar.objectEventId].mapobj_unk_1E);
        DestroyTask(FindTaskIdByFunc(Task_Dive));
        FieldEffectActiveListRemove(FLDEFF_USE_DIVE);
    }
    return FALSE;
}

void Task_LavaridgeGymB1FWarp(u8 taskId);
bool8 LavaridgeGymB1FWarpEffect_1(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpEffect_2(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpEffect_3(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpEffect_4(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpEffect_5(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpEffect_6(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
void FieldCB_LavaridgeGymB1FWarpExit(void);
void Task_LavaridgeGymB1FWarpExit(u8 taskId);
bool8 LavaridgeGymB1FWarpExitEffect_1(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpExitEffect_2(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpExitEffect_3(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGymB1FWarpExitEffect_4(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);

bool8 (*const sLavaridgeGymB1FWarpEffectFuncs[])(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite) = {
    LavaridgeGymB1FWarpEffect_1,
    LavaridgeGymB1FWarpEffect_2,
    LavaridgeGymB1FWarpEffect_3,
    LavaridgeGymB1FWarpEffect_4,
    LavaridgeGymB1FWarpEffect_5,
    LavaridgeGymB1FWarpEffect_6
};

void StartLavaridgeGymB1FWarp(u8 priority)
{
    CreateTask(Task_LavaridgeGymB1FWarp, priority);
}

void Task_LavaridgeGymB1FWarp(u8 taskId)
{
    while (sLavaridgeGymB1FWarpEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId], &gObjectEvents[gPlayerAvatar.objectEventId], &gSprites[gPlayerAvatar.spriteId]));
}

bool8 LavaridgeGymB1FWarpEffect_1(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    FreezeObjectEvents();
    CameraObjectReset2();
    SetCameraPanningCallback(NULL);
    gPlayerAvatar.preventStep = TRUE;
    objectEvent->fixedPriority = 1;
    task->data[1] = 1;
    task->data[0]++;
    return TRUE;
}

bool8 LavaridgeGymB1FWarpEffect_2(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    SetCameraPanning(0, task->data[1]);
    task->data[1] = -task->data[1];
    task->data[2]++;
    if (task->data[2] > 7)
    {
        task->data[2] = 0;
        task->data[0]++;
    }
    return FALSE;
}

bool8 LavaridgeGymB1FWarpEffect_3(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    sprite->pos2.y = 0;
    task->data[3] = 1;
    gFieldEffectArguments[0] = objectEvent->currentCoords.x;
    gFieldEffectArguments[1] = objectEvent->currentCoords.y;
    gFieldEffectArguments[2] = sprite->subpriority - 1;
    gFieldEffectArguments[3] = sprite->oam.priority;
    FieldEffectStart(FLDEFF_LAVARIDGE_GYM_WARP);
    PlaySE(SE_W153);
    task->data[0]++;
    return TRUE;
}

bool8 LavaridgeGymB1FWarpEffect_4(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    s16 centerToCornerVecY;
    SetCameraPanning(0, task->data[1]);
    if (task->data[1] = -task->data[1], ++task->data[2] <= 17)
    {
        if (!(task->data[2] & 1) && (task->data[1] <= 3))
        {
            task->data[1] <<= 1;
        }
    } else if (!(task->data[2] & 4) && (task->data[1] > 0))
    {
        task->data[1] >>= 1;
    }
    if (task->data[2] > 6)
    {
        centerToCornerVecY = -(sprite->centerToCornerVecY << 1);
        if (sprite->pos2.y > -(sprite->pos1.y + sprite->centerToCornerVecY + gSpriteCoordOffsetY + centerToCornerVecY))
        {
            sprite->pos2.y -= task->data[3];
            if (task->data[3] <= 7)
            {
                task->data[3]++;
            }
        } else
        {
            task->data[4] = 1;
        }
    }
    if (task->data[5] == 0 && sprite->pos2.y < -0x10)
    {
        task->data[5]++;
        objectEvent->fixedPriority = 1;
        sprite->oam.priority = 1;
        sprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
    }
    if (task->data[1] == 0 && task->data[4] != 0)
    {
        task->data[0]++;
    }
    return FALSE;
}

bool8 LavaridgeGymB1FWarpEffect_5(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    TryFadeOutOldMapMusic();
    WarpFadeOutScreen();
    task->data[0]++;
    return FALSE;
}

bool8 LavaridgeGymB1FWarpEffect_6(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    if (!gPaletteFade.active && BGMusicStopped() == TRUE)
    {
        WarpIntoMap();
        gFieldCallback = FieldCB_LavaridgeGymB1FWarpExit;
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(FindTaskIdByFunc(Task_LavaridgeGymB1FWarp));
    }
    return FALSE;
}

bool8 (*const sLavaridgeGymB1FWarpExitEffectFuncs[])(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite) = {
    LavaridgeGymB1FWarpExitEffect_1,
    LavaridgeGymB1FWarpExitEffect_2,
    LavaridgeGymB1FWarpExitEffect_3,
    LavaridgeGymB1FWarpExitEffect_4
};

void FieldCB_LavaridgeGymB1FWarpExit(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    sub_8111CF0();
    ScriptContext2_Enable();
    gFieldCallback = NULL;
    CreateTask(Task_LavaridgeGymB1FWarpExit, 0);
}

void Task_LavaridgeGymB1FWarpExit(u8 taskId)
{
    while (sLavaridgeGymB1FWarpExitEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId], &gObjectEvents[gPlayerAvatar.objectEventId], &gSprites[gPlayerAvatar.spriteId]));
}

bool8 LavaridgeGymB1FWarpExitEffect_1(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    CameraObjectReset2();
    FreezeObjectEvents();
    gPlayerAvatar.preventStep = TRUE;
    objectEvent->invisible = TRUE;
    task->data[0]++;
    return FALSE;
}

bool8 LavaridgeGymB1FWarpExitEffect_2(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    if (IsWeatherNotFadingIn())
    {
        gFieldEffectArguments[0] = objectEvent->currentCoords.x;
        gFieldEffectArguments[1] = objectEvent->currentCoords.y;
        gFieldEffectArguments[2] = sprite->subpriority - 1;
        gFieldEffectArguments[3] = sprite->oam.priority;
        task->data[1] = FieldEffectStart(FLDEFF_POP_OUT_OF_ASH);
        task->data[0]++;
    }
    return FALSE;
}

bool8 LavaridgeGymB1FWarpExitEffect_3(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    sprite = &gSprites[task->data[1]];
    if (sprite->animCmdIndex > 1)
    {
        task->data[0]++;
        objectEvent->invisible = FALSE;
        CameraObjectReset1();
        PlaySE(SE_W091);
        ObjectEventSetHeldMovement(objectEvent, sub_8064194(DIR_EAST));
    }
    return FALSE;
}

bool8 LavaridgeGymB1FWarpExitEffect_4(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        gPlayerAvatar.preventStep = FALSE;
        ScriptContext2_Disable();
        UnfreezeObjectEvents();
        DestroyTask(FindTaskIdByFunc(Task_LavaridgeGymB1FWarpExit));
    }
    return FALSE;
}

void Task_LavaridgeGym1FWarp(u8 taskId);
bool8 LavaridgeGym1FWarpEffect_1(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGym1FWarpEffect_2(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGym1FWarpEffect_3(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGym1FWarpEffect_4(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);
bool8 LavaridgeGym1FWarpEffect_5(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite);

bool8 (*const sLavaridgeGym1FWarpEffectFuncs[])(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite) = {
    LavaridgeGym1FWarpEffect_1,
    LavaridgeGym1FWarpEffect_2,
    LavaridgeGym1FWarpEffect_3,
    LavaridgeGym1FWarpEffect_4,
    LavaridgeGym1FWarpEffect_5
};

// For the ash puff effect when warping off the B1F ash tiles
u8 FldEff_LavaridgeGymWarp(void)
{
    u8 spriteId;
    sub_8063BC4((s16 *)&gFieldEffectArguments[0], (s16 *)&gFieldEffectArguments[1], 8, 8);
    spriteId = CreateSpriteAtEnd(gFieldEffectObjectTemplatePointers[33], gFieldEffectArguments[0], gFieldEffectArguments[1], gFieldEffectArguments[2]);
    gSprites[spriteId].oam.priority = gFieldEffectArguments[3];
    gSprites[spriteId].coordOffsetEnabled = 1;
    return spriteId;
}

void SpriteCB_LavaridgeGymWarp(struct Sprite * sprite)
{
    if (sprite->animEnded)
    {
        FieldEffectStop(sprite, FLDEFF_LAVARIDGE_GYM_WARP);
    }
}

void StartLavaridgeGym1FWarp(u8 priority)
{
    CreateTask(Task_LavaridgeGym1FWarp, priority);
}

void Task_LavaridgeGym1FWarp(u8 taskId)
{
    while(sLavaridgeGym1FWarpEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId], &gObjectEvents[gPlayerAvatar.objectEventId], &gSprites[gPlayerAvatar.spriteId]));
}

bool8 LavaridgeGym1FWarpEffect_1(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    FreezeObjectEvents();
    CameraObjectReset2();
    gPlayerAvatar.preventStep = TRUE;
    objectEvent->fixedPriority = 1;
    task->data[0]++;
    return FALSE;
}

bool8 LavaridgeGym1FWarpEffect_2(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        if (task->data[1] > 3)
        {
            gFieldEffectArguments[0] = objectEvent->currentCoords.x;
            gFieldEffectArguments[1] = objectEvent->currentCoords.y;
            gFieldEffectArguments[2] = sprite->subpriority - 1;
            gFieldEffectArguments[3] = sprite->oam.priority;
            task->data[1] = FieldEffectStart(FLDEFF_POP_OUT_OF_ASH);
            task->data[0]++;
        } else
        {
            task->data[1]++;
            ObjectEventSetHeldMovement(objectEvent, GetStepInPlaceDelay4AnimId(objectEvent->facingDirection));
            PlaySE(SE_FU_ZUZUZU);
        }
    }
    return FALSE;
}

bool8 LavaridgeGym1FWarpEffect_3(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    if (gSprites[task->data[1]].animCmdIndex == 2)
    {
        objectEvent->invisible = TRUE;
        task->data[0]++;
    }
    return FALSE;
}

bool8 LavaridgeGym1FWarpEffect_4(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    if (!FieldEffectActiveListContains(FLDEFF_POP_OUT_OF_ASH))
    {
        TryFadeOutOldMapMusic();
        WarpFadeOutScreen();
        task->data[0]++;
    }
    return FALSE;
}

bool8 LavaridgeGym1FWarpEffect_5(struct Task * task, struct ObjectEvent * objectEvent, struct Sprite * sprite)
{
    if (!gPaletteFade.active && BGMusicStopped() == TRUE)
    {
        WarpIntoMap();
        gFieldCallback = FieldCB_FallWarpExit;
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(FindTaskIdByFunc(Task_LavaridgeGym1FWarp));
    }
    return FALSE;
}

u8 FldEff_PopOutOfAsh(void)
{
    u8 spriteId;
    sub_8063BC4((s16 *)&gFieldEffectArguments[0], (s16 *)&gFieldEffectArguments[1], 8, 8);
    spriteId = CreateSpriteAtEnd(gFieldEffectObjectTemplatePointers[32], gFieldEffectArguments[0], gFieldEffectArguments[1], gFieldEffectArguments[2]);
    gSprites[spriteId].oam.priority = gFieldEffectArguments[3];
    gSprites[spriteId].coordOffsetEnabled = 1;
    return spriteId;
}

void SpriteCB_PopOutOfAsh(struct Sprite * sprite)
{
    if (sprite->animEnded)
    {
        FieldEffectStop(sprite, FLDEFF_POP_OUT_OF_ASH);
    }
}

void Task_DoEscapeRopeFieldEffect(u8 taskId);
void EscapeRopeFieldEffect_Step0(struct Task * task);
void EscapeRopeFieldEffect_Step1(struct Task * task);
u8 sub_808576C(struct ObjectEvent * playerObj, s16 *a1p, s16 *a2p);
bool32 sub_80857F0(struct ObjectEvent * playerObj, s16 *a1p, s16 *a2p);
void FieldCallback_EscapeRopeExit(void);
void Task_DoEscapeRopeExitFieldEffect(u8 taskId);
void EscapeRopeExitFieldEffect_Step0(struct Task * task);
void EscapeRopeExitFieldEffect_Step1(struct Task * task);

void (*const gEscapeRopeFieldEffectFuncs[])(struct Task * task) = {
    EscapeRopeFieldEffect_Step0,
    EscapeRopeFieldEffect_Step1
};

void StartEscapeRopeFieldEffect(void)
{
    ScriptContext2_Enable();
    FreezeObjectEvents();
    CreateTask(Task_DoEscapeRopeFieldEffect, 80);
}

void Task_DoEscapeRopeFieldEffect(u8 taskId)
{
    gEscapeRopeFieldEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void EscapeRopeFieldEffect_Step0(struct Task * task)
{
    task->data[0]++;
    task->data[13] = 64;
    task->data[14] = GetPlayerFacingDirection();
    task->data[15] = 0;
}

void EscapeRopeFieldEffect_Step1(struct Task * task)
{
    struct ObjectEvent * playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    s16 *data = task->data;
    sub_808576C(playerObj, &task->data[1], &task->data[2]);
    if (data[3] < 60)
    {
        data[3]++;
        if (data[3] == 20)
        {
            PlaySE(SE_TK_WARPIN);
        }
    }
    else if (data[4] == 0 && !sub_80857F0(playerObj, &task->data[5], &task->data[6]))
    {
        TryFadeOutOldMapMusic();
        WarpFadeOutScreen();
        data[4] = 1;
    }
    if (data[4] == 1 && !gPaletteFade.active && BGMusicStopped() == TRUE)
    {
        ObjectEventSetDirection(playerObj, task->data[15]);
        sub_80555E0();
        WarpIntoMap();
        gFieldCallback = FieldCallback_EscapeRopeExit;
        SetMainCallback2(CB2_LoadMap);
        DestroyTask(FindTaskIdByFunc(Task_DoEscapeRopeFieldEffect));
    }
}

const u8 gUnknown_83CC0E8[] = {
    [DIR_NONE]  = DIR_SOUTH,
    [DIR_SOUTH] = DIR_WEST,
    [DIR_WEST]  = DIR_NORTH,
    [DIR_NORTH] = DIR_EAST,
    [DIR_EAST]  = DIR_SOUTH,
};

u8 sub_808576C(struct ObjectEvent * playerObj, s16 *delay_p, s16 *stage_p)
{
    if (!ObjectEventIsMovementOverridden(playerObj) || ObjectEventClearHeldMovementIfFinished(playerObj))
    {
        if (*delay_p != 0 && --(*delay_p) != 0)
            return playerObj->facingDirection;
        ObjectEventSetHeldMovement(playerObj, GetFaceDirectionMovementAction(gUnknown_83CC0E8[playerObj->facingDirection]));
        if (*stage_p < 12)
            (*stage_p)++;
        *delay_p = 12 >> (*stage_p); // 12 >> 4 = 0
        return gUnknown_83CC0E8[playerObj->facingDirection];
    }
    return playerObj->facingDirection;
}

bool32 sub_80857F0(struct ObjectEvent * playerObj, s16 *state_p, s16 *y_p)
{
    struct Sprite * sprite = &gSprites[playerObj->spriteId];
    switch (*state_p)
    {
    case 0:
        CameraObjectReset2();
        (*state_p)++;
        // fallthrough
    case 1:
        sprite->pos2.y -= 8;
        (*y_p) -= 8;
        if (*y_p <= -16)
        {
            playerObj->fixedPriority = TRUE;
            sprite->oam.priority = 1;
            sprite->subpriority = 0;
            sprite->subspriteMode = SUBSPRITES_OFF;
            (*state_p)++;
        }
        break;
    case 2:
        sprite->pos2.y -= 8;
        (*y_p) -= 8;
        if (*y_p <= -88)
        {
            (*state_p)++;
            return FALSE;
        }
        break;
    case 3:
        return FALSE;
    }
    return TRUE;
}

void (*const sEscapeRopeExitEffectFuncs[])(struct Task * task) = {
    EscapeRopeExitFieldEffect_Step0,
    EscapeRopeExitFieldEffect_Step1
};

bool32 sub_80858A4(struct ObjectEvent * playerObj, s16 *state_p, s16 *y_p, s16 *priority_p, s16 *subpriority_p, s16 *subspriteMode_p)
{
    struct Sprite * sprite = &gSprites[playerObj->spriteId];
    switch (*state_p)
    {
    case 0:
        CameraObjectReset2();
        *y_p = -88;
        sprite->pos2.y -= 88;
        *priority_p = sprite->oam.priority;
        *subpriority_p = sprite->subpriority;
        *subspriteMode_p = sprite->subspriteMode;
        playerObj->fixedPriority = TRUE;
        sprite->oam.priority = 1;
        sprite->subpriority = 0;
        sprite->subspriteMode = SUBSPRITES_OFF;
        (*state_p)++;
        // fallthrough
    case 1:
        sprite->pos2.y += 4;
        (*y_p) += 4;
        if (*y_p >= -16)
        {
            sprite->oam.priority = *priority_p;
            sprite->subpriority = *subpriority_p;
            sprite->subspriteMode = *subspriteMode_p;
            (*state_p)++;
        }
        break;
    case 2:
        sprite->pos2.y += 4;
        (*y_p) += 4;
        if (*y_p >= 0)
        {
            PlaySE(SE_TK_KASYA);
            CameraObjectReset1();
            (*state_p)++;
            return FALSE;
        }
        break;
    case 3:
        return FALSE;
    }
    return TRUE;
}

void FieldCallback_EscapeRopeExit(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    sub_8111CF0();
    ScriptContext2_Enable();
    FreezeObjectEvents();
    gFieldCallback = NULL;
    gObjectEvents[gPlayerAvatar.objectEventId].invisible = TRUE;
    CreateTask(Task_DoEscapeRopeExitFieldEffect, 0);
}

void Task_DoEscapeRopeExitFieldEffect(u8 taskId)
{
    sEscapeRopeExitEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void EscapeRopeExitFieldEffect_Step0(struct Task * task)
{
    if (IsWeatherNotFadingIn())
    {
        PlaySE(SE_TK_WARPOUT);
        task->data[15] = GetPlayerFacingDirection();
        task->data[0]++;
    }
}

void EscapeRopeExitFieldEffect_Step1(struct Task * task)
{
    s16 *data = task->data;
    struct ObjectEvent * playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    bool32 finished = sub_80858A4(playerObj, &data[1], &data[2], &data[3], &data[4], &data[5]);
    playerObj->invisible = FALSE;
    if (data[6] < 8)
        data[6]++;
    else if (data[7] == 0)
    {
        data[6]++;
        data[8] = sub_808576C(playerObj, &data[9], &data[10]);
        if (data[6] >= 50 && data[8] == data[15])
            data[7] = 1;
    }
    if (!finished && data[8] == data[15] && ObjectEventCheckHeldMovementStatus(playerObj) == TRUE)
    {
        playerObj->invisible = FALSE;
        playerObj->fixedPriority = FALSE;
        ScriptContext2_Disable();
        UnfreezeObjectEvents();
        DestroyTask(FindTaskIdByFunc(Task_DoEscapeRopeExitFieldEffect));
    }
}

void Task_DoTeleportFieldEffect(u8 taskId);
void TeleportFieldEffectTask1(struct Task * task);
void TeleportFieldEffectTask2(struct Task * task);
void TeleportFieldEffectTask3(struct Task * task);
void TeleportFieldEffectTask4(struct Task * task);
void FieldCallback_TeleportIn(void);
void Task_DoTeleportInFieldEffect(u8 taskId);
void TeleportInFieldEffectTask1(struct Task * task);
void TeleportInFieldEffectTask2(struct Task * task);
void TeleportInFieldEffectTask3(struct Task * task);

void (*const sTeleportEffectFuncs[])(struct Task * ) = {
    TeleportFieldEffectTask1,
    TeleportFieldEffectTask2,
    TeleportFieldEffectTask3,
    TeleportFieldEffectTask4
};

void CreateTeleportFieldEffectTask(void)
{
    CreateTask(Task_DoTeleportFieldEffect, 0);
}

void Task_DoTeleportFieldEffect(u8 taskId)
{
    sTeleportEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void TeleportFieldEffectTask1(struct Task * task)
{
    ScriptContext2_Enable();
    FreezeObjectEvents();
    CameraObjectReset2();
    task->data[15] = GetPlayerFacingDirection();
    task->data[0]++;
}

void TeleportFieldEffectTask2(struct Task * task)
{
    u8 spinDirections[5] = {
        [DIR_NONE]  = DIR_SOUTH,
        [DIR_SOUTH] = DIR_WEST,
        [DIR_WEST]  = DIR_NORTH,
        [DIR_NORTH] = DIR_EAST,
        [DIR_EAST]  = DIR_SOUTH
    };
    struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (task->data[1] == 0 || (--task->data[1]) == 0)
    {
        ObjectEventTurn(objectEvent, spinDirections[objectEvent->facingDirection]);
        task->data[1] = 8;
        task->data[2]++;
    }
    if (task->data[2] > 7 && task->data[15] == objectEvent->facingDirection)
    {
        task->data[0]++;
        task->data[1] = 4;
        task->data[2] = 8;
        task->data[3] = 1;
        PlaySE(SE_TK_WARPIN);
    }
}

void TeleportFieldEffectTask3(struct Task * task)
{
    u8 spinDirections[5] = {DIR_SOUTH, DIR_WEST, DIR_EAST, DIR_NORTH, DIR_SOUTH};
    struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Sprite * sprite = &gSprites[gPlayerAvatar.spriteId];
    if ((--task->data[1]) <= 0)
    {
        task->data[1] = 4;
        ObjectEventTurn(objectEvent, spinDirections[objectEvent->facingDirection]);
    }
    sprite->pos1.y -= task->data[3];
    task->data[4] += task->data[3];
    if ((--task->data[2]) <= 0 && (task->data[2] = 4, task->data[3] < 8))
    {
        task->data[3] <<= 1;
    }
    if (task->data[4] > 8 && (sprite->oam.priority = 1, sprite->subspriteMode != SUBSPRITES_OFF))
    {
        sprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
    }
    if (task->data[4] >= 0xa8)
    {
        task->data[0]++;
        TryFadeOutOldMapMusic();
        WarpFadeOutScreen();
    }
}

void TeleportFieldEffectTask4(struct Task * task)
{
    if (!gPaletteFade.active)
    {
        if (BGMusicStopped() == TRUE)
        {
            copy_saved_warp3_bank_and_enter_x_to_warp1();
            WarpIntoMap();
            SetMainCallback2(CB2_LoadMap);
            gFieldCallback = FieldCallback_TeleportIn;
            DestroyTask(FindTaskIdByFunc(Task_DoTeleportFieldEffect));
        }
    }
}

void (*const sTeleportInEffectFuncs[])(struct Task * ) = {
    TeleportInFieldEffectTask1,
    TeleportInFieldEffectTask2,
    TeleportInFieldEffectTask3
};

void FieldCallback_TeleportIn(void)
{
    Overworld_PlaySpecialMapMusic();
    WarpFadeInScreen();
    sub_8111CF0();
    ScriptContext2_Enable();
    FreezeObjectEvents();
    gFieldCallback = NULL;
    gObjectEvents[gPlayerAvatar.objectEventId].invisible = TRUE;
    CameraObjectReset2();
    CreateTask(Task_DoTeleportInFieldEffect, 0);
}

void Task_DoTeleportInFieldEffect(u8 taskId)
{
    sTeleportInEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void TeleportInFieldEffectTask1(struct Task * task)
{
    struct Sprite * sprite;
    s16 centerToCornerVecY;
    if (IsWeatherNotFadingIn())
    {
        sprite = &gSprites[gPlayerAvatar.spriteId];
        centerToCornerVecY = -(sprite->centerToCornerVecY << 1);
        sprite->pos2.y = -(sprite->pos1.y + sprite->centerToCornerVecY + gSpriteCoordOffsetY + centerToCornerVecY);
        gObjectEvents[gPlayerAvatar.objectEventId].invisible = FALSE;
        task->data[0]++;
        task->data[1] = 8;
        task->data[2] = 1;
        task->data[14] = sprite->subspriteMode;
        task->data[15] = GetPlayerFacingDirection();
        PlaySE(SE_TK_WARPIN);
    }
}

void TeleportInFieldEffectTask2(struct Task * task)
{
    u8 spinDirections[5] = {1, 3, 4, 2, 1};
    struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    struct Sprite * sprite = &gSprites[gPlayerAvatar.spriteId];
    if ((sprite->pos2.y += task->data[1]) >= -8)
    {
        if (task->data[13] == 0)
        {
            task->data[13]++;
            objectEvent->triggerGroundEffectsOnMove = 1;
            sprite->subspriteMode = task->data[14];
        }
    } else
    {
        sprite->oam.priority = 1;
        if (sprite->subspriteMode != SUBSPRITES_OFF)
        {
            sprite->subspriteMode = SUBSPRITES_IGNORE_PRIORITY;
        }
    }
    if (sprite->pos2.y >= -0x30 && task->data[1] > 1 && !(sprite->pos2.y & 1))
    {
        task->data[1]--;
    }
    if ((--task->data[2]) == 0)
    {
        task->data[2] = 4;
        ObjectEventTurn(objectEvent, spinDirections[objectEvent->facingDirection]);
    }
    if (sprite->pos2.y >= 0)
    {
        sprite->pos2.y = 0;
        task->data[0]++;
        task->data[1] = 1;
        task->data[2] = 0;
    }
}

void TeleportInFieldEffectTask3(struct Task * task)
{
    u8 spinDirections[5] = {1, 3, 4, 2, 1};
    struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if ((--task->data[1]) == 0)
    {
        ObjectEventTurn(objectEvent, spinDirections[objectEvent->facingDirection]);
        task->data[1] = 8;
        if ((++task->data[2]) > 4 && task->data[14] == objectEvent->facingDirection)
        {
            ScriptContext2_Disable();
            CameraObjectReset1();
            UnfreezeObjectEvents();
            DestroyTask(FindTaskIdByFunc(Task_DoTeleportInFieldEffect));
        }
    }
}

void Task_ShowMon_Outdoors(u8 taskId);
void ShowMonEffect_Outdoors_1(struct Task * task);
void ShowMonEffect_Outdoors_2(struct Task * task);
void ShowMonEffect_Outdoors_3(struct Task * task);
void ShowMonEffect_Outdoors_4(struct Task * task);
void ShowMonEffect_Outdoors_5(struct Task * task);
void ShowMonEffect_Outdoors_6(struct Task * task);
void ShowMonEffect_Outdoors_7(struct Task * task);
void VBlankCB_ShowMonEffect_Outdoors(void);
void LoadFieldMoveStreaksTilemapToVram(u16 screenbase);
void Task_ShowMon_Indoors(u8 taskId);
void ShowMonEffect_Indoors_1(struct Task * task);
void ShowMonEffect_Indoors_2(struct Task * task);
void ShowMonEffect_Indoors_3(struct Task * task);
void ShowMonEffect_Indoors_4(struct Task * task);
void ShowMonEffect_Indoors_5(struct Task * task);
void ShowMonEffect_Indoors_6(struct Task * task);
void ShowMonEffect_Indoors_7(struct Task * task);
void VBlankCB_ShowMonEffect_Indoors(void);
void sub_8086728(struct Task * task);
bool8 sub_8086738(struct Task * task);
bool8 sub_80867F0(struct Task * task);
u8 sub_8086860(u32 species, u32 otId, u32 personality);
void sub_80868C0(struct Sprite * sprite);
void sub_8086904(struct Sprite * sprite);
void sub_8086920(struct Sprite * sprite);

void (*const sShowMonOutdoorsEffectFuncs[])(struct Task * task) = {
    ShowMonEffect_Outdoors_1,
    ShowMonEffect_Outdoors_2,
    ShowMonEffect_Outdoors_3,
    ShowMonEffect_Outdoors_4,
    ShowMonEffect_Outdoors_5,
    ShowMonEffect_Outdoors_6,
    ShowMonEffect_Outdoors_7
};

u32 FldEff_FieldMoveShowMon(void)
{
    u8 taskId;
    if (IsMapTypeOutdoors(GetCurrentMapType()) == TRUE)
        taskId = CreateTask(Task_ShowMon_Outdoors, 0xFF);
    else
        taskId = CreateTask(Task_ShowMon_Indoors, 0xFF);
    gTasks[taskId].data[15] = sub_8086860(gFieldEffectArguments[0], gFieldEffectArguments[1], gFieldEffectArguments[2]);
    return 0;
}

u32 FldEff_FieldMoveShowMonInit(void)
{
    u32 r6 = gFieldEffectArguments[0] & 0x80000000;
    u8 partyIdx = gFieldEffectArguments[0];
    gFieldEffectArguments[0] = GetMonData(&gPlayerParty[partyIdx], MON_DATA_SPECIES);
    gFieldEffectArguments[1] = GetMonData(&gPlayerParty[partyIdx], MON_DATA_OT_ID);
    gFieldEffectArguments[2] = GetMonData(&gPlayerParty[partyIdx], MON_DATA_PERSONALITY);
    gFieldEffectArguments[0] |= r6;
    FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON);
    FieldEffectActiveListRemove(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    return 0;
}

void Task_ShowMon_Outdoors(u8 taskId)
{
    sShowMonOutdoorsEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void ShowMonEffect_Outdoors_1(struct Task * task)
{
    task->data[11] = GetGpuReg(REG_OFFSET_WININ);
    task->data[12] = GetGpuReg(REG_OFFSET_WINOUT);
    StoreWordInTwoHalfwords((u16*)&task->data[13], (u32)gMain.vblankCallback);
    task->data[1] = 0xf0f1;
    task->data[2] = 0x5051;
    task->data[3] = 0x3f;
    task->data[4] = 0x3e;
    SetGpuReg(REG_OFFSET_WIN0H, task->data[1]);
    SetGpuReg(REG_OFFSET_WIN0V, task->data[2]);
    SetGpuReg(REG_OFFSET_WININ, task->data[3]);
    SetGpuReg(REG_OFFSET_WINOUT, task->data[4]);
    SetVBlankCallback(VBlankCB_ShowMonEffect_Outdoors);
    task->data[0]++;
}

void ShowMonEffect_Outdoors_2(struct Task * task)
{
    u16 charbase = ((GetGpuReg(REG_OFFSET_BG0CNT) >> 2) << 14);
    u16 screenbase = ((GetGpuReg(REG_OFFSET_BG0CNT) >> 8) << 11);
    CpuCopy16(gFieldMoveStreaksTiles, (void *)(VRAM + charbase), 0x200);
    CpuFill32(0, (void *)(VRAM + screenbase), 0x800);
    LoadPalette(gFieldMoveStreaksPalette, 0xf0, 0x20);
    LoadFieldMoveStreaksTilemapToVram(screenbase);
    task->data[0]++;
}

void ShowMonEffect_Outdoors_3(struct Task * task)
{
    s16 v0;
    s16 v2;
    s16 v3;
    task->data[5] -= 16;
    v0 = ((u16)task->data[1] >> 8);
    v2 = ((u16)task->data[2] >> 8);
    v3 = ((u16)task->data[2] & 0xff);
    v0 -= 16;
    v2 -= 2;
    v3 += 2;
    if (v0 < 0)
    {
        v0 = 0;
    }
    if (v2 < 0x28)
    {
        v2 = 0x28;
    }
    if (v3 > 0x78)
    {
        v3 = 0x78;
    }
    task->data[1] = (v0 << 8) | (task->data[1] & 0xff);
    task->data[2] = (v2 << 8) | v3;
    if (v0 == 0 && v2 == 0x28 && v3 == 0x78)
    {
        gSprites[task->data[15]].callback = sub_80868C0;
        task->data[0]++;
    }
}

void ShowMonEffect_Outdoors_4(struct Task * task)
{
    task->data[5] -= 16;
    if (gSprites[task->data[15]].data[7])
    {
        task->data[0]++;
    }
}

void ShowMonEffect_Outdoors_5(struct Task * task)
{
    s16 v2;
    s16 v3;
    task->data[5] -= 16;
    v2 = (task->data[2] >> 8);
    v3 = (task->data[2] & 0xff);
    v2 += 6;
    v3 -= 6;
    if (v2 > 0x50)
    {
        v2 = 0x50;
    }
    if (v3 < 0x51)
    {
        v3 = 0x51;
    }
    task->data[2] = (v2 << 8) | v3;
    if (v2 == 0x50 && v3 == 0x51)
    {
        task->data[0]++;
    }
}

void ShowMonEffect_Outdoors_6(struct Task * task)
{
    u16 bg0cnt = (GetGpuReg(REG_OFFSET_BG0CNT) >> 8) << 11;
    CpuFill32(0, (void *)VRAM + bg0cnt, 0x800);
    task->data[1] = 0xf1;
    task->data[2] = 0xa1;
    task->data[3] = task->data[11];
    task->data[4] = task->data[12];
    task->data[0]++;
}

void ShowMonEffect_Outdoors_7(struct Task * task)
{
    IntrCallback callback;
    LoadWordFromTwoHalfwords((u16 *)&task->data[13], (u32 *)&callback);
    SetVBlankCallback(callback);
    ChangeBgX(0, 0, 0);
    ChangeBgY(0, 0, 0);
    Menu_LoadStdPal();
    FreeResourcesAndDestroySprite(&gSprites[task->data[15]], task->data[15]);
    FieldEffectActiveListRemove(FLDEFF_FIELD_MOVE_SHOW_MON);
    DestroyTask(FindTaskIdByFunc(Task_ShowMon_Outdoors));
}

void VBlankCB_ShowMonEffect_Outdoors(void)
{
    IntrCallback callback;
    struct Task * task = &gTasks[FindTaskIdByFunc(Task_ShowMon_Outdoors)];
    LoadWordFromTwoHalfwords((u16 *)&task->data[13], (u32 *)&callback);
    callback();
    SetGpuReg(REG_OFFSET_WIN0H, task->data[1]);
    SetGpuReg(REG_OFFSET_WIN0V, task->data[2]);
    SetGpuReg(REG_OFFSET_WININ, task->data[3]);
    SetGpuReg(REG_OFFSET_WINOUT, task->data[4]);
    SetGpuReg(REG_OFFSET_BG0HOFS, task->data[5]);
    SetGpuReg(REG_OFFSET_BG0VOFS, task->data[6]);
}

void LoadFieldMoveStreaksTilemapToVram(u16 screenbase)
{
    u16 i;
    u16 *dest;
    dest = (u16 *)(VRAM + (10 * 32) + screenbase);
    for (i = 0; i < (10 * 32); i++, dest++)
    {
        *dest = gFieldMoveStreaksTilemap[i] | METATILE_ELEVATION_MASK;
    }
}

void (*const sShowMonIndoorsEffectFuncs[])(struct Task * ) = {
    ShowMonEffect_Indoors_1,
    ShowMonEffect_Indoors_2,
    ShowMonEffect_Indoors_3,
    ShowMonEffect_Indoors_4,
    ShowMonEffect_Indoors_5,
    ShowMonEffect_Indoors_6,
    ShowMonEffect_Indoors_7
};

void Task_ShowMon_Indoors(u8 taskId)
{
    sShowMonIndoorsEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void ShowMonEffect_Indoors_1(struct Task * task)
{
    SetGpuReg(REG_OFFSET_BG0HOFS, task->data[1]);
    SetGpuReg(REG_OFFSET_BG0VOFS, task->data[2]);
    StoreWordInTwoHalfwords((u16 *)&task->data[13], (u32)gMain.vblankCallback);
    SetVBlankCallback(VBlankCB_ShowMonEffect_Indoors);
    task->data[0]++;
}

void ShowMonEffect_Indoors_2(struct Task * task)
{
    u16 charbase;
    u16 screenbase;
    charbase = ((GetGpuReg(REG_OFFSET_BG0CNT) >> 2) << 14);
    screenbase = ((GetGpuReg(REG_OFFSET_BG0CNT) >> 8) << 11);
    task->data[12] = screenbase;
    CpuCopy16(gDarknessFieldMoveStreaksTiles, (void *)(VRAM + charbase), 0x80);
    CpuFill32(0, (void *)(VRAM + screenbase), 0x800);
    LoadPalette(gDarknessFieldMoveStreaksPalette, 0xf0, 0x20);
    task->data[0]++;
}

void ShowMonEffect_Indoors_3(struct Task * task)
{
    if (sub_8086738(task))
    {
        task->data[5] = GetGpuReg(REG_OFFSET_WININ);
        SetGpuReg(REG_OFFSET_WININ, (task->data[5] & 0xFF) | 0x1100);
        SetGpuReg(REG_OFFSET_WIN1H, 0x00f0);
        SetGpuReg(REG_OFFSET_WIN1V, 0x2878);
        gSprites[task->data[15]].callback = sub_80868C0;
        task->data[0]++;
    }
    sub_8086728(task);
}

void ShowMonEffect_Indoors_4(struct Task * task)
{
    sub_8086728(task);
    if (gSprites[task->data[15]].data[7])
    {
        task->data[0]++;
    }
}

void ShowMonEffect_Indoors_5(struct Task * task)
{
    sub_8086728(task);
    task->data[3] = task->data[1] & 7;
    task->data[4] = 0;
    SetGpuReg(REG_OFFSET_WIN1H, 0xffff);
    SetGpuReg(REG_OFFSET_WIN1V, 0xffff);
    SetGpuReg(REG_OFFSET_WININ, task->data[5]);
    task->data[0]++;
}

void ShowMonEffect_Indoors_6(struct Task * task)
{
    sub_8086728(task);
    if (sub_80867F0(task))
    {
        task->data[0]++;
    }
}

void ShowMonEffect_Indoors_7(struct Task * task)
{
    IntrCallback intrCallback;
    u16 charbase;
    charbase = (GetGpuReg(REG_OFFSET_BG0CNT) >> 8) << 11;
    CpuFill32(0, (void *)VRAM + charbase, 0x800);
    LoadWordFromTwoHalfwords((u16 *)&task->data[13], (u32 *)&intrCallback);
    SetVBlankCallback(intrCallback);
    ChangeBgX(0, 0, 0);
    ChangeBgY(0, 0, 0);
    Menu_LoadStdPal();
    FreeResourcesAndDestroySprite(&gSprites[task->data[15]], task->data[15]);
    FieldEffectActiveListRemove(FLDEFF_FIELD_MOVE_SHOW_MON);
    DestroyTask(FindTaskIdByFunc(Task_ShowMon_Indoors));
}

void VBlankCB_ShowMonEffect_Indoors(void)
{
    IntrCallback intrCallback;
    struct Task * task;
    task = &gTasks[FindTaskIdByFunc(Task_ShowMon_Indoors)];
    LoadWordFromTwoHalfwords((u16 *)&task->data[13], (u32 *)&intrCallback);
    intrCallback();
    SetGpuReg(REG_OFFSET_BG0HOFS, task->data[1]);
    SetGpuReg(REG_OFFSET_BG0VOFS, task->data[2]);
}

void sub_8086728(struct Task * task)
{
    task->data[1] -= 16;
    task->data[3] += 16;
}

bool8 sub_8086738(struct Task * task)
{
    u16 i;
    u16 srcOffs;
    u16 dstOffs;
    u16 *dest;
    if (task->data[4] >= 32)
    {
        return TRUE;
    }
    dstOffs = (task->data[3] >> 3) & 0x1f;
    if (dstOffs >= task->data[4])
    {
        dstOffs = (32 - dstOffs) & 0x1f;
        srcOffs = (32 - task->data[4]) & 0x1f;
        dest = (u16 *)(VRAM + 0x140 + (u16)task->data[12]);
        for (i = 0; i < 10; i++)
        {
            dest[dstOffs + i * 32] = gDarknessFieldMoveStreaksTilemap[srcOffs + i * 32];
            dest[dstOffs + i * 32] |= 0xf000;

            dest[((dstOffs + 1) & 0x1f) + i * 32] = gDarknessFieldMoveStreaksTilemap[((srcOffs + 1) & 0x1f) + i * 32] | 0xf000;
            dest[((dstOffs + 1) & 0x1f) + i * 32] |= 0xf000;
        }
        task->data[4] += 2;
    }
    return FALSE;
}

bool8 sub_80867F0(struct Task * task)
{
    u16 i;
    u16 dstOffs;
    u16 *dest;
    if (task->data[4] >= 32)
    {
        return TRUE;
    }
    dstOffs = task->data[3] >> 3;
    if (dstOffs >= task->data[4])
    {
        dstOffs = (task->data[1] >> 3) & 0x1f;
        dest = (u16 *)(VRAM + 0x140 + (u16)task->data[12]);
        for (i = 0; i < 10; i++)
        {
            dest[dstOffs + i * 32] = 0xf000;
            dest[((dstOffs + 1) & 0x1f) + i * 32] = 0xf000;
        }
        task->data[4] += 2;
    }
    return FALSE;
}

u8 sub_8086860(u32 species, u32 otId, u32 personality)
{
    bool16 playCry;
    u8 monSprite;
    struct Sprite * sprite;
    playCry = (species & 0x80000000) >> 16;
    species &= 0x7fffffff;
    monSprite = CreateMonSprite_FieldMove(species, otId, personality, 0x140, 0x50, 0);
    sprite = &gSprites[monSprite];
    sprite->callback = SpriteCallbackDummy;
    sprite->oam.priority = 0;
    sprite->data[0] = species;
    sprite->data[6] = playCry;
    return monSprite;
}

void sub_80868C0(struct Sprite * sprite)
{
    if ((sprite->pos1.x -= 20) <= 0x78)
    {
        sprite->pos1.x = 0x78;
        sprite->data[1] = 30;
        sprite->callback = sub_8086904;
        if (sprite->data[6])
        {
            PlayCry2(sprite->data[0], 0, 0x7d, 0xa);
        }
        else
        {
            PlayCry1(sprite->data[0], 0);
        }
    }
}

void sub_8086904(struct Sprite * sprite)
{
    if ((--sprite->data[1]) == 0)
    {
        sprite->callback = sub_8086920;
    }
}

void sub_8086920(struct Sprite * sprite)
{
    if (sprite->pos1.x < -0x40)
    {
        sprite->data[7] = 1;
    }
    else
    {
        sprite->pos1.x -= 20;
    }
}

void Task_FldEffUseSurf(u8 taskId);
void UseSurfEffect_1(struct Task * task);
void UseSurfEffect_2(struct Task * task);
void UseSurfEffect_3(struct Task * task);
void UseSurfEffect_4(struct Task * task);
void UseSurfEffect_5(struct Task * task);

void (*const sUseSurfEffectFuncs[])(struct Task * ) = {
    UseSurfEffect_1,
    UseSurfEffect_2,
    UseSurfEffect_3,
    UseSurfEffect_4,
    UseSurfEffect_5,
};

u8 FldEff_UseSurf(void)
{
    u8 taskId = CreateTask(Task_FldEffUseSurf, 0xff);
    gTasks[taskId].data[15] = gFieldEffectArguments[0];
    sav1_reset_battle_music_maybe();
    if (sub_8056124(MUS_NAMINORI))
        Overworld_ChangeMusicTo(MUS_NAMINORI);
    return FALSE;
}

void Task_FldEffUseSurf(u8 taskId)
{
    sUseSurfEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void UseSurfEffect_1(struct Task * task)
{
    ScriptContext2_Enable();
    FreezeObjectEvents();
    gPlayerAvatar.preventStep = TRUE;
    SetPlayerAvatarStateMask(8);
    PlayerGetDestCoords(&task->data[1], &task->data[2]);
    MoveCoords(gObjectEvents[gPlayerAvatar.objectEventId].placeholder18, &task->data[1], &task->data[2]);
    task->data[0]++;
}

void UseSurfEffect_2(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(objectEvent) || ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        sub_805CB70();
        ObjectEventSetHeldMovement(objectEvent, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
        task->data[0]++;
    }
}

void UseSurfEffect_3(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventCheckHeldMovementStatus(objectEvent))
    {
        gFieldEffectArguments[0] = task->data[15] | 0x80000000;
        FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
        task->data[0]++;
    }
}

void UseSurfEffect_4(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    if (!FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON))
    {
        objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        ObjectEventSetGraphicsId(objectEvent, GetPlayerAvatarGraphicsIdByStateId(2));
        ObjectEventClearHeldMovementIfFinished(objectEvent);
        ObjectEventSetHeldMovement(objectEvent, sub_80641C0(objectEvent->placeholder18));
        gFieldEffectArguments[0] = task->data[1];
        gFieldEffectArguments[1] = task->data[2];
        gFieldEffectArguments[2] = gPlayerAvatar.objectEventId;
        objectEvent->mapobj_unk_1A = FieldEffectStart(FLDEFF_SURF_BLOB);
        task->data[0]++;
    }
}

void UseSurfEffect_5(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        gPlayerAvatar.preventStep = FALSE;
        gPlayerAvatar.flags &= 0xdf;
        ObjectEventSetHeldMovement(objectEvent, GetFaceDirectionMovementAction(objectEvent->placeholder18));
        sub_80DC44C(objectEvent->mapobj_unk_1A, 1);
        UnfreezeObjectEvents();
        ScriptContext2_Disable();
        FieldEffectActiveListRemove(FLDEFF_USE_SURF);
        DestroyTask(FindTaskIdByFunc(Task_FldEffUseSurf));
        HelpSystem_SetSomeVariable2(22);
    }
}

void Task_FldEffUseVsSeeker(u8 taskId);
void UseVsSeekerEffect_1(struct Task * task);
void UseVsSeekerEffect_2(struct Task * task);
void UseVsSeekerEffect_3(struct Task * task);
void UseVsSeekerEffect_4(struct Task * task);

void (*const sUseVsSeekerEffectFuncs[])(struct Task * task) = {
    UseVsSeekerEffect_1,
    UseVsSeekerEffect_2,
    UseVsSeekerEffect_3,
    UseVsSeekerEffect_4
};

u32 FldEff_UseVsSeeker(void)
{
    if (gQuestLogState == QL_STATE_1)
        sub_811278C(8, 89);
    CreateTask(Task_FldEffUseVsSeeker, 0xFF);
    return 0;
}

void Task_FldEffUseVsSeeker(u8 taskId)
{
    sUseVsSeekerEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void UseVsSeekerEffect_1(struct Task * task)
{
    ScriptContext2_Enable();
    FreezeObjectEvents();
    gPlayerAvatar.preventStep = TRUE;
    task->data[0]++;
}

void UseVsSeekerEffect_2(struct Task * task)
{
    struct ObjectEvent * playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(playerObj) || ObjectEventClearHeldMovementIfFinished(playerObj))
    {
        sub_805CBE8();
        ObjectEventSetHeldMovement(playerObj, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
        task->data[0]++;
    }
}

void UseVsSeekerEffect_3(struct Task * task)
{
    struct ObjectEvent * playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventClearHeldMovementIfFinished(playerObj))
    {
        if (gPlayerAvatar.flags & (PLAYER_AVATAR_FLAG_ACRO_BIKE | PLAYER_AVATAR_FLAG_MACH_BIKE))
            ObjectEventSetGraphicsId(playerObj, GetPlayerAvatarGraphicsIdByStateId(1));
        else if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING)
            ObjectEventSetGraphicsId(playerObj, GetPlayerAvatarGraphicsIdByStateId(2));
        else
            ObjectEventSetGraphicsId(playerObj, GetPlayerAvatarGraphicsIdByStateId(0));
        ObjectEventForceSetSpecialAnim(playerObj, GetFaceDirectionMovementAction(playerObj->facingDirection));
        task->data[0]++;
    }
}

void UseVsSeekerEffect_4(struct Task * task)
{
    struct ObjectEvent * playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventClearHeldMovementIfFinished(playerObj))
    {
        gPlayerAvatar.preventStep = FALSE;
        FieldEffectActiveListRemove(FLDEFF_USE_VS_SEEKER);
        DestroyTask(FindTaskIdByFunc(Task_FldEffUseVsSeeker));
    }
}

void sub_8086D94(struct Sprite * sprite);

u8 FldEff_NpcFlyOut(void)
{
    u8 spriteId = CreateSprite(gFieldEffectObjectTemplatePointers[26], 0x78, 0, 1);
    struct Sprite * sprite = &gSprites[spriteId];

    sprite->oam.paletteNum = 0;
    sprite->oam.priority = 1;
    sprite->callback = sub_8086D94;
    sprite->data[1] = gFieldEffectArguments[0];
    PlaySE(SE_W019);
    return spriteId;
}

void sub_8086D94(struct Sprite * sprite)
{
    struct Sprite * npcSprite;

    sprite->pos2.x = Cos(sprite->data[2], 0x8c);
    sprite->pos2.y = Sin(sprite->data[2], 0x48);
    sprite->data[2] = (sprite->data[2] + 4) & 0xff;
    if (sprite->data[0])
    {
        npcSprite = &gSprites[sprite->data[1]];
        npcSprite->coordOffsetEnabled = 0;
        npcSprite->pos1.x = sprite->pos1.x + sprite->pos2.x;
        npcSprite->pos1.y = sprite->pos1.y + sprite->pos2.y - 8;
        npcSprite->pos2.x = 0;
        npcSprite->pos2.y = 0;
    }
    if (sprite->data[2] >= 0x80)
    {
        FieldEffectStop(sprite, FLDEFF_NPCFLY_OUT);
    }
}

void Task_UseFly(u8 taskId);
void UseFlyEffect_1(struct Task * task);
void UseFlyEffect_2(struct Task * task);
void UseFlyEffect_3(struct Task * task);
void UseFlyEffect_4(struct Task * task);
void UseFlyEffect_5(struct Task * task);
void UseFlyEffect_6(struct Task * task);
void UseFlyEffect_7(struct Task * task);
void UseFlyEffect_8(struct Task * task);
void UseFlyEffect_9(struct Task * task);
u8 sub_8087168(void);
bool8 sub_80871AC(u8 flyBlobSpriteId);
void sub_80871C8(u8 flyBlobSpriteId);
void sub_8087204(u8 flyBlobSpriteId, u8 playerSpriteId);
void sub_8087220(struct Sprite * sprite);
void sub_80872F0(struct Sprite * sprite);
void sub_80877FC(struct Sprite * sprite, u8 affineAnimId);
void sub_8087828(struct Sprite * sprite);

void (*const sUseFlyEffectFuncs[])(struct Task * ) = {
    UseFlyEffect_1,
    UseFlyEffect_2,
    UseFlyEffect_3,
    UseFlyEffect_4,
    UseFlyEffect_5,
    UseFlyEffect_6,
    UseFlyEffect_7,
    UseFlyEffect_8,
    UseFlyEffect_9
};

u8 FldEff_UseFly(void)
{
    u8 taskId = CreateTask(Task_UseFly, 0xfe);
    gTasks[taskId].data[1] = gFieldEffectArguments[0];
    return 0;
}

void Task_UseFly(u8 taskId)
{
    sUseFlyEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void UseFlyEffect_1(struct Task * task)
{
    struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(objectEvent) || ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->data[15] = gPlayerAvatar.flags;
        gPlayerAvatar.preventStep = TRUE;
        SetPlayerAvatarStateMask(1);
        sub_805CB70();
        ObjectEventSetHeldMovement(objectEvent, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
        task->data[0]++;
    }
}

void UseFlyEffect_2(struct Task * task)
{
    struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->data[0]++;
        gFieldEffectArguments[0] = task->data[1];
        FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    }
}

void UseFlyEffect_3(struct Task * task)
{
    if (!FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON))
    {
        struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        if (task->data[15] & 0x08)
        {
            sub_80DC44C(objectEvent->mapobj_unk_1A, 2);
            sub_80DC478(objectEvent->mapobj_unk_1A, 0);
        }
        task->data[1] = sub_8087168();
        task->data[0]++;
    }
}

void UseFlyEffect_4(struct Task * task)
{
    if (sub_80871AC(task->data[1]))
    {
        task->data[0]++;
        task->data[2] = 16;
        SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_ON_FOOT);
        ObjectEventSetHeldMovement(&gObjectEvents[gPlayerAvatar.objectEventId], MOVEMENT_ACTION_FACE_LEFT);
    }
}

void UseFlyEffect_5(struct Task * task)
{
    struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if ((task->data[2] == 0 || (--task->data[2]) == 0) && ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->data[0]++;
        PlaySE(SE_W019);
        sub_80871C8(task->data[1]);
    }
}

void UseFlyEffect_6(struct Task * task)
{
    if ((++task->data[2]) >= 8)
    {
        struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        ObjectEventSetGraphicsId(objectEvent, GetPlayerAvatarGraphicsIdByStateId(2));
        StartSpriteAnim(&gSprites[objectEvent->spriteId], 0x16);
        objectEvent->inanimate = 1;
        ObjectEventSetHeldMovement(objectEvent, MOVEMENT_ACTION_JUMP_IN_PLACE_LEFT);
        task->data[0]++;
        task->data[2] = 0;
    }
}

void UseFlyEffect_7(struct Task * task)
{
    if ((++task->data[2]) >= 10)
    {
        struct ObjectEvent * objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        ObjectEventClearAnimIfSpecialAnimActive(objectEvent);
        objectEvent->inanimate = 0;
        objectEvent->hasShadow = 0;
        sub_8087204(task->data[1], objectEvent->spriteId);
        StartSpriteAnim(&gSprites[task->data[1]], gSaveBlock2Ptr->playerGender * 2 + 1);
        sub_80877FC(&gSprites[task->data[1]], 0);
        gSprites[task->data[1]].callback = sub_8087828;
        CameraObjectReset2();
        task->data[0]++;
    }
}

void UseFlyEffect_8(struct Task * task)
{
    if (sub_80871AC(task->data[1]))
    {
        WarpFadeOutScreen();
        task->data[0]++;
    }
}

void UseFlyEffect_9(struct Task * task)
{
    if (!gPaletteFade.active)
    {
        FieldEffectActiveListRemove(FLDEFF_USE_FLY);
        DestroyTask(FindTaskIdByFunc(Task_UseFly));
    }
}

u8 sub_8087168(void)
{
    u8 spriteId;
    struct Sprite * sprite;
    spriteId = CreateSprite(gFieldEffectObjectTemplatePointers[26], 0xff, 0xb4, 0x1);
    sprite = &gSprites[spriteId];
    sprite->oam.paletteNum = 0;
    sprite->oam.priority = 1;
    sprite->callback = sub_8087220;
    return spriteId;
}

u8 sub_80871AC(u8 spriteId)
{
    return gSprites[spriteId].data[7];
}

void sub_80871C8(u8 spriteId)
{
    struct Sprite * sprite;
    sprite = &gSprites[spriteId];
    sprite->callback = sub_80872F0;
    sprite->pos1.x = 0x78;
    sprite->pos1.y = 0x00;
    sprite->pos2.x = 0;
    sprite->pos2.y = 0;
    memset(&sprite->data[0], 0, 8 * sizeof(u16) /* zero all data cells */);
    sprite->data[6] = 0x40;
}

void sub_8087204(u8 a0, u8 a1)
{
    gSprites[a0].data[6] = a1;
}

const union AffineAnimCmd gUnknown_83CC19C[] = {
    AFFINEANIMCMD_FRAME( 8,  8, 226,  0),
    AFFINEANIMCMD_FRAME(28, 28,   0, 30),
    AFFINEANIMCMD_END
};

const union AffineAnimCmd gUnknown_83CC1B4[] = {
    AFFINEANIMCMD_FRAME(256, 256, 64,  0),
    AFFINEANIMCMD_FRAME(-10, -10,  0, 22),
    AFFINEANIMCMD_END
};

const union AffineAnimCmd *const gUnknown_83CC1CC[] = {
    gUnknown_83CC19C,
    gUnknown_83CC1B4
};

void sub_8087220(struct Sprite * sprite)
{
    if (sprite->data[7] == 0)
    {
        if (sprite->data[0] == 0)
        {
            sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
            sprite->affineAnims = gUnknown_83CC1CC;
            InitSpriteAffineAnim(sprite);
            StartSpriteAffineAnim(sprite, 0);
            if (gSaveBlock2Ptr->playerGender == MALE)
                sprite->pos1.x = 0x80;
            else
                sprite->pos1.x = 0x76;
            sprite->pos1.y = -0x30;
            sprite->data[0]++;
            sprite->data[1] = 0x40;
            sprite->data[2] = 0x100;
        }
        sprite->data[1] += (sprite->data[2] >> 8);
        sprite->pos2.x = Cos(sprite->data[1], 0x78);
        sprite->pos2.y = Sin(sprite->data[1], 0x78);
        if (sprite->data[2] < 0x800)
        {
            sprite->data[2] += 0x60;
        }
        if (sprite->data[1] > 0x81)
        {
            sprite->data[7]++;
            sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
            FreeOamMatrix(sprite->oam.matrixNum);
            CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, ST_OAM_AFFINE_OFF);
        }
    }
}

void sub_80872F0(struct Sprite * sprite)
{
    sprite->pos2.x = Cos(sprite->data[2], 0x8c);
    sprite->pos2.y = Sin(sprite->data[2], 0x48);
    sprite->data[2] = (sprite->data[2] + 4) & 0xff;
    if (sprite->data[6] != MAX_SPRITES)
    {
        struct Sprite * sprite1 = &gSprites[sprite->data[6]];
        sprite1->coordOffsetEnabled = 0;
        sprite1->pos1.x = sprite->pos1.x + sprite->pos2.x;
        sprite1->pos1.y = sprite->pos1.y + sprite->pos2.y - 8;
        sprite1->pos2.x = 0;
        sprite1->pos2.y = 0;
    }
    if (sprite->data[2] >= 0x80)
    {
        sprite->data[7] = 1;
    }
}

void sub_8087364(struct Sprite * sprite)
{
    if (sprite->data[7] == 0)
    {
        if (sprite->data[0] == 0)
        {
            sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
            sprite->affineAnims = gUnknown_83CC1CC;
            InitSpriteAffineAnim(sprite);
            StartSpriteAffineAnim(sprite, 1);
            if (gSaveBlock2Ptr->playerGender == MALE)
                sprite->pos1.x = 0x70;
            else
                sprite->pos1.x = 0x64;
            sprite->pos1.y = -0x20;
            sprite->data[0]++;
            sprite->data[1] = 0xf0;
            sprite->data[2] = 0x800;
            sprite->data[4] = 0x80;
        }
        sprite->data[1] += sprite->data[2] >> 8;
        sprite->data[3] += sprite->data[2] >> 8;
        sprite->data[1] &= 0xff;
        sprite->pos2.x = Cos(sprite->data[1], 0x20);
        sprite->pos2.y = Sin(sprite->data[1], 0x78);
        if (sprite->data[2] > 0x100)
        {
            sprite->data[2] -= sprite->data[4];
        }
        if (sprite->data[4] < 0x100)
        {
            sprite->data[4] += 24;
        }
        if (sprite->data[2] < 0x100)
        {
            sprite->data[2] = 0x100;
        }
        if (sprite->data[3] >= 60)
        {
            sprite->data[7]++;
            sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
            FreeOamMatrix(sprite->oam.matrixNum);
            sprite->invisible = TRUE;
        }
    }
}

void sub_8087458(u8 spriteId)
{
    sub_80871C8(spriteId);
    gSprites[spriteId].callback = sub_8087364;
}

void Task_FldEffFlyIn(u8 taskId);
void FlyInEffect_1(struct Task * task);
void FlyInEffect_2(struct Task * task);
void FlyInEffect_3(struct Task * task);
void FlyInEffect_4(struct Task * task);
void FlyInEffect_5(struct Task * task);
void FlyInEffect_6(struct Task * task);
void FlyInEffect_7(struct Task * task);
void sub_80878C0(struct Sprite * sprite);

void (*const sFlyInEffectFuncs[])(struct Task * task) = {
    FlyInEffect_1,
    FlyInEffect_2,
    FlyInEffect_3,
    FlyInEffect_4,
    FlyInEffect_5,
    FlyInEffect_6,
    FlyInEffect_7
};

u32 FldEff_FlyIn(void)
{
    CreateTask(Task_FldEffFlyIn, 0xfe);
    return 0;
}

void Task_FldEffFlyIn(u8 taskId)
{
    sFlyInEffectFuncs[gTasks[taskId].data[0]](&gTasks[taskId]);
}

void FlyInEffect_1(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(objectEvent) || ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->data[0]++;
        task->data[2] = 33;
        task->data[15] = gPlayerAvatar.flags;
        gPlayerAvatar.preventStep = TRUE;
        SetPlayerAvatarStateMask(0x01);
        if (task->data[15] & 0x08)
        {
            sub_80DC44C(objectEvent->mapobj_unk_1A, 0);
        }
        ObjectEventSetGraphicsId(objectEvent, GetPlayerAvatarGraphicsIdByStateId(2));
        CameraObjectReset2();
        ObjectEventTurn(objectEvent, DIR_WEST);
        StartSpriteAnim(&gSprites[objectEvent->spriteId], 0x16);
        objectEvent->invisible = FALSE;
        task->data[1] = sub_8087168();
        sub_80871C8(task->data[1]);
        sub_8087204(task->data[1], objectEvent->spriteId);
        StartSpriteAnim(&gSprites[task->data[1]], gSaveBlock2Ptr->playerGender * 2 + 2);
        sub_80877FC(&gSprites[task->data[1]], 1);
        gSprites[task->data[1]].callback = sub_8087828;
    }
}

void FlyInEffect_2(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    struct Sprite * sprite;
    sub_80878C0(&gSprites[task->data[1]]);
    if (task->data[2] == 0 || (--task->data[2]) == 0)
    {
        objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        sprite = &gSprites[objectEvent->spriteId];
        sub_8087204(task->data[1], 0x40);
        sprite->pos1.x += sprite->pos2.x;
        sprite->pos1.y += sprite->pos2.y;
        sprite->pos2.x = 0;
        sprite->pos2.y = 0;
        task->data[0]++;
        task->data[2] = 0;
    }
}

void FlyInEffect_3(struct Task * task)
{
    s16 gUnknown_83CC1F0[18] = {
        -2,
        -4,
        -5,
        -6,
        -7,
        -8,
        -8,
        -8,
        -7,
        -7,
        -6,
        -5,
        -3,
        -2,
        0,
        2,
        4,
        8
    };
    struct Sprite * sprite = &gSprites[gPlayerAvatar.spriteId];
    sprite->pos2.y = gUnknown_83CC1F0[task->data[2]];
    if ((++task->data[2]) >= 18)
    {
        task->data[0]++;
    }
}

void FlyInEffect_4(struct Task * task)
{
    struct ObjectEvent * objectEvent;
    struct Sprite * sprite;
    if (sub_80871AC(task->data[1]))
    {
        objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        sprite = &gSprites[objectEvent->spriteId];
        objectEvent->inanimate = 0;
        sub_805F724(objectEvent, objectEvent->currentCoords.x, objectEvent->currentCoords.y);
        sprite->pos2.x = 0;
        sprite->pos2.y = 0;
        sprite->coordOffsetEnabled = 1;
        sub_805CB70();
        ObjectEventSetHeldMovement(objectEvent, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
        task->data[0]++;
    }
}

void FlyInEffect_5(struct Task * task)
{
    if (ObjectEventClearHeldMovementIfFinished(&gObjectEvents[gPlayerAvatar.objectEventId]))
    {
        task->data[0]++;
        sub_8087458(task->data[1]);
    }
}

void FlyInEffect_6(struct Task * task)
{
    if (sub_80871AC(task->data[1]))
    {
        DestroySprite(&gSprites[task->data[1]]);
        task->data[0]++;
        task->data[1] = 0x10;
    }
}

void FlyInEffect_7(struct Task * task)
{
    u8 state;
    struct ObjectEvent * objectEvent;
    if ((--task->data[1]) == 0)
    {
        objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        state = 0;
        if (task->data[15] & 0x08)
        {
            state = 2;
            sub_80DC44C(objectEvent->mapobj_unk_1A, 1);
        }
        ObjectEventSetGraphicsId(objectEvent, GetPlayerAvatarGraphicsIdByStateId(state));
        ObjectEventTurn(objectEvent, DIR_SOUTH);
        gPlayerAvatar.flags = task->data[15];
        gPlayerAvatar.preventStep = FALSE;
        FieldEffectActiveListRemove(FLDEFF_FLY_IN);
        DestroyTask(FindTaskIdByFunc(Task_FldEffFlyIn));
    }
}

const union AffineAnimCmd gUnknown_83CC214[] = {
    AFFINEANIMCMD_FRAME(24, 24, 0, 1),
    AFFINEANIMCMD_JUMP(0)
};

const union AffineAnimCmd gUnknown_83CC224[] = {
    AFFINEANIMCMD_FRAME(512, 512, 0, 1),
    AFFINEANIMCMD_FRAME(-16, -16, 0, 1),
    AFFINEANIMCMD_JUMP(1)
};

const union AffineAnimCmd *const gUnknown_83CC23C[] = {
    gUnknown_83CC214,
    gUnknown_83CC224
};

void sub_80877FC(struct Sprite * sprite, u8 affineAnimId)
{
    sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    sprite->affineAnims = gUnknown_83CC23C;
    InitSpriteAffineAnim(sprite);
    StartSpriteAffineAnim(sprite, affineAnimId);
}

void sub_8087828(struct Sprite * sprite)
{
    struct Sprite * sprite2;
    sprite->pos2.x = Cos(sprite->data[2], 0xB4);
    sprite->pos2.y = Sin(sprite->data[2], 0x48);
    sprite->data[2] += 2;
    sprite->data[2] &= 0xFF;
    if (sprite->data[6] != MAX_SPRITES)
    {
        sprite2 = &gSprites[sprite->data[6]];
        sprite2->coordOffsetEnabled = FALSE;
        sprite2->pos1.x = sprite->pos1.x + sprite->pos2.x;
        sprite2->pos1.y = sprite->pos1.y + sprite->pos2.y - 8;
        sprite2->pos2.x = 0;
        sprite2->pos2.y = 0;
    }
    if (sprite->data[2] >= 0x80)
    {
        sprite->data[7] = 1;
        sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
        FreeOamMatrix(sprite->oam.matrixNum);
        CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, ST_OAM_AFFINE_OFF);
    }
}

void sub_80878C0(struct Sprite * sprite)
{
    if (sprite->oam.affineMode != ST_OAM_AFFINE_OFF)
    {
        if (gOamMatrices[sprite->oam.matrixNum].a == 0x100 || gOamMatrices[sprite->oam.matrixNum].d == 0x100)
        {
            sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
            FreeOamMatrix(sprite->oam.matrixNum);
            CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, ST_OAM_AFFINE_OFF);
            StartSpriteAnim(sprite, 0);
            sprite->callback = sub_80872F0;
        }
    }
}

void Task_FldEffUnk43(u8 taskId);

bool8 FldEff_Unk43(void)
{
    u8 taskId;
    u8 objectEventIdBuffer;
    s32 x;
    s32 y;
    struct ObjectEvent * objectEvent;
    if (!TryGetObjectEventIdByLocalIdAndMap(gFieldEffectArguments[0], gFieldEffectArguments[1], gFieldEffectArguments[2], &objectEventIdBuffer))
    {
        objectEvent = &gObjectEvents[objectEventIdBuffer];
        x = objectEvent->currentCoords.x - 7;
        y = objectEvent->currentCoords.y - 7;
        x = (gFieldEffectArguments[3] - x) * 16;
        y = (gFieldEffectArguments[4] - y) * 16;
        npc_coords_shift(objectEvent, gFieldEffectArguments[3] + 7, gFieldEffectArguments[4] + 7);
        taskId = CreateTask(Task_FldEffUnk43, 0x50);
        gTasks[taskId].data[1] = objectEvent->spriteId;
        gTasks[taskId].data[2] = gSprites[objectEvent->spriteId].pos1.x + x;
        gTasks[taskId].data[3] = gSprites[objectEvent->spriteId].pos1.y + y;
        gTasks[taskId].data[8] = gFieldEffectArguments[5];
        gTasks[taskId].data[9] = objectEventIdBuffer;
    }
    return FALSE;
}

void Task_FldEffUnk43(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    struct Sprite * sprite = &gSprites[data[1]];
    struct ObjectEvent * objectEvent;
    switch (data[0])
    {
    case 0:
        data[4] = sprite->pos1.x << 4;
        data[5] = sprite->pos1.y << 4;
        data[6] = ((data[2] << 4) - data[4]) / data[8];
        data[7] = ((data[3] << 4) - data[5]) / data[8];
        data[0]++;
        // fallthrough
    case 1:
        if (data[8] != 0)
        {
            data[8]--;
            data[4] += data[6];
            data[5] += data[7];
            sprite->pos1.x = data[4] >> 4;
            sprite->pos1.y = data[5] >> 4;
        }
        else
        {
            objectEvent = &gObjectEvents[data[9]];
            sprite->pos1.x = data[2];
            sprite->pos1.y = data[3];
            npc_coords_shift_still(objectEvent);
            objectEvent->triggerGroundEffectsOnStop = TRUE;
            FieldEffectActiveListRemove(FLDEFF_UNK_43);
            DestroyTask(taskId);
        }
        break;
    }
}
