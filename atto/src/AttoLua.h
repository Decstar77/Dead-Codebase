#pragma once

#include "AttoLib.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <utility>

namespace atto
{
    class LuaTable {
    public:
        LuaTable();
        LuaTable(lua_State* L, i32 index, bool isSubTable = false);
        ~LuaTable();

        LuaTable operator[](const char *key) const;

        bool Loop();
        bool LoopGet(LuaTable& value, SmallString &tableName);
        void EndLoop();

        bool Get(i32& value) const;
        bool Get(u32& value) const;
        bool Get(f32& value) const;
        bool Get(bool& value) const;
        bool Get(SmallString& value) const;
        bool Get(LargeString& value) const;
        bool Get(glm::vec2& value) const;

        void Put(const char * name, u32 value);

        lua_State* L = nullptr;
        i32 index = 0;
        i32 loopIteration = 0;
        bool isSubTable = false;
    };
    
    class LuaScript {
    public:
        
        void CreateNew();
        void LoadFile(const char* filename);
        
        bool Load(const char* filename);
        bool LoadSafe(const char* filename);
        void Free();

        bool GetGlobalSafe(const char* name, i32& value);
        bool GetGlobalSafe(const char* name, f32& value);
        bool GetGlobalSafe(const char* name, bool& value);
        
        bool GetGlobal(const char* name, i32& value);
        bool GetGlobal(const char* name, f32& value);
        bool GetGlobal(const char* name, bool& value);
        bool GetGlobal(const char* name, LuaTable& value);

        void SetGlobal(const char* name, i32 value);
        void SetGlobal(const char* name, f32 value);
        void SetGlobal(const char* name, bool value);
        void SetGlobal(const char* name, void* value);
        
        void RegisterFunction(const char* name, lua_CFunction func);
        
        void PushValue(i32 value);
        void PushValue(f32 value);
        void PushValue(void* value);
        bool CallFunction(i32 numArgs, i32 numResults);

        static void* GetUserData(lua_State* L, i32 index);

        inline void PushValues() { }
        template<typename _type_, typename... _args_>
        void PushValues(const _type_& arg0, const _args_&&... args);
        template<typename... _args_>
        bool CallVoidFunction(const char* name, const _args_&&... args);

        lua_State* L = nullptr;
        
    private:
        template<typename _type_>
        bool GetGlobalNumber(const char* name, _type_& value);

        template<typename _type_>
        bool GetGlobalNumberSafe(const char* name, _type_& value);

        template<typename _type_>
        void SetGlobalNumber(const char* name, _type_ value);
    };

    template<typename _type_>
    bool LuaScript::GetGlobalNumber(const char* name, _type_& value) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");

        if (lua_getglobal(L, name) != LUA_TNUMBER) {
            lua_pop(L, 1);
            ATTOERROR("LuaScript::GetGlobal -> Could not get global %s", name);
            return false;
        }

        if (!lua_isnumber(L, -1)) {
            ATTOERROR("LuaScript::GetGlobal -> Global %s is not a number", name);
            return false;
        }

        value = (_type_)lua_tonumber(L, -1);
        
        lua_pop(L, 1);

        return true;
    }

    template<typename _type_>
    bool LuaScript::GetGlobalNumberSafe(const char* name, _type_& value) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");

        if (lua_getglobal(L, name) != LUA_TNUMBER) {
            ATTOTRACE("LuaScript::GetGlobal -> Global %s is not a number", name);
            return false;
        }

        if (!lua_isnumber(L, -1)) {
            ATTOTRACE("LuaScript::GetGlobal -> Global %s is not a number", name);
            return false;
        }

        value = (_type_)lua_tonumber(L, -1);

        lua_pop(L, 1);

        return true;
    }

    template<typename _type_>
    void LuaScript::SetGlobalNumber(const char* name, _type_ value) {
        Assert(L != nullptr, "LuaScript::SetGlobal -> Lua state is null");

        lua_pushnumber(L, (double)value);
        lua_setglobal(L, name);
    }

    template<typename _type_, typename... _args_>
    void LuaScript::PushValues(const _type_& arg0, const _args_&&... args) {
        PushValue(arg0);
        PushValues(args...);
    }

    template<typename... _args_>
    bool LuaScript::CallVoidFunction(const char* name, const _args_&&... args) {
        Assert(L != nullptr, "LuaScript::GetGlobal -> Lua state is null");
        if (lua_getglobal(L, name) != LUA_TFUNCTION) {
            ATTOERROR("LuaScript::GetGlobal -> Could not get global %s", name);
            return false;
        }

        PushValues(args...);

        if (!CallFunction(sizeof...(args), 0)) {
            return false;
        }

        return true;
    }

    //template<typename _type_>
    //sbool LuaScript::GetGlobal(const char* name, _type_& value) {
    //s    if (std::is_arithmetic<_type_>::value) {
    //s        int result = lua_getglobal(L, name);
    //s        if (result == LUA_OK) {
    //s            value = (_type_)lua_tonumber(L, -1);
    //s            lua_pop(L, 1);
    //s            return true;
    //s        }
    //s        else {
    //s            ATTOERROR("LuaScript::GetGlobal -> Could not get global %s", name);
    //s            return false;
    //s        }
    //s    }
    //s    else {
    //s        
    //s    }
    //s}

}
