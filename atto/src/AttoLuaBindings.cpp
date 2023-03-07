#include "AttoAsset.h"

#define CHECK_PARAM_COUNT(x) if (lua_gettop(L) != x) { ATTOERROR("Invalid number of parameters for function %s", __func__); return -1; }
#define RET_IF_FAILED(expr) if (!(expr)) { ATTOERROR("Invalid parameters for function %s", __func__) return -1;}
#define GET_TABLE(idx) LuaTable table(L, idx)

#define GET_ENGINE_PARAM()          LeEngine *engine = (LeEngine*)LuaScript::GetUserData(L, 1);
#define GET_I32_PARAM(v, idx)       RET_IF_FAILED( lua_isnumber(L, idx) ) v = (i32)lua_tointeger(L, idx)
#define GET_U32_PARAM(v, idx)       RET_IF_FAILED( lua_isnumber(L, idx) ) v = (u32)lua_tointeger(L, idx)
#define GET_F32_PARAM(v, idx)       RET_IF_FAILED( lua_isnumber(L, idx) ) v = (f32)lua_tonumber(L, idx)
#define GET_BOOL_PARAM(v, idx)      RET_IF_FAILED( lua_isboolean(L, idx) ) v = lua_toboolean(L, idx)
#define GET_STRING_PARAM(v, idx)    RET_IF_FAILED( lua_isstring(L, idx) ) v = lua_tostring(L, idx)

#define GET_OPTIONAL_F32_PARAM(v, idx)       if( idx <= lua_gettop(L) && lua_isnumber(L, idx) ) { v = (f32)lua_tonumber(L, idx); }
#define GET_OPTIONAL_BOOL_PARAM(v, idx)      if( idx <= lua_gettop(L) && lua_isboolean(L, idx) ) { v = lua_toboolean(L, idx); }


#define PUSH_I32_PARAM(v)           lua_pushinteger(L, v)
#define PUSH_U32_PARAM(v)           lua_pushinteger(L, v)

namespace atto
{

    void LeEngine::InitializeLuaBindings() {
        luaEngine.CreateNew();

        luaEngine.SetGlobal("engine", this);

        luaEngine.RegisterFunction("atto_id_from_string", Lua_IdFromString);
        luaEngine.RegisterFunction("atto_audio_play", Lua_AudioPlay);
        luaEngine.RegisterFunction("atto_draw_shape_rect", Lua_DrawShapeRect);
        luaEngine.RegisterFunction("atto_draw_shape_rect_cd", Lua_DrawShapeRectCenterDim);
        luaEngine.RegisterFunction("atto_draw_shape_circle", Lua_DrawShapeCircle);
        luaEngine.RegisterFunction("atto_draw_shape_round_rect", Lua_DrawShapeRoundRect);
        luaEngine.RegisterFunction("atto_draw_sprite", Lua_DrawSprite);

        // TODO: Load assets via asset functions and remove hard coded string!!
        // TODO: Asset system for lua
        luaEngine.LoadFile("assets/scripts/engine.lua");
    }

    i32 LeEngine::Lua_IdFromString(lua_State* L) {
        CHECK_PARAM_COUNT(1);

        LargeString str = {};
        GET_STRING_PARAM(str, 1);

        u32 id = AssetId::Create(str.GetCStr()).id;

        PUSH_U32_PARAM(id);
        
        return 1;
    }

    i32 LeEngine::Lua_AudioPlay(lua_State* L) {
        CHECK_PARAM_COUNT(4);
        GET_ENGINE_PARAM();
        
        AudioAssetId    audioAssetId = {};
        bool            looping = false;
        f32             volume = 1.0f;
        
        GET_U32_PARAM(audioAssetId.id, 2);
        GET_BOOL_PARAM(looping, 3);
        GET_F32_PARAM(volume, 4);
        
        engine->AudioPlay(audioAssetId, looping, volume);
        
        return 0;
    }

    i32 LeEngine::Lua_DrawShapeRect(lua_State* L) {
        CHECK_PARAM_COUNT(5);
        GET_ENGINE_PARAM();

        glm::vec2 bl = {};
        glm::vec2 tr = {};
        
        GET_F32_PARAM(bl.x, 2);
        GET_F32_PARAM(bl.y, 3);
        GET_F32_PARAM(tr.x, 4);
        GET_F32_PARAM(tr.y, 5);

        engine->DrawShapeRect(bl, tr);

        return 0;
    }

    i32 LeEngine::Lua_DrawShapeRectCenterDim(lua_State* L) {
        CHECK_PARAM_COUNT(5);
        GET_ENGINE_PARAM();

        glm::vec2 center = {};
        glm::vec2 dims = {};

        GET_F32_PARAM(center.x, 2);
        GET_F32_PARAM(center.y, 3);
        GET_F32_PARAM(dims.x, 4);
        GET_F32_PARAM(dims.y, 5);

        engine->DrawShapeRectCenterDim(center, dims);

        return 0;
    }

    i32 LeEngine::Lua_DrawShapeCircle(lua_State* L) {
        CHECK_PARAM_COUNT(5);
        GET_ENGINE_PARAM();

        glm::vec2 center = {};
        f32 radius = 0.0f;

        GET_F32_PARAM(center.x, 2);
        GET_F32_PARAM(center.y, 3);
        GET_F32_PARAM(radius, 4);

        engine->DrawShapeCircle(center, radius);

        return i32();
    }

    i32 LeEngine::Lua_DrawShapeRoundRect(lua_State* L) {
        CHECK_PARAM_COUNT(6);
        GET_ENGINE_PARAM();

        glm::vec2 bl = {};
        glm::vec2 tr = {};
        f32 rad = 0.0f;

        GET_F32_PARAM(bl.x, 2);
        GET_F32_PARAM(bl.y, 3);
        GET_F32_PARAM(tr.x, 4);
        GET_F32_PARAM(tr.y, 5);
        GET_F32_PARAM(rad, 5);

        engine->DrawShapeRoundRect(bl, tr, rad);

        return 0;
    }

    i32 LeEngine::Lua_DrawSprite(lua_State* L) {
        CHECK_PARAM_COUNT(5);
        GET_ENGINE_PARAM();

        AssetId         id = {};
        i32             frameIndex = 0;
        glm::vec2       pos = glm::vec2(0);

        
        GET_U32_PARAM(id.id, 2);
        GET_F32_PARAM(pos.x, 3);
        GET_F32_PARAM(pos.y, 4);
        GET_I32_PARAM(frameIndex, 5);
        
        engine->DrawSprite(id, pos, 0, frameIndex);

        return 0;
    }
}
