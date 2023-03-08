#pragma once

#include "AttoLib.h"
#include "AttoMath.h"
#include "AttoLua.h"
#include "AttoRendering.h"


#include <json/json.hpp>

struct nk_glfw;
struct nk_context;
struct nk_font_atlas;

namespace atto
{
    class Mutex
    {
    public:
        b8          Create();
        void        Destroy();
        b8          Lock();
        b8          TryLock();
        b8          Unlock();

        inline b8   IsValid() const { return handle != nullptr; }

    private:
        void* handle = nullptr;
    };

    class ScopedMutex
    {
    public:
        ScopedMutex() { Assert(mutex.Create() == true, "Could not create scoped mutex que ?"); }
        ~ScopedMutex() { mutex.Destroy(); }

        DISABLE_COPY_AND_MOVE(ScopedMutex)

        inline b8 Lock() { return mutex.Lock(); }
        inline b8 Unlock() { return mutex.Unlock(); }

    private:
        Mutex mutex;
    };

    enum AssetType {
        ASSET_TYPE_INVALID = 0,
        ASSET_TYPE_TEXTURE,
        ASSET_TYPE_AUDIO,
        ASSET_TYPE_FONT,
        ASSET_TYPE_SPRITE,
        ASSET_TYPE_TILESHEET,
        ASSET_TYPE_COUNT
    };

    struct AssetId {
        inline static AssetId Create(const char* str) {
            AssetId id;
            id.id = StringHash::Hash(str);
            return id; 
        }

        inline static AssetId Create(u32 idNumber) {
            AssetId id;
            id.id = idNumber;
            return id;
        }

        inline b8 IsValid() const { return id != 0; }

        inline b8 operator ==(const AssetId& other) const { return id == other.id; }
        inline b8 operator !=(const AssetId& other) const { return id != other.id; }

        u32 id;
    };

    template<u32 _type_>
    class TypedAssetId {
    public:
        inline static TypedAssetId<_type_> Create(const char* str) {
            TypedAssetId<_type_> id;
            id.id = StringHash::Hash(str);
            return id;
        }

        inline b8 IsValid() const { return id != 0; }
        inline u32 GetValue() const { return id; }
        inline AssetId ToRawId() const { return AssetId::Create(id); }

        inline b8 operator ==(const AssetId& other) const { return id == other.id; }
        inline b8 operator !=(const AssetId& other) const { return id != other.id; }

        u32 id;
    };

    typedef TypedAssetId<ASSET_TYPE_TEXTURE> TextureAssetId;
    typedef TypedAssetId<ASSET_TYPE_AUDIO>   AudioAssetId;
    typedef TypedAssetId<ASSET_TYPE_FONT>    FontAssetId;
    
    struct TextureAsset {
        u32         textureHandle;
        i32         width;
        i32         height;
        i32         channels;
        i32         wrapMode;
        bool        generateMipMaps;

        static TextureAsset CreateDefault() {
            TextureAsset textureAsset = {};
            textureAsset.wrapMode = 0x2901; // GL_REPEAT
            textureAsset.generateMipMaps = true;

            return textureAsset;
        }
    };

    struct AudioAsset {
        u32         bufferHandle;
        i32         channels;
        i32         sampleRate;
        i32         sizeBytes;
        i32         bitDepth;

        static AudioAsset CreateDefault() {
            return {};
        }
    };

    struct Speaker {
        u32         sourceHandle;
        i32         index;
    };

    enum SpriteOrigin {
        SPRITE_ORIGIN_BOTTOM_LEFT = 0,
        SPRITE_ORIGIN_CENTER = 1,
        SPRITE_ORIGIN_BOTTOM_CENTER = 2,
        SPRITE_ORIGIN_COUNT
    };

    struct SpriteAsset {
        AssetId                 id;

        // Texture stuffies
        glm::vec2               uv0;
        glm::vec2               uv1;
        TextureAssetId          textureId;
        const TextureAsset*     texture;

        // Animation stuffies
        bool                    animated;

        glm::vec2               frameSize;
        i32                     frameCount;
        SpriteOrigin            origin;

        inline static SpriteAsset CreateDefault() {
            SpriteAsset spriteAsset = {};
            spriteAsset.uv1 = glm::vec2(1, 1);
            spriteAsset.frameCount = 1;
            return spriteAsset;
        }
    };

    struct Glyph {
        glm::ivec2      size;       // Size of glyph
        glm::ivec2      bearing;    // Offset from baseline to left/top of glyph
        glm::vec2       uv0;        // Uv0
        glm::vec2       uv1;        // Uv1
        u32             advance;    // Horizontal offset to advance to next glyph
    };

    struct FontAsset {
        u32                     textureHandle;
        i32                     width;
        i32                     height;
        
        i32                     fontSize; // TODO: Think of a good way to set these things...
        FixedList<Glyph, 128>   glyphs;

        static FontAsset CreateDefault() {
            FontAsset fontAsset = {};
            fontAsset.fontSize = 18;
            return fontAsset;
        }
    };

    struct VertexBuffer {
        u32 vao;
        u32 vbo;
        i32 size;
        i32 stride;
    };

    struct VertexBufferIndexed {
        u32 vao;
        u32 vbo;
        u32 ibo;
    };

    struct ShaderUniform {
        SmallString name;
        i32 location;
    };

    struct ShaderProgram {
        u32                                 programHandle;
        FixedList<ShaderUniform, 16>        uniforms;
    };

    class PackedAssetFile {
    public:
        void        PutData(byte* data, i32 size);
        void        GetData(byte* data, i32 size);

        void        SerializeAsset(TextureAsset& textureAsset);
        void        SerializeAsset(AudioAsset& audioAsset);
        void        SerializeAsset(FontAsset& fontAsset);

        void        Reset();

        bool        Save(const char* name);
        bool        Load(const char* name);

        void        Finished();

        bool        isLoading = false;
        i32         currentOffset = 0;
        List<byte>  storedData;

        template<typename _type_>
        void Serialize(_type_& value) {
            if (isLoading) {
                Get(value);
            }
            else {
                Put(value);
            }
        }

        template<typename _type_>
        void Put(const _type_& data) {
            const i32 s = storedData.GetNum();
            const i32 size = sizeof(_type_);
            storedData.SetNum(s + size, true);
            std::memcpy(storedData.GetData() + s, (const void*)&data, size);
        }

        template<typename _type_>
        void Put(const List<_type_>& data) {
            Put(data.GetNum());
            PutData((byte*)data.GetData(), data.GetNum() * sizeof(_type_));
        }

        template<typename _type_, u32 c>
        void Put(const FixedList<_type_, c>& data) {
            Put(data.GetCount());
            PutData((byte*)data.GetData(), data.GetCount() * sizeof(_type_));
        }

        template<typename _type_>
        void Get(_type_& data) {
            const i32 size = sizeof(_type_);
            Assert(currentOffset + size <= storedData.GetNum(), "PackedAssetFile::Get: currentOffset + size > storedData.GetNum()");
            byte* p = storedData.GetData() + currentOffset;
            std::memcpy((void*)&data, p, size);
            currentOffset += size;
        }

        template<typename _type_>
        void        Get(List<_type_>& data) {
            i32 num = 0;
            Get(num);
            data.SetNum(num, true);
            GetData((byte*)data.GetData(), num * sizeof(_type_));
        }

        template<typename _type_, u32 c>
        void        Get(FixedList<_type_, c>& data) {
            u32 num = 0;
            Get(num);
            data.SetCount(num);
            GetData((byte*)data.GetData(), data.GetCount() * sizeof(_type_));
        }
    };
    
    enum class DirectoryChangeType {
        FILE_ADDED,
        FILE_REMOVED,
        FILE_MODIFIED,
    };

    struct DirectoryChange {
        DirectoryChangeType type;
        LargeString         path;
    };
    
    struct EngineAsset {
        AssetType               type = ASSET_TYPE_INVALID;
        AssetId                 id = {};
        LargeString             path = {};

        union {
            TextureAsset   texture;
            FontAsset      font;
            AudioAsset     audio;
        };
    };
    
    struct ShapeVertex {
        glm::vec2 position;
    };

    // Values matter here
    enum ShapeType {
        DRAW_SHAPE_TYPE_RECT = 0,
        DRAW_SHAPE_TYPE_CIRCLE = 1,
        DRAW_SHAPE_TYPE_RECT_ROUND = 2,
        DRAW_SHAPE_TYPE_RECT_POLY = 3,
    };

    struct DrawShapeCommand {
        ShapeType   type;
        glm::vec4   color;
        glm::mat4   projection;
        union {
            struct {
                glm::vec2   tr;
                glm::vec2   br;
                glm::vec2   bl;
                glm::vec2   tl;
            };
            struct {
                glm::vec2   center;
                f32         radius;
            };
            struct {
                FixedList<glm::vec2, 16> poly;
            };
        };
    };

    struct ShapeRenderingState {
        ShaderProgram                       program;
        VertexBuffer                        vertexBuffer;
        FixedList<DrawShapeCommand, 256>    commands;
    };

    struct UIRenderingState {
        nk_glfw* nkGlfw;
        nk_context* nkContext;
        nk_font_atlas* nkFontAtlas;
    };

    struct SpriteVertex {
        glm::vec2 position;
        glm::vec2 uv;
        glm::vec4 color;
    };

    struct SpriteRenderingState {
        glm::vec4           color;
        ShaderProgram       program;
        VertexBuffer        vertexBuffer;
    };

    enum FontHAlignment {
        FONT_HALIGN_LEFT,
        FONT_HALIGN_CENTER,
        FONT_HALIGN_RIGHT
    };

    enum FontVAlignment {
        FONT_VALIGN_BOTTOM,
        FONT_VALIGN_MIDDLE,
        FONT_VALIGN_TOP,
    };

    struct FontVertex {
        glm::vec2 position;
        glm::vec2 uv;
    };

    struct DrawEntryFont {
        LargeString         text;
        FontAsset*          font;
        FontHAlignment      hAlignment;
        FontVAlignment      vAlignment;
        glm::vec4           color;
        f32                 underlineThinkness;
        f32                 underlinePercent;
        f32                 textWidth;
        glm::vec2           pos;
        BoxBounds           bounds;
    };
    
    struct TextRenderingState {
        FontAsset *         font;
        FontHAlignment      hAlignment;
        FontVAlignment      vAlignment;
        f32                 underlineThinkness;
        f32                 underlinePercent;
        glm::vec4           color;
        ShaderProgram       program;
        VertexBuffer        vertexBuffer;
    };

    struct DebugLineVertex {
        glm::vec2 position;
        glm::vec4 color;
    };

    struct DebugRenderingState {
        ShaderProgram                       program;
        VertexBuffer                        vertexBufer;
        FixedList<DebugLineVertex, 2048>    lines;
        glm::vec4                           color;
    };

    enum VertexLayoutType {
        VERTEX_LAYOUT_TYPE_SHAPE,           // Vec2(POS)
        VERTEX_LAYOUT_TYPE_SPRITE,          // Vec2(POS), Vec2(UV), Vec4(COLOR)
        VERTEX_LAYOUT_TYPE_FONT,            // Vec2(POS), Vec2(UV)
        VERTEX_LAYOUT_TYPE_DEBUG_LINE,      // Vec2(POS), Vec4(COLOR)
    };

    struct GlobalRenderingState {
        ShaderProgram *                     program;
    };

    struct SpriteInstance {
        f32                     animationDuration;
        f32                     animationPlayhead;
    };

    struct EditorState {
        bool                            consoleOpen;
        f32                             consoleScroll;
        
        // @TODO: Thread safe queue
        ScopedMutex                     directoryLock;
        FixedList<DirectoryChange, 64>  directoryChanges;
    };

    struct EntityId {
        i32 index;
        i32 generation;

        inline bool operator ==(const EntityId& other) const {
            return index == other.index && generation == other.generation;
        }
        
        inline bool operator !=(const EntityId& other) const {
            return !(*this == other);
        }
    };

    static const EntityId ENTITY_ID_INVALID = { -1, -1 };

    struct EntitySprite {
        bool            active;
        SpriteAsset*    sprite;
        i32             currentFrameIndex;
        f32             animationDuration;
        f32             animationPlayhead;
    };

    enum UnitTargetType {
        UNIT_TARGET_TYPE_NONE = 0,
        UNIT_TARGET_TYPE_GROUND_POS,
        UNIT_TARGET_TYPE_UNIT,
    };

    struct UnitTarget {
        UnitTargetType type;
        union {
            glm::vec2   groundPos;
            EntityId    unitId;
        };
    };

    struct Unit {
        bool        active;
        bool        isSelected;
        bool        teamNumber;
        Circle      localColldier;
        UnitTarget  target;
        f32         timeToNextFire;
        f32         timeFiring;
        glm::vec2   steering;
        i32         health;
    };
    
    struct Blocker {
        bool                active;
        PolygonCollider     collider;
    };

    struct Entity {
        EntityId        id;
        glm::vec2       pos;
        glm::vec2       vel;
        f32             rotation;
        BoxBounds       localBoundingBox;
        EntitySprite    sprite1;
        EntitySprite    sprite2;
        Unit            unit;
        Blocker         blocker;
    };

    struct Map {
        i32                         mapWidth;
        i32                         mapHeight;
        i32                         tileWidth;
        i32                         tileHeight;
        i32                         tileHalfWidth;
        i32                         tileHalfHeight;
        
        FixedList<Entity, 1024>     groundTileEntities;
        FixedList<Entity, 1024>     blockerTileEntities;
        FixedList<Entity, 2048>     unitEntities;
    };

    class LeEngine {
    public:
        bool                                Initialize(AppState* app);
        void                                Update(AppState* app);
        void                                Render(AppState* app);
        void                                Shutdown();
        
        void                                MouseWheelCallback(f32 x, f32 y);

        f32                                 Random();
        f32                                 Random(f32 min, f32 max);
        i32                                 RandomInt(i32 min, i32 max);

        glm::vec2                           ScreenPosToWorldPos(glm::vec2 screenPixelPos);
        glm::vec2                           WorldPosToScreenPos(glm::vec2 worldPos);
        f32                                 WorldLengthToScreenLength(f32 worldLength);
        glm::vec2                           WorldDimensionToScreenDimension(glm::vec2 worldDim);
        glm::vec2                           GetMousePosWorldSpace();

        void                                MapCreate(Map* map, const char *mapData, i32 mapWidth, i32 mapHeight);
        Entity*                             MapCreateEntity();
        Entity*                             MapGetEntity(const EntityId &id);
        void                                MapDestroyEntity(Entity* entity);
        glm::vec2                           MapTilePosToWorldPos(Map *map, glm::vec2 tilePos);

        BoxBounds                           EntityGetBoundingBox(const Entity& entity);

        Circle                              UnitGetCollider(const Entity& unit);
        glm::vec2                           UnitSteerSeek(const Entity& unit, glm::vec2 target);
        glm::vec2                           UnitSteerFlee(Entity& unit);
        glm::vec2                           UnitSteerWander(Entity& entity);

        const void *                        LoadEngineAsset(AssetId id, AssetType type);

        PolygonCollider                     BlockerGetCollider(const Entity& entity);
        
        TextureAsset*                       GetTextureAsset(TextureAssetId id);
        TextureAsset*                       LoadTextureAsset(TextureAssetId id);
        void                                FreeTextureAsset(TextureAssetId id);

        FontAsset*                          LoadFontAsset(FontAssetId id);
        void                                FreeAudioAsset(AudioAssetId id);

        AudioAsset*                         LoadAudioAsset(AudioAssetId id);
        
        SpriteAsset*                        GetSpriteAsset(AssetId id);

        Speaker                             AudioPlay(AudioAssetId audioAssetId, bool looping = false, f32 volume = 1.0f);
        void                                AudioPause(Speaker speaker);
        void                                AudioStop(Speaker speaker);
        bool                                AudioIsSpeakerPlaying(Speaker speaker);
        bool                                AudioIsSpeakerAlive(Speaker speaker);

        void                                ShaderProgramBind(ShaderProgram* program);
        i32                                 ShaderProgramGetUniformLocation(const char* name);
        void                                ShaderProgramSetInt(const char* name, i32 value);
        void                                ShaderProgramSetSampler(const char* name, i32 value);
        void                                ShaderProgramSetTexture(i32 location, u32 textureHandle);
        void                                ShaderProgramSetFloat(const char* name, f32 value);
        void                                ShaderProgramSetVec2(const char* name, glm::vec2 value);
        void                                ShaderProgramSetVec3(const char* name, glm::vec3 value);
        void                                ShaderProgramSetVec4(const char* name, glm::vec4 value);
        void                                ShaderProgramSetMat3(const char* name, glm::mat3 value);
        void                                ShaderProgramSetMat4(const char* name, glm::mat4 value);

        void                                VertexBufferUpdate(VertexBuffer vertexBuffer, i32 offset, i32 size, const void* data);

        void                                DrawSurfaceResized(i32 w, i32 h);

        void                                DrawClearSurface(const glm::vec4& color = glm::vec4(0, 0, 0, 1));
        void                                DrawEnableAlphaBlending();
        
        DrawShapeCommand                    DrawShapeCreateCommand();
        void                                DrawShapeRect(glm::vec2 bl, glm::vec2 tr, const glm::vec4& color = glm::vec4(1, 1, 1, 1));
        void                                DrawShapeRectCenterDimRot(glm::vec2 center, glm::vec2 dim, f32 rot, const glm::vec4& color = glm::vec4(1, 1, 1, 1));
        void                                DrawShapeRectCenterDim(glm::vec2 center, glm::vec2 dim);
        void                                DrawShapeCircle(glm::vec2 center, f32 radius, const glm::vec4& color = glm::vec4(1, 1, 1, 1));
        void                                DrawShapeRoundRect(glm::vec2 bl, glm::vec2 tr, f32 radius = 10.0f);
        void                                DrawShapePolygon(const PolygonCollider& polygon, const glm::vec4& color = glm::vec4(1, 1, 1, 1));
        void                                DrawShapeAddCommand(const DrawShapeCommand& cmd);
        void                                DrawShapeClearCommands();
        void                                DrawShapeRender();

        void                                InitializeUIRendering(AppState* app);
        void                                ShutdownUIRendering(AppState* app);
        void                                DrawUINewFrame(AppState* app);
        void                                DrawUIRender(AppState * app);
        void                                DrawUIDisplayJson(nlohmann::json j);
        void                                DrawUIDemoJSON();
        void                                DrawUIDemoWindow();

        void                                DrawSpriteSetColor(glm::vec4 color);
        void                                DrawSprite(const AssetId& id, glm::vec2 pos, f32 rotation, i32 frameIndex);
        void                                DrawSprite(SpriteAsset* spriteAsset, glm::vec2 pos, f32 rotation, i32 frameIndex);

        void                                DrawTextSetFont(FontAssetId id);
        void                                DrawTextSetColor(glm::vec4 color);
        void                                DrawTextSetHalign(FontHAlignment hAlignment);
        void                                DrawTextSetValign(FontVAlignment vAlignment);
        f32                                 DrawTextWidth(const char* text);
        BoxBounds                           DrawTextBounds(const char* text);
        f32                                 DrawTextWidth(FontAsset* font, const char* text);
        BoxBounds                           DrawTextBounds(FontAsset *font, const char* text);
        DrawEntryFont                       DrawTextCreate(const char* text, glm::vec2 pos);
        void                                DrawText(const char* text, glm::vec2 pos);
        void                                DrawText(SmallString text, glm::vec2 pos);

        void                                DEBUGPushLine(glm::vec2 a, glm::vec2 b);
        void                                DEBUGPushRay(Ray2D ray);
        void                                DEBUGPushCircle(glm::vec2 pos, f32 radius);
        void                                DEBUGPushCircle(Circle circle);
        void                                DEBUGPushBox(BoxBounds bounds);
        void                                DEBUGSubmit();

        void                                EditorToggleConsole();

        glm::mat4                           screenProjection;
        glm::mat4                           cameraProjection;
        glm::mat4                           cameraView;

        glm::vec2                           cameraPos;
        f32                                 cameraZoom;

        i32                                 mainSurfaceWidth;
        i32                                 mainSurfaceHeight;

    private:
        void                                InitializeShapeRendering();
        void                                InitializeSpriteRendering();
        void                                InitializeTextRendering();
        void                                InitializeDebugRendering();
        void                                InitializeLuaBindings();

        void                                UpdateAssets();

        void                                Win32WatchDirectory(const char* directory);
        void                                Win32OnDirectoryChanged(const char* directory, DirectoryChangeType changeType);

        void                                RegisterAssets();
        bool                                LoadTextureAsset(const char* name, TextureAsset& textureAsset);
        bool                                LoadAudioAsset(const char* name, AudioAsset& audioAsset);
        bool                                LoadFontAsset(const char* name, FontAsset& fontAsset);
        bool                                LoadWAV(const char* filename, AudioAsset& audioAsset);
        bool                                LoadOGG(const char* filename, AudioAsset& audioAsset);

        VertexBuffer                        SubmitVertexBuffer(i32 sizeBytes, const void* data, VertexLayoutType layoutType, bool dyanmic);
        ShaderProgram                       SubmitShaderProgram(const char* vertexSource, const char* fragmentSource);
        u32                                 SubmitTextureR8B8G8A8(i32 width, i32 height, byte* data, i32 wrapMode, bool generateMipMaps);
        u32                                 SubmitAudioClip(i32 sizeBytes, byte* data, i32 channels, i32 bitDepth, i32 sampleRate);
        
        void                                ALCheckErrors();
        u32                                 ALGetFormat(u32 numChannels, u32 bitDepth);
        bool                                GLCheckShaderCompilationErrors(u32 shader);
        bool                                GLCheckShaderLinkErrors(u32 program);

        AppState*                           app;

        LuaScript                           luaEngine;

        LargeString                         basePathAssets;
        LargeString                         basePathSprites;
        LargeString                         basePathSounds;

        GlobalRenderingState                globalRenderingState;
        ShapeRenderingState                 shapeRenderingState;
        SpriteRenderingState                spriteRenderingState;
        TextRenderingState                  textRenderingState;
        DebugRenderingState                 debugRenderingState;
        UIRenderingState                    uiRenderingState;
        EditorState                         editorState;

        FixedList<EngineAsset, 2048>        engineAssets;      // These never get moved, so it's safe to store a pointer to them.
        FixedList<SpriteAsset, 2048>        registeredSprites; // These never get moved, so it's safe to store a pointer to them.

        FixedList<Speaker,       8>        speakers;

        // Make this game state
        Map                                 demoMap;
        Map*                                currentMap;
        bool                                isDragging;
        glm::vec2                           startingDrag;
        glm::vec2                           endingDrag;
        
    private:
        static i32 Lua_IdFromString(lua_State* L);
        
        static i32 Lua_AudioPlay(lua_State* L);
        
        static i32 Lua_DrawShapeRect(lua_State *L);
        static i32 Lua_DrawShapeRectCenterDim(lua_State *L);
        static i32 Lua_DrawShapeCircle(lua_State *L);
        static i32 Lua_DrawShapeRoundRect(lua_State* L);

        static i32 Lua_DrawSprite(lua_State* L);

    };
}
