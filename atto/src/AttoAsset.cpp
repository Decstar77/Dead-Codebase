#include "AttoAsset.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/std_image.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis/stb_vorbis.c"

#include <audio/AudioFile.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>

#include <al/alc.h>
#include <al/al.h>

#include <filesystem>
#include <random>

namespace atto {

    class DebugFunctionKey {
    public:
        bool value;
        KeyCode keyCode;

        DebugFunctionKey(KeyCode keyCode, bool value = false) {
            this->value = value;
            this->keyCode = keyCode;
            keys.Add(this);
        }

        inline static FixedList<DebugFunctionKey*, 32> keys;
    };

    const static DebugFunctionKey debugDrawBoundsAndColliders(KEY_CODE_F1);
    const static DebugFunctionKey debugDrawUnitRanges(KEY_CODE_F2);

    static void FindAllFiles(const char* path, const char* extension, List<LargeString> &files) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.path().extension() == extension) {
                files.Add( LargeString::FromLiteral( entry.path().string().c_str() ));
            }
        }
    }

    bool LeEngine::Initialize(AppState* appState) {
        app = appState;

        basePathAssets = LargeString::FromLiteral("assets/");
        basePathSprites = LargeString::FromLiteral("assets/sprites/");
        basePathSounds = LargeString::FromLiteral("assets/sounds/");

        cameraView = glm::mat4(1);
        cameraZoom = 1.0f;

        DrawSurfaceResized(app->windowWidth, app->windowHeight);

        RegisterAssets();

        InitializeLuaBindings();

        InitializeShapeRendering();
        InitializeUIRendering(app);
        InitializeSpriteRendering();
        InitializeTextRendering();
        InitializeDebugRendering();

        const char* mapData =
            "1111111111111111"  //1
            "1000000000000001"  //2
            "1000000000000001"  //3
            "1000000000000001"  //4
            "1000000000000001"  //5
            "1000111111110001"  //6
            "1000000000000001"  //7
            "1000000000000001"  //8
            "1000000000000001"  //9
            "1000000000000001"  //10
            "1000000000000001"  //11
            "1000000000000001"  //12
            "1000000000000001"  //13
            "1000000000000001"  //14
            "1000000000000001"  //15
            "1111111111111111"; //16

        MapCreate(&demoMap, mapData, 16, 16);
        currentMap = &demoMap;

        cameraPos = MapTilePosToWorldPos(currentMap, glm::vec2(4, 4));

        {
            Entity entity = {};
            //entity.pos = glm::vec2(mainSurfaceWidth/ 2, mainSurfaceHeight/2);
            //entity.unit.active = true;
            entity.unit.isSelected = true;
            entity.sprite1.active = true;
            entity.sprite1.sprite = GetSpriteAsset(AssetId::Create("tile_test"));
            //entities.Add(entity);
        }
        {
            for (int j = 0; j < 2; j++) {
                f32 x = (f32)12 * j - 12 * 2;
                f32 y = (f32)-6 * j - 6 * 6;
                
                for (int i = 0; i < 11; i++) {
                    //entity.pos = glm::vec2(mainSurfaceWidth / 2, mainSurfaceHeight / 2);
                    Entity* entity = MapCreateEntity();
                    entity->pos.x = x;
                    entity->pos.y = y;

                    x -= 12;
                    y -= 6;

                    entity->localBoundingBox.min = glm::vec2(-4, 0);
                    entity->localBoundingBox.max = glm::vec2(3, 14);
                    entity->unit.localColldier.rad = 4;
                    entity->unit.health = 100;
                    entity->unit.active = true;
                    entity->unit.isSelected = false;
                    entity->sprite1.active = true;
                    entity->sprite1.sprite = GetSpriteAsset(AssetId::Create("unit_basic_man"));

                    entity->sprite2.active = true;
                    entity->sprite2.sprite = GetSpriteAsset(AssetId::Create("unit_basic_man_selection"));
                }
            }
        }
        {
            for (int j = 0; j < 2; j++) {
                f32 x = 400 + 12.0f * j;
                f32 y = -200 + -6.0f * j;
                for (int i = 0; i < 5; i++) {
                    //entity.pos = glm::vec2(mainSurfaceWidth / 2, mainSurfaceHeight / 2);
                    Entity* entity = MapCreateEntity();
                    entity->pos.x = x;
                    entity->pos.y = y;

                    x -= 12;
                    y -= 6;

                    entity->localBoundingBox.min = glm::vec2(-4, 0);
                    entity->localBoundingBox.max = glm::vec2(3, 14);

                    entity->unit.active = true;
                    entity->unit.isSelected = false;
                    entity->unit.teamNumber = true;
                    entity->unit.localColldier.rad = 4;
                    entity->unit.health = 100;
                    entity->sprite1.active = true;
                    entity->sprite1.sprite = GetSpriteAsset(AssetId::Create("unit_basic_man_enemy"));
                }
            }
        }


        return true;
    }

    void LeEngine::Update(AppState* app) {
        ProfilerClock profilerClock("Update");

#if ATTO_DEBUG
        for (i32 i = 0; i < DebugFunctionKey::keys.GetCount(); i++) {
            if (IsKeyJustDown(app->input, DebugFunctionKey::keys[i]->keyCode)) {
                DebugFunctionKey::keys[i]->value = !DebugFunctionKey::keys[i]->value;
            }
        }
#endif

        DrawShapeClearCommands();

        if (app->input->keys[KEY_CODE_A]) {
            cameraPos.x -= 100.0f * app->deltaTime;
        }

        if (app->input->keys[KEY_CODE_D]) {
            cameraPos.x += 100.0f * app->deltaTime;
        }

        if (app->input->keys[KEY_CODE_W]) {
            cameraPos.y += 100.0f * app->deltaTime;
        }

        if (app->input->keys[KEY_CODE_S]) {
            cameraPos.y -= 100.0f * app->deltaTime;
        }

        if (app->input->keys[KEY_CODE_1]) {
            cameraZoom += app->deltaTime;
            DrawSurfaceResized(mainSurfaceWidth, mainSurfaceHeight);
        }
        
        if (app->input->keys[KEY_CODE_2]) {
            cameraZoom -= app->deltaTime;
            DrawSurfaceResized(mainSurfaceWidth, mainSurfaceHeight);
        }

        // Don't forget to update the camera view matrix !!
        cameraView = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraPos, 0.0f));

        if (IsMouseJustDown(app->input, MOUSE_BUTTON_LEFT)) {
            startingDrag = app->input->mousePosPixels;
        }

        if (IsMouseDown(app->input, MOUSE_BUTTON_LEFT) && startingDrag != app->input->mousePosPixels) {
            isDragging = true;
        }

        bool selectionBoxNow = false;
        bool selectionSingleNow = false;
        BoxBounds selectionBounds;
        if (isDragging) {
            endingDrag = app->input->mousePosPixels;
            const glm::vec2 bl = glm::min(startingDrag, endingDrag);
            const glm::vec2 tr = glm::max(startingDrag, endingDrag);
            const glm::vec4 color = glm::vec4(0.6f, 0.8f, 0.6f, 0.8f);

            DrawShapeRect(bl, tr, color);

            if (IsMouseJustUp(app->input, MOUSE_BUTTON_LEFT)) {
                isDragging = false;
                selectionBoxNow = true;
                glm::vec2 blWorld = ScreenPosToWorldPos(bl);
                glm::vec2 trWorld = ScreenPosToWorldPos(tr);

                selectionBounds.min = glm::min(blWorld, trWorld);
                selectionBounds.max = glm::max(blWorld, trWorld);
            }
        }
        else {
            if (IsMouseJustUp(app->input, MOUSE_BUTTON_LEFT)) {
                selectionSingleNow = true;
            }
        }

        const glm::vec2 mousePosWorldSpace = GetMousePosWorldSpace();

        const f32 roomWidth =  (f32)mainSurfaceWidth;
        const f32 roomHeight = (f32)mainSurfaceHeight;
        
        const f32 halfRoomWidth = roomWidth / 2.0f;
        const f32 halfRoomHeight = roomHeight / 2.0f;
        
        const i32 entityCapcity = currentMap->unitEntities.GetCapcity();
        for (i32 unitIndex = 0; unitIndex < entityCapcity; unitIndex++) {
            Entity& entity = currentMap->unitEntities[unitIndex];
            
            if (entity.id == ENTITY_ID_INVALID) {
                continue;
            }

            if (entity.unit.active) {
                const f32 moveDistance = 25.0f * app->deltaTime;
                const f32 firingRange = 25;
                const f32 fieldOfView = 50;
                const f32 fieldOfViewSqrd = fieldOfView * fieldOfView;

                if (entity.unit.teamNumber == 0) {
                    // Player updates
                    if (selectionBoxNow) {
                        entity.unit.isSelected = false;
                        const glm::vec2 entityPos = entity.pos;
                        const BoxBounds entityBounds = EntityGetBoundingBox(entity);
                        if (selectionBounds.Intersects(entityBounds)) {
                            entity.unit.isSelected = true;
                        }
                    }

                    if (selectionSingleNow) {
                        entity.unit.isSelected = false;
                        const BoxBounds entityBounds = EntityGetBoundingBox(entity);
                        if (entityBounds.Contains(mousePosWorldSpace)) {
                            entity.unit.isSelected = true;
                        }
                    }

                    if (IsMouseJustDown(app->input, MOUSE_BUTTON_RIGHT) && entity.unit.isSelected) {
                        bool isBasicMoveCommand = true;
                        for (i32 otherUnitIndex = 0; otherUnitIndex < entityCapcity; otherUnitIndex++) {
                            if (otherUnitIndex == unitIndex) {
                                continue;
                            }

                            Entity& otherUnit = currentMap->unitEntities[otherUnitIndex];

                            if (otherUnit.unit.teamNumber) {
                                const BoxBounds otherUnitBounds = EntityGetBoundingBox(otherUnit);
                                if (otherUnitBounds.Contains(mousePosWorldSpace)) {
                                    entity.unit.target.type = UNIT_TARGET_TYPE_UNIT;
                                    entity.unit.target.unitId = otherUnit.id;
                                    isBasicMoveCommand = false;
                                    break;
                                }
                            }
                        }

                        if (isBasicMoveCommand) {
                            entity.unit.target.type = UNIT_TARGET_TYPE_GROUND_POS;
                            entity.unit.target.groundPos = mousePosWorldSpace;
                        }
                    }
                }
                else {
                    // Strat AI Updates
                }

                entity.sprite1.currentFrameIndex = 0;

                entity.unit.timeToNextFire -= app->deltaTime;
                entity.unit.timeFiring -= app->deltaTime;

                entity.unit.timeToNextFire = glm::max(entity.unit.timeToNextFire, 0.0f);
                entity.unit.timeFiring = glm::max(entity.unit.timeFiring, 0.0f);

                if (entity.unit.target.type == UNIT_TARGET_TYPE_GROUND_POS) {
                    const glm::vec2 toTarget = entity.unit.target.groundPos - entity.pos;
                    const f32 distanceToTarget = glm::length(toTarget);
                    const glm::vec2 direction = toTarget / distanceToTarget;
                    
                    
                    if (!ApproxEqual(distanceToTarget, 0.0f)) {
                        if (direction.x > 0.0f) {
                            entity.sprite1.currentFrameIndex = 0;
                        }
                        else {
                            entity.sprite1.currentFrameIndex = 1;
                        }

                        if (moveDistance > distanceToTarget) {
                            entity.pos = entity.unit.target.groundPos;
                            entity.unit.target.type = UNIT_TARGET_TYPE_NONE;
                        }
                        else {
                            entity.pos += direction * moveDistance;
                        }
                    }
                }
                else if (entity.unit.target.type == UNIT_TARGET_TYPE_UNIT) {
                    if (Entity * otherEntity = MapGetEntity(entity.unit.target.unitId)) {
                        const glm::vec2 toTarget = otherEntity->pos - entity.pos;
                        const f32 distanceToTarget = glm::length(toTarget);
                        const glm::vec2 direction = toTarget / distanceToTarget;

                        if (distanceToTarget <= firingRange) {
                            if (otherEntity->pos.x > entity.pos.x) {
                                entity.sprite1.currentFrameIndex = 2;
                            }
                            else {
                                entity.sprite1.currentFrameIndex = 3;
                            }

                            if (entity.unit.timeToNextFire <= 0.0f) {
                                entity.unit.timeToNextFire = 1.0f;
                                AudioPlay(AudioAssetId::Create("assets/sounds/gun_pistol_shot_01"));
                                entity.unit.timeFiring = 0.2f;

                                if (otherEntity->unit.health > 0) {
                                    otherEntity->unit.health -= 50;
                                    if (otherEntity->unit.health <= 0) {
                                        otherEntity->sprite1.active = false;
                                        AudioPlay(AudioAssetId::Create("assets/sounds/basic_death_1"));
                                        entity.unit.target.type = UNIT_TARGET_TYPE_NONE;
                                        MapDestroyEntity(otherEntity);
                                    }
                                }
                            }

                            if (entity.unit.timeFiring > 0.0) {
                                if (otherEntity->pos.x > entity.pos.x) {
                                    entity.sprite1.currentFrameIndex = 5;
                                }
                                else {
                                    entity.sprite1.currentFrameIndex = 4;
                                }
                            }
                        }
                        else {
                            entity.pos += direction * moveDistance;
                        }
                    }
                    else {
                        entity.unit.target.type = UNIT_TARGET_TYPE_NONE;
                    }
                }
                else if (entity.unit.target.type == UNIT_TARGET_TYPE_NONE) {
                    f32 unitDistance = 999999.0f;
                    for (i32 otherUnitIndex = 0; otherUnitIndex < entityCapcity; otherUnitIndex++) {
                        if (otherUnitIndex == unitIndex) {
                            continue;
                        }

                        Entity& otherUnit = currentMap->unitEntities[otherUnitIndex];

                        if (entity.unit.teamNumber != otherUnit.unit.teamNumber) {
                            f32 d = glm::distance2(entity.pos, otherUnit.pos);
                            if (d < fieldOfViewSqrd && d < unitDistance) {
                                unitDistance = d;
                                entity.unit.target.type = UNIT_TARGET_TYPE_UNIT;
                                entity.unit.target.unitId = otherUnit.id;
                            }
                        }
                    }
                }

                for (i32 otherUnitIndex = 0; otherUnitIndex < entityCapcity; otherUnitIndex++) {
                    if (otherUnitIndex == unitIndex) {
                        continue;
                    }

                    Entity& otherUnit = currentMap->unitEntities[otherUnitIndex];

                    if (otherUnit.unit.active) {
                        const Circle currentUnitCollider = UnitGetCollider(entity);
                        const Circle otherUnitCollider = UnitGetCollider(otherUnit);

                        Manifold manifold = {};
                        if (otherUnitCollider.Collision(currentUnitCollider, manifold)) {
                            entity.pos += manifold.normal * manifold.penetration;
                        }
                    }
                }

                const i32 blockerCapcity = currentMap->blockerTileEntities.GetCapcity();
                for (i32 i = 0; i < blockerCapcity; i++) {
                    Entity& blocker = currentMap->blockerTileEntities[i];
                    if (blocker.blocker.active) {
                        const Circle currentUnitCollider = UnitGetCollider(entity);
                        const PolygonCollider blockerCollider = BlockerGetCollider(blocker);

                        Manifold manifold = {};
                        if (CollisionTests::CirclePoly(currentUnitCollider, blockerCollider, manifold)) {
                            entity.pos -= manifold.normal * manifold.penetration;
                        }
                    }
                }

                if (debugDrawBoundsAndColliders.value) {
                    const BoxBounds bounds = EntityGetBoundingBox(entity);
                    const Circle collider = UnitGetCollider(entity);
                    DrawShapeRect(WorldPosToScreenPos(bounds.min), WorldPosToScreenPos(bounds.max), glm::vec4(0.5f));
                    DrawShapeCircle(WorldPosToScreenPos(collider.pos), WorldLengthToScreenLength(collider.rad), glm::vec4(0.5f));
                }
                if (debugDrawUnitRanges.value) {
                    DrawShapeCircle(WorldPosToScreenPos(entity.pos), WorldLengthToScreenLength(firingRange), glm::vec4(1.0f, 0.4f, 0.4f, 0.5f) * 0.1f);
                    DrawShapeCircle(WorldPosToScreenPos(entity.pos), WorldLengthToScreenLength(fieldOfView), glm::vec4(0.4f, 1.0f, 0.4f, 0.5f) * 0.1f);
                }
            }
        }

        if (debugDrawBoundsAndColliders.value) {
            const i32 blockerCapcity = currentMap->blockerTileEntities.GetCapcity();
            for (i32 blockerIndex = 0; blockerIndex < blockerCapcity; blockerIndex++) {
                Entity& blocker = currentMap->blockerTileEntities[blockerIndex];
                if (blocker.blocker.active) {
                    PolygonCollider collider = blocker.blocker.collider;
                    collider.Translate(blocker.pos);
                    DrawShapePolygon(collider, glm::vec4(1.0f, 0.4f, 0.4f, 0.5f) * 1.1f);
                }
            }
        }

        //DrawShapeCircle(glm::vec2(200, 200), 50);

        //luaEngine.SetGlobal("dt", app->deltaTime);
        //luaEngine.SetGlobal("key_a", app->input->keys[KEY_CODE_A]);
        //luaEngine.SetGlobal("key_d", app->input->keys[KEY_CODE_D]);
        //luaEngine.CallVoidFunction("update");

    }

    void LeEngine::Render(AppState* app) {
        //ProfilerClock profilerClock("Render");

        DrawClearSurface();
        DrawEnableAlphaBlending();
        //DrawSprite(AssetId::Creaste("starfield_02"), glm::vec2(0, 0), 0, 0);

        const i32 entityCapcity = currentMap->groundTileEntities.GetCapcity();
        for (i32 entityIndex = 0; entityIndex < entityCapcity; entityIndex++) {
            Entity& entity = currentMap->groundTileEntities[entityIndex];
            if (entity.sprite1.active) {
                DrawSprite(entity.sprite1.sprite, entity.pos, entity.rotation, 0);
            }
        }

        const i32 blockerCapcity = currentMap->blockerTileEntities.GetCapcity();
        for (i32 entityIndex = 0; entityIndex < entityCapcity; entityIndex++) {
            Entity& entity = currentMap->blockerTileEntities[entityIndex];
            if (entity.sprite1.active) {
                DrawSprite(entity.sprite1.sprite, entity.pos, entity.rotation, 0);
            }
        }

        const i32 unitEntityCapcity = currentMap->unitEntities.GetCapcity();
        for (i32 entityIndex = 0; entityIndex < unitEntityCapcity; entityIndex++) {
            Entity& entity = currentMap->unitEntities[entityIndex];
            if (entity.unit.active && entity.unit.isSelected) {
                if (entity.sprite2.active) {
                    DrawSprite(entity.sprite2.sprite, entity.pos, entity.rotation, 0);
                }
            }

            if (entity.sprite1.active) {
                DrawSprite(entity.sprite1.sprite, entity.pos, entity.rotation, entity.sprite1.currentFrameIndex);
            }
        }

        //for (i32 y = 0; y < currentMap->mapHeight; y++) {
        //    for (i32 x = 0; x < currentMap->mapWidth; x++) {
        //        glm::vec2 tilePos = glm::vec2(x, y);
        //        glm::vec2 worldPos = MapTilePosToWorldPos(currentMap, tilePos);
        //        glm::vec2 screenPos = WorldPosToScreenPos(worldPos);
        //        SmallString text = StringFormat::Small("%d,%d", x, y);
        //        DrawTextSetHalign(FONT_HALIGN_CENTER);
        //        DrawText(text, screenPos);
        //    }
        //}

        //glm::vec2 mousePos = app->input->mousePosPixels;
        glm::vec2 pos = GetMousePosWorldSpace();
        
        DrawUINewFrame(app);
        //DrawUIDemoJSON();
        //DrawUIDemoWindow();
        
        //PolygonCollider p = {};
        //p.vertices.Add(  glm::vec2(-16, -9) );
        //p.vertices.Add(  glm::vec2(0, -16) );
        //p.vertices.Add(  glm::vec2(16, -9) );
        //p.vertices.Add(  glm::vec2(16, 9) );
        //p.vertices.Add(  glm::vec2(0, 16) );
        //p.vertices.Add(  glm::vec2(-16, 9) );
        //DrawShapePolygon(p);

        static f32 r = 0;
        r += app->deltaTime * 2.0f;
        //DrawSprite(AssetId::Create("ship_b"), pos, r, 0);

        DrawShapeRender();
        DrawUIRender(app);
        
        DEBUGPushLine(glm::vec2(0, 0), glm::vec2(100, 0));
        DEBUGSubmit();
    }

    void LeEngine::Shutdown() {
        ShutdownUIRendering(app);
    }

    void LeEngine::MouseWheelCallback(f32 x, f32 y) {
        //cameraZoom += y * 0.1f;
        //DrawSurfaceResized(mainSurfaceWidth, mainSurfaceHeight);
        //static f32 zoom = 0;
        //zoom -= (i32)y * 150;
        //
        //f32 aspect = 1.777777776f;
        //
        //
        //i32 h = 256 + zoom;
        //i32 w = (i32)((f32)h * aspect);
        //
        //f32 right = (f32)w / 2.0f;
        //f32 left = -right;
        //
        //f32 top = (f32)h / 2.0f;
        //f32 bottom = -top;
        //
        ////glm::mat4 view = glm::inverse( glm::translate(glm::mat4(1), glm::vec3(100, 0, 200 + zoom)) );
        ////projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10000.0f) * view;
        //glm::mat4 view = glm::inverse(glm::translate(glm::mat4(1), glm::vec3(100, 0, 0)));
        //projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f) * view;
        //mainSurfaceWidth = w;
        //mainSurfaceHeight = h;
    }

    f32 LeEngine::Random() {
        return Random(0.0f, 1.0f);
    }

    f32 LeEngine::Random(f32 min, f32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<f32> dis(min, max);
        return dis(gen);
    }

    i32 LeEngine::RandomInt(i32 min, i32 max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<i32> dis(min, max);
        return dis(gen);
    }

    glm::vec2 LeEngine::ScreenPosToWorldPos(glm::vec2 screenPixelPos) {
        glm::mat4 ip = glm::inverse(cameraProjection);
        glm::mat4 iv = glm::inverse(cameraView);

        f32 x = (screenPixelPos.x / (f32)mainSurfaceWidth) * 2.0f - 1.0f;
        f32 y = -((screenPixelPos.y / (f32)mainSurfaceHeight) * 2.0f - 1.0f);

        glm::vec4 mousePosClipSpace = glm::vec4(x, y, 0, 1);
        glm::vec4 mousePosEyeSpace = ip * mousePosClipSpace;
        glm::vec4 mousePosWorldSpace = iv * mousePosEyeSpace;

        glm::vec2 pos = glm::vec2(mousePosWorldSpace.x, mousePosWorldSpace.y);

        return pos;
    }

    glm::vec2 LeEngine::WorldPosToScreenPos(glm::vec2 worldPos) {
        glm::vec4 clipSpacePos = cameraProjection * cameraView * glm::vec4(worldPos, 0.0f, 1.0f);

        // Divide by w component to get normalized device coordinates
        glm::vec3 ndc = glm::vec3(clipSpacePos) / clipSpacePos.w;

        // Map NDC to pixel coordinates
        glm::vec2 screenPos;
        screenPos.x = ((ndc.x + 1.0f) / 2.0f) * mainSurfaceWidth;
        screenPos.y = ((1.0f - ndc.y) / 2.0f) * mainSurfaceHeight;
        
        return screenPos;
    }

    f32 LeEngine::WorldLengthToScreenLength(f32 worldLength) {
        glm::vec4 clipSpacePos = cameraProjection * glm::vec4(worldLength, 0.0f, 0.0f, 1.0f);

        f32 l = ((clipSpacePos.x + 1.0f) / 2.0f) * mainSurfaceWidth - mainSurfaceWidth / 2.0f;

        return l;
    }

    glm::vec2 LeEngine::WorldDimensionToScreenDimension(glm::vec2 worldDim) {
        glm::vec4 clipSpacePos = cameraProjection * glm::vec4(worldDim, 0.0f, 1.0f);
        
        f32 x = ((clipSpacePos.x + 1.0f) / 2.0f) * mainSurfaceWidth - mainSurfaceWidth / 2.0f;
        f32 y = ((clipSpacePos.y + 1.0f) / 2.0f) * mainSurfaceHeight - mainSurfaceHeight / 2.0f;
        
        return glm::vec2(x, y);
    }

    glm::vec2 LeEngine::GetMousePosWorldSpace() {
        return ScreenPosToWorldPos(app->input->mousePosPixels);
    }

    Circle LeEngine::UnitGetCollider(const Entity& unit) {
        Circle collider = unit.unit.localColldier;
        collider.pos += unit.pos;
        return collider;
    }

    glm::vec2 LeEngine::UnitSteerSeek(const Entity& unitEntity, glm::vec2 target) {
        const f32 maxSpeed = 50.0f;
        
        const f32 slowDownInnerRad = 2.0f;
        const f32 slowDownOuterRad = 14.0f;

        glm::vec2 desiredVel = target - unitEntity.pos;
        const f32 distance = glm::length(desiredVel);

        glm::vec2 desiredDir = (desiredVel / distance);
        f32 speed = glm::clamp(glm::abs(distance - slowDownInnerRad) / slowDownOuterRad, 0.0f, 1.0f) * maxSpeed;
        desiredVel = (desiredVel / distance) * speed;

        DEBUGPushCircle(target, slowDownInnerRad);
        DEBUGPushCircle(target, slowDownOuterRad);

        return desiredVel - unitEntity.vel;
    }

    glm::vec2 LeEngine::UnitSteerWander(Entity& entity) {
        if (glm::length2(entity.vel) < 0.0001f) {
            entity.vel = glm::vec2(0, 1);
        }

        glm::vec2 circleCenter = glm::normalize(entity.vel) * 50.0f;

        static f32 wanderAngle = glm::radians(90.0f);
        glm::vec2 displacement;
        displacement.x = glm::cos(wanderAngle) * 25.0f;
        displacement.y = glm::sin(wanderAngle) * 25.0f;
        

        static f32 angleArc = glm::radians(30.0f);
        wanderAngle += Random() * angleArc - angleArc * 0.5f;

        return circleCenter + displacement;
    }

    BoxBounds LeEngine::EntityGetBoundingBox(const Entity& entity) {
        BoxBounds bounds = entity.localBoundingBox;
        bounds.Translate(entity.pos);
        return bounds;
    }

    void LeEngine::MapCreate(Map* map, const char* mapData, i32 mapWidth, i32 mapHeight) {
        map->mapWidth = mapWidth;
        map->mapHeight = mapHeight;
        map->tileWidth = 32;
        map->tileHeight = 16;
        map->tileHalfWidth = map->tileWidth / 2;
        map->tileHalfHeight = map->tileHeight / 2;

        for (i32 y = 0; y < map->mapHeight; y++) {
            for (i32 x = 0; x < map->mapWidth; x++) {
                i32 index = y * map->mapWidth + x;

                Entity entity = {};
                entity.pos = MapTilePosToWorldPos(map, glm::vec2(x, y));
                entity.sprite1.active = true;
                entity.sprite1.sprite = GetSpriteAsset(AssetId::Create("tile_test"));

                map->groundTileEntities.Add(entity);
            }
        }
        
        for (i32 y = 0; y < map->mapHeight; y++) {
            for (i32 x = 0; x < map->mapWidth; x++) {
                i32 index = y * map->mapWidth + x;
                
                char tileType = mapData[index];
                if (tileType == '1') {
                    Entity entity = {};
                    entity.pos = MapTilePosToWorldPos(map, glm::vec2(x, y));

                    entity.blocker.active = true;
                    entity.blocker.collider.vertices.Add(glm::vec2(-16, -9));
                    entity.blocker.collider.vertices.Add(glm::vec2(0, -16));
                    entity.blocker.collider.vertices.Add(glm::vec2(16, -9));
                    entity.blocker.collider.vertices.Add(glm::vec2(16, 7));
                    entity.blocker.collider.vertices.Add(glm::vec2(0, 15));
                    entity.blocker.collider.vertices.Add(glm::vec2(-16, 7));

                    entity.sprite1.active = true;
                    entity.sprite1.sprite = GetSpriteAsset(AssetId::Create("tile_blocker"));

                    map->blockerTileEntities.Add(entity);
                }
            }
        }

        const i32 entityCapcity = map->unitEntities.GetCapcity();
        for (i32 entityIndex = 0; entityIndex < entityCapcity; entityIndex++) {
            Entity &entity = map->unitEntities[entityIndex];
            entity.id = ENTITY_ID_INVALID;
        }
    }

    Entity* LeEngine::MapCreateEntity() {
        const i32 entityCapcity = currentMap->unitEntities.GetCapcity();
        for (i32 entityIndex = 0; entityIndex < entityCapcity; entityIndex++) {
            Entity& entity = currentMap->unitEntities[entityIndex];
            if (entity.id == ENTITY_ID_INVALID) {
                entity.id.generation = 1;
                entity.id.index = entityIndex;

                return &entity;
            }
        }

        Assert(0, "not sure if theis is allowed");

        return nullptr;
    }

    Entity* LeEngine::MapGetEntity(const EntityId& id) {
        if (id != ENTITY_ID_INVALID) {
            const EntityId& otherId = currentMap->unitEntities[id.index].id;
            if (otherId == id) {
                return &currentMap->unitEntities[id.index];
            }
        }
        return nullptr;
    }

    void LeEngine::MapDestroyEntity(Entity* entity) {
        if (entity != nullptr) {
            if (entity->id != ENTITY_ID_INVALID) {
                Entity* placedEntity = &currentMap->unitEntities[entity->id.index];
                if (entity->id == placedEntity->id) {
                    *placedEntity = {};
                    placedEntity->id = ENTITY_ID_INVALID;
                }
            }
        }
    }

    glm::vec2 LeEngine::MapTilePosToWorldPos(Map* map, glm::vec2 tilePos) {
        glm::vec2 world;
        world.x = (tilePos.x - tilePos.y) *  (f32)map->tileHalfWidth;
        world.y = -((tilePos.x + tilePos.y) * (f32)map->tileHalfHeight);
        return world;
    }

    const void* LeEngine::LoadEngineAsset(AssetId id, AssetType type) {
        const i32 count = engineAssets.GetCount();
        for (i32 assetIndex = 0; assetIndex < count; ++assetIndex) {
            EngineAsset& asset = engineAssets[assetIndex];

            if (!asset.id.IsValid()) {
                break;
            }

            if (asset.id == id && asset.type == type) {
                switch (asset.type) {
                    case ASSET_TYPE_TEXTURE: {
                        if (asset.texture.textureHandle != 0) {
                            return &asset.texture;
                        }
                        
                        if (LoadTextureAsset(asset.path.GetCStr(), asset.texture)) {
                            ATTOTRACE("Loaded texture asset %s", asset.path.GetCStr());
                            return &asset.texture;
                        }
                    } break;
                    
                    case ASSET_TYPE_AUDIO: {
                        if (asset.audio.bufferHandle != 0) {
                            return &asset.audio;
                        }

                        if (LoadAudioAsset(asset.path.GetCStr(), asset.audio)) {
                            ATTOTRACE("Loaded audio sasset %s", asset.path.GetCStr());
                            return &asset.audio;
                        }
                    } break;

                    case ASSET_TYPE_FONT: {
                        if (asset.font.textureHandle != 0) {
                            return &asset.font;
                        }

                        if (LoadFontAsset(asset.path.GetCStr(), asset.font)) {
                            ATTOTRACE("Loaded font asset %s", asset.path.GetCStr());
                            return &asset.font;
                        }
                    } break;

                    default: {
                        Assert(0, "");
                    }
                }
            }
        }

        return nullptr;
    }

    PolygonCollider LeEngine::BlockerGetCollider(const Entity& entity) {
        PolygonCollider collider = entity.blocker.collider;
        collider.Translate(entity.pos);
        
        return collider;
    }

    TextureAsset* LeEngine::LoadTextureAsset(TextureAssetId id) {
        return (TextureAsset*)LoadEngineAsset(id.ToRawId(), ASSET_TYPE_TEXTURE);
    }
    
    FontAsset* LeEngine::LoadFontAsset(FontAssetId id) {
        return (FontAsset*)LoadEngineAsset(id.ToRawId(), ASSET_TYPE_FONT);
    }

    AudioAsset* LeEngine::LoadAudioAsset(AudioAssetId id) {
        return (AudioAsset*)LoadEngineAsset(id.ToRawId(), ASSET_TYPE_AUDIO);
    }

    Speaker LeEngine::AudioPlay(AudioAssetId audioAssetId, bool looping, f32 volume /*= 1.0f*/) {
        const AudioAsset* audioAsset = LoadAudioAsset(audioAssetId);
        if (!audioAsset) {
            ATTOERROR("Could not load audio asset");
            return {};
        }

        const i32 speakerCount = speakers.GetCount();
        for (i32 speakerIndex = 0; speakerIndex < speakerCount; ++speakerIndex) {
            Speaker& speaker = speakers[speakerIndex];
            if (speaker.sourceHandle == 0) {
                continue;
            }

            ALint state = {};
            alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED) {
                alSourcei(speaker.sourceHandle, AL_BUFFER, audioAsset->bufferHandle);
                alSourcei(speaker.sourceHandle, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
                alSourcef(speaker.sourceHandle, AL_GAIN, volume);
                alSourcePlay(speaker.sourceHandle);
                ALCheckErrors();

                return speaker;
            }
        }

        Speaker speaker = {};
        
        if (speakers.GetCount() < speakers.GetCapcity()) {
            speaker.index = speakers.GetCount();
            alGenSources(1, &speaker.sourceHandle);
            alSourcei(speaker.sourceHandle, AL_BUFFER, audioAsset->bufferHandle);
            alSourcei(speaker.sourceHandle, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
            alSourcef(speaker.sourceHandle, AL_GAIN, volume);
            alSourcePlay(speaker.sourceHandle);
            ALCheckErrors();
            speakers.Add(speaker);
        }
        else {
        }

        return speaker;
    }

    void LeEngine::AudioPause(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            alSourcePause(speaker.sourceHandle);
        }
    }

    void LeEngine::AudioStop(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            alSourceStop(speaker.sourceHandle);
        }
    }

    bool LeEngine::AudioIsSpeakerPlaying(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            ALint state = {};
            alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
            return  state == AL_PLAYING;
        }

        return false;
    }

    bool LeEngine::AudioIsSpeakerAlive(Speaker speaker) {
        if (speaker.sourceHandle != 0) {
            ALint state = {};
            alGetSourcei(speaker.sourceHandle, AL_SOURCE_STATE, &state);
            return state != AL_STOPPED;
        }

        return false;
    }

    SpriteAsset* LeEngine::GetSpriteAsset(AssetId id) {
        const i32 spriteCount = registeredSprites.GetCount();
        for (i32 i = 0; i < spriteCount; i++) {
            if (registeredSprites[i].id == id) {
                return &registeredSprites[i];
            }
        }

        return nullptr;
    }

    void LeEngine::ShaderProgramBind(ShaderProgram* program) {
        Assert(program->programHandle != 0, "Shader program not created");
        globalRenderingState.program = program;
        glUseProgram(program->programHandle);
    }

    i32 LeEngine::ShaderProgramGetUniformLocation(const char* name) {
        ShaderProgram* program = globalRenderingState.program;
        if (program == nullptr) {
            ATTOERROR("Shader progam in not bound");
            return -1;
        }

        if (program->programHandle == 0) {
            ATTOERROR("Shader progam in not valid");
            return -1;
        }

        const u32 uniformCount = program->uniforms.GetCount();
        for (u32 uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++) {
            ShaderUniform& uniform = program->uniforms[uniformIndex];
            if (uniform.name == name) {
                return uniform.location;
            }
        }

        i32 location = glGetUniformLocation(program->programHandle, name);
        if (location >= 0) {
            ShaderUniform newUniform = {};
            newUniform.location = location;
            newUniform.name = name;

            program->uniforms.Add(newUniform);
        }
        else {
            ATTOERROR("Could not find uniform value %s", name);
        }

        return location;
    }

    void LeEngine::ShaderProgramSetInt(const char* name, i32 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniform1i(location, value);
        }
    }

    void LeEngine::ShaderProgramSetSampler(const char* name, i32 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniform1i(location, value);
        }
    }

    void LeEngine::ShaderProgramSetTexture(i32 location, u32 textureHandle) {
        glBindTextureUnit(0, textureHandle);
    }

    void LeEngine::ShaderProgramSetVec4(const char* name, glm::vec4 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniform4fv(location, 1, glm::value_ptr(value));
        }
    }

    void LeEngine::ShaderProgramSetMat4(const char* name, glm::mat4 value) {
        i32 location = ShaderProgramGetUniformLocation(name);
        if (location >= 0) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void LeEngine::VertexBufferUpdate(VertexBuffer vertexBuffer, i32 offset, i32 size, const void* data) {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void LeEngine::DrawSurfaceResized(i32 w, i32 h) {

        // TODO: I'm not sure on how to handle dynamic resolutions...
#if 0
        // IDK
        f32 aspect = (f32)w / (f32)h;
        mainSurfaceHeight = (glm::abs(720 - h) < glm::abs(1080 - h)) ? 720 : 1080;
        mainSurfaceWidth = (i32)((f32)h * aspect);
        glViewport(0, 0, mainSurfaceWidth, mainSurfaceHeight);
#elif 0
        // Widen the camera ???
        mainSurfaceWidth  = (i32)((f32)w * 1.0f);
        mainSurfaceHeight = (i32)((f32)h * 1.0f);
        glViewport(0, 0, mainSurfaceWidth, mainSurfaceHeight);
        
        cameraProjection = glm::ortho(0.0f, (f32)w, 0.0f, (f32)h, -1.0f, 1.0f);
#elif 0
        // Stretch
        mainSurfaceWidth = 1280;
        mainSurfaceHeight = 720;
        glViewport(0, 0, w, h);
#elif 0
        // Maintain aspect ratio with black bars
        mainSurfaceWidth = (i32)(1280.0 * 1.6f);
        mainSurfaceHeight = (i32)(720.0 * 1.6f);
        //mainSurfaceWidth = 480;
        //mainSurfaceHeight = 360;

        f32 ratioX = (f32)w / (f32)mainSurfaceWidth;
        f32 ratioY = (f32)h / (f32)mainSurfaceHeight;
        f32 ratio = ratioX < ratioY ? ratioX : ratioY;
        
        i32 viewWidth = (i32)(mainSurfaceWidth * ratio);
        i32 viewHeight = (i32)(mainSurfaceHeight * ratio);
        
        i32 viewX = (i32)((w - mainSurfaceWidth * ratio) / 2);
        i32 viewY = (i32)((h - mainSurfaceHeight * ratio) / 2);

        glViewport(viewX, viewY, viewWidth, viewHeight);
#else 
        mainSurfaceWidth = (i32)((f32)w * 1.0f);
        mainSurfaceHeight = (i32)((f32)h * 1.0f);

        glViewport(0, 0, mainSurfaceWidth, mainSurfaceHeight);

        f32 right = (f32)mainSurfaceWidth / 2.0f;
        f32 left = -right;
        
        f32 top = (f32)mainSurfaceHeight / 2.0f;
        f32 bottom = -top;
        
        f32 n = 256 * cameraZoom;

        f32 aspectRatio = (f32)mainSurfaceWidth / (f32)mainSurfaceHeight;
        left = - n * 0.5f * aspectRatio;
        right = n * 0.5f * aspectRatio;
        bottom = -n * 0.5f;
        top = n * 0.5f;
        
        left = roundf(left);
        right = roundf(right);
        bottom = roundf(bottom);
        top = roundf(top);

        cameraProjection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
#endif
        //cameraProjection = glm::ortho(left * cameraZoom, right * cameraZoom, bottom * cameraZoom, top * cameraZoom, -1.0f, 1.0f);
        ATTOTRACE("Main surface resized to %d x %d", mainSurfaceWidth, mainSurfaceHeight);

        screenProjection = glm::ortho(0.0f, (f32)mainSurfaceWidth, (f32)mainSurfaceHeight, 0.0f, -1.0f, 1.0f);
    }

    void LeEngine::DrawClearSurface(const glm::vec4& color) {
        //glClearColor(color.r, color.g, color.b, color.a);
        //glClearColor(0.5f, 0.5f, 0.5f, 1);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void LeEngine::DrawEnableAlphaBlending() {
        glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        //glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    DrawShapeCommand LeEngine::DrawShapeCreateCommand() {
        DrawShapeCommand cmd = {};
        cmd.projection = screenProjection;
        cmd.color = glm::vec4(1, 1, 1, 1);
        return cmd;
    }

    void LeEngine::DrawShapeRect(glm::vec2 bl, glm::vec2 tr, const glm::vec4& color) {
        DrawShapeCommand cmd = DrawShapeCreateCommand();
        cmd.type = DRAW_SHAPE_TYPE_RECT;
        cmd.bl = bl;
        cmd.br = glm::vec2(tr.x, bl.y);
        cmd.tr = tr;
        cmd.tl = glm::vec2(bl.x, tr.y);
        cmd.color = color;
        DrawShapeAddCommand(cmd);
    }

    void LeEngine::DrawShapeRectCenterDimRot(glm::vec2 center, glm::vec2 dim, f32 rot, const glm::vec4& color /*= glm::vec4(1, 1, 1, 1)*/) {
        DrawShapeCommand cmd = DrawShapeCreateCommand();
        cmd.type = DRAW_SHAPE_TYPE_RECT;
        cmd.bl = -dim / 2.0f;
        cmd.tr = dim / 2.0f;
        cmd.br = glm::vec2(cmd.tr.x, cmd.bl.y);
        cmd.tl = glm::vec2(cmd.bl.x, cmd.tr.y);
        
        glm::mat2 rotationMatrix = glm::mat2(cos(rot), -sin(rot), sin(rot), cos(rot));
        cmd.bl = rotationMatrix * cmd.bl;
        cmd.tr = rotationMatrix * cmd.tr;
        cmd.br = rotationMatrix * cmd.br;
        cmd.tl = rotationMatrix * cmd.tl;

        cmd.bl += center;
        cmd.tr += center;
        cmd.br += center;
        cmd.tl += center;

        cmd.color = color;

        DrawShapeAddCommand(cmd);
    }

    void LeEngine::DrawShapeRectCenterDim(glm::vec2 center, glm::vec2 dim) {
        glm::vec2 bottomLeft = center - dim / 2.0f;
        glm::vec2 topRight = center + dim / 2.0f;
        DrawShapeRect(bottomLeft, topRight);
    }

    void LeEngine::DrawShapeCircle(glm::vec2 center, f32 radius, const glm::vec4& color) {
        DrawShapeCommand cmd = DrawShapeCreateCommand();
        cmd.type = DRAW_SHAPE_TYPE_CIRCLE;
        cmd.center = center;
        cmd.radius = radius;
        cmd.color = color;
        DrawShapeAddCommand(cmd);
    }

    void LeEngine::DrawShapeRoundRect(glm::vec2 bl, glm::vec2 tr, f32 radius) {
        DrawShapeCommand cmd = DrawShapeCreateCommand();
        cmd.type = DRAW_SHAPE_TYPE_RECT_ROUND;
        cmd.bl = bl;
        cmd.tr = tr;
        cmd.radius = radius;
        DrawShapeAddCommand(cmd);
    }

    void LeEngine::DrawShapePolygon(const PolygonCollider& polygon, const glm::vec4& color ) {
        DrawShapeCommand cmd = DrawShapeCreateCommand();
        cmd.type = DRAW_SHAPE_TYPE_RECT_POLY;

        const i32 polyCount = Geometry::Triangulate(polygon, cmd.poly.GetData(), cmd.poly.GetCapcity());
        cmd.poly.SetCount(polyCount);
        for (i32 i = 0; i < cmd.poly.GetCount(); i++) {
            cmd.poly[i] = WorldPosToScreenPos(cmd.poly[i]);
        }

        cmd.color = color;
        DrawShapeAddCommand(cmd);
    }

    void LeEngine::DrawShapeAddCommand(const DrawShapeCommand& cmd) {
        shapeRenderingState.commands.Add(cmd);
    }

    void LeEngine::DrawShapeClearCommands() {
        shapeRenderingState.commands.Clear();
    }

    void LeEngine::DrawShapeRender() {
        const i32 commandCount = shapeRenderingState.commands.GetCount();
        for (i32 drawCommandIndex = 0; drawCommandIndex < commandCount; drawCommandIndex++) {
            DrawShapeCommand& cmd = shapeRenderingState.commands[drawCommandIndex];
            switch (cmd.type)
            {
            case DRAW_SHAPE_TYPE_RECT:
            {
                f32 vertices[6][2] = {
                    { cmd.tl.x, cmd.tl.y, },
                    { cmd.bl.x, cmd.bl.y, },
                    { cmd.br.x, cmd.br.y, },

                    { cmd.tl.x, cmd.tl.y, },
                    { cmd.br.x, cmd.br.y, },
                    { cmd.tr.x, cmd.tr.y, }
                };

                ShaderProgramBind(&shapeRenderingState.program);
                ShaderProgramSetMat4("p", cmd.projection);
                ShaderProgramSetInt("mode", 0);
                ShaderProgramSetVec4("color", cmd.color);

                glBindVertexArray(shapeRenderingState.vertexBuffer.vao);
                VertexBufferUpdate(shapeRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);
            } break;
            case DRAW_SHAPE_TYPE_CIRCLE:
            {
                f32 x1 = cmd.center.x - cmd.radius;
                f32 y1 = cmd.center.y - cmd.radius;
                f32 x2 = cmd.center.x + cmd.radius;
                f32 y2 = cmd.center.y + cmd.radius;

                f32 vertices[6][2] = {
                    { x1, y2 },
                    { x1, y1 },
                    { x2, y1 },
                    { x1, y2 },
                    { x2, y1 },
                    { x2, y2 }
                };

                ShaderProgramBind(&shapeRenderingState.program);
                ShaderProgramSetMat4("p", cmd.projection);
                ShaderProgramSetInt("mode", 1);
                ShaderProgramSetVec4("color", cmd.color);
                ShaderProgramSetVec4("shapePosAndSize", glm::vec4(cmd.center.x, (f32)mainSurfaceHeight - cmd.center.y, cmd.radius, cmd.radius));
                ShaderProgramSetVec4("shapeRadius", glm::vec4(cmd.radius - 2, 0, 0, 0)); // The 4 here is to stop the circle from being cut of from the edges

                glBindVertexArray(shapeRenderingState.vertexBuffer.vao);
                VertexBufferUpdate(shapeRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);
            } break;
            case DRAW_SHAPE_TYPE_RECT_ROUND:
            {
                f32 xpos = cmd.bl.x;
                f32 ypos = cmd.bl.y;
                f32 h = cmd.tr.y - cmd.bl.y;
                f32 w = cmd.tr.x - cmd.bl.x;

                f32 vertices[6][2] = {
                    { xpos,     ypos + h, },
                    { xpos,     ypos,     },
                    { xpos + w, ypos,     },

                    { xpos,     ypos + h, },
                    { xpos + w, ypos,     },
                    { xpos + w, ypos + h, }
                };

                f32 centerX = (cmd.bl.x + cmd.tr.x) / 2.0f;
                f32 centerY = (cmd.bl.y + cmd.tr.y) / 2.0f;

                ShaderProgramBind(&shapeRenderingState.program);
                ShaderProgramSetMat4("p", cmd.projection);
                ShaderProgramSetInt("mode", 2);
                ShaderProgramSetVec4("color", cmd.color);
                ShaderProgramSetVec4("shapePosAndSize", glm::vec4(centerX, centerY, w, h));
                ShaderProgramSetVec4("shapeRadius", glm::vec4(cmd.radius, 0, 0, 0));

                glBindVertexArray(shapeRenderingState.vertexBuffer.vao);
                VertexBufferUpdate(shapeRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);
            } break;
            case DRAW_SHAPE_TYPE_RECT_POLY:
            {
                const f32* vertices = (f32*)cmd.poly.GetData();
                const i32 vertexCount = cmd.poly.GetCount();
                const i32 vertexSize = cmd.poly.GetCount() * sizeof(glm::vec2);
                
                ShaderProgramBind(&shapeRenderingState.program);
                ShaderProgramSetMat4("p", cmd.projection);
                ShaderProgramSetInt("mode", 0);
                ShaderProgramSetVec4("color", cmd.color);

                glBindVertexArray(shapeRenderingState.vertexBuffer.vao);
                VertexBufferUpdate(shapeRenderingState.vertexBuffer, 0, vertexSize, vertices);
                glDrawArrays(GL_TRIANGLES, 0, vertexCount);
                glBindVertexArray(0);
            } break;
            default:
                break;
            }
        }
    }

    void LeEngine::DrawSprite(const AssetId& id, glm::vec2 pos, f32 rotation, i32 frameIndex) {
        SpriteAsset *sprite = GetSpriteAsset(id);
        
        if (sprite == nullptr) {
            ATTOERROR("SPRITE: Could not find sprite asset");
            return;
        }

        DrawSprite(sprite, pos, rotation, frameIndex);
    }

    void LeEngine::DrawSprite(SpriteAsset* sprite, glm::vec2 pos, f32 rotation, i32 frameIndex) {
        Assert(sprite != nullptr, "SPRITE: Sprite is null");

        if (sprite->texture == nullptr) {
            sprite->texture = LoadTextureAsset(sprite->textureId);
            if (sprite->texture == nullptr) {
                ATTOERROR("SPRITE: Could not load texture asset");
                return;
            }
        }

        ShaderProgramBind(&spriteRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection * cameraView);
        ShaderProgramSetSampler("texture0", 0);
        ShaderProgramSetTexture(0, sprite->texture->textureHandle);

        f32 xpos = 0.0f;
        f32 ypos = 0.0f;

        f32 scaleX = 1;
        f32 scaleY = 1;

        //sprite->frameSize.y = (f32)sprite->texture->height;
        //sprite->frameSize.x = (f32)sprite->texture->width;

        f32 w = sprite->frameSize.x * scaleX;
        f32 h = sprite->frameSize.y * scaleY;

        if (sprite->origin == SPRITE_ORIGIN_CENTER) {
            xpos -= w / 2.0f;
            ypos -= h / 2.0f;
        }
        else if (sprite->origin == SPRITE_ORIGIN_BOTTOM_CENTER) {
            xpos -= w / 2.0f;
        }
        
        glm::mat2 rotationMatrix = glm::mat2(cos(rotation), -sin(rotation), sin(rotation), cos(rotation));
        glm::vec2 vertex1 = rotationMatrix * glm::vec2(xpos, ypos + h);
        glm::vec2 vertex2 = rotationMatrix * glm::vec2(xpos, ypos);
        glm::vec2 vertex3 = rotationMatrix * glm::vec2(xpos + w, ypos);
        glm::vec2 vertex4 = rotationMatrix * glm::vec2(xpos + w, ypos + h);
        
        vertex1 += pos;
        vertex2 += pos;
        vertex3 += pos;
        vertex4 += pos;

        glm::vec2 uv0 = sprite->uv0;
        glm::vec2 uv1 = sprite->uv1;

        uv0.x = (frameIndex * sprite->frameSize.x) / sprite->texture->width;
        uv1.x = (frameIndex * sprite->frameSize.x + sprite->frameSize.x) / sprite->texture->width;

        glm::vec4 color = spriteRenderingState.color;

        f32 vertices[6][2 + 2 + 4] = {
            { vertex1.x,  vertex1.y,    uv0.x, uv0.y,    color.r, color.g, color.b, color.a },
            { vertex2.x,  vertex2.y,    uv0.x, uv1.y,    color.r, color.g, color.b, color.a },
            { vertex3.x,  vertex3.y,    uv1.x, uv1.y,    color.r, color.g, color.b, color.a },

            { vertex1.x, vertex1.y,    uv0.x, uv0.y,    color.r, color.g, color.b, color.a },
            { vertex3.x, vertex3.y,    uv1.x, uv1.y,    color.r, color.g, color.b, color.a },
            { vertex4.x, vertex4.y,    uv1.x, uv0.y,    color.r, color.g, color.b, color.a }
        };

        static_assert(sizeof(vertices) == sizeof(SpriteVertex) * 6, "Sprite vertex size mismatch");

        glBindVertexArray(spriteRenderingState.vertexBuffer.vao);
        VertexBufferUpdate(spriteRenderingState.vertexBuffer, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void LeEngine::DrawTextSetFont(FontAssetId id) {
        FontAsset* fontAsset = LoadFontAsset(id);
        if (!fontAsset) {
            ATTOERROR("Could not load font asset");
            return;
        }

        textRenderingState.font = fontAsset;
    }

    void LeEngine::DrawTextSetHalign(FontHAlignment hAlignment) {
        textRenderingState.hAlignment = hAlignment;
    }

    void LeEngine::DrawTextSetValign(FontVAlignment vAlignment) {
        textRenderingState.vAlignment = vAlignment;
    }

    f32 LeEngine::DrawTextWidth(FontAsset* currentFont, const char* text) {
        Assert(currentFont, "No font set");

        f32 width = 0;
        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = currentFont->glyphs[index];
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            // bit shift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            width += (ch.advance >> 6);
        }

        return width;
    }

    BoxBounds LeEngine::DrawTextBounds(FontAsset* currentFont, const char* text) {
        Assert(currentFont, "No font set");

        BoxBounds bounds = {};

        f32 x = 0.0f;
        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = currentFont->glyphs[index];

            f32 xpos = (f32)ch.bearing.x;
            f32 ypos = (f32)(-(ch.size.y - ch.bearing.y));
            f32 w = (f32)ch.size.x;
            f32 h = (f32)ch.size.y;

            bounds.min.y = glm::min(bounds.min.y, ypos);
            bounds.max.y = glm::max(bounds.max.y, ypos + h);

            x += (ch.advance >> 6);
        }

        bounds.max.x = x;

        return bounds;
    }

    DrawEntryFont LeEngine::DrawTextCreate(const char* text, glm::vec2 pos) {
        DrawEntryFont entry = {};
        entry.text = text;
        entry.pos = pos;
        entry.font = textRenderingState.font;
        entry.hAlignment = textRenderingState.hAlignment;
        entry.vAlignment = textRenderingState.vAlignment;
        entry.underlineThinkness = textRenderingState.underlineThinkness;
        entry.underlinePercent = textRenderingState.underlinePercent;
        entry.color = textRenderingState.color;

        BoxBounds bounds = DrawTextBounds(textRenderingState.font, text);
        if (entry.hAlignment == FONT_HALIGN_CENTER) {
            f32 width = bounds.max.x - bounds.min.x;
            bounds.min.x -= width / 2.0f;
            bounds.max.x -= width / 2.0f;
        }

        entry.textWidth = DrawTextWidth(textRenderingState.font, text);

        bounds.Translate(entry.pos);
        entry.bounds = bounds;

        return entry;
    }

    void LeEngine::DrawText(const char* inText, glm::vec2 pos) {
        DrawEntryFont entry = DrawTextCreate(inText, pos);

        glm::mat4 p = glm::ortho(0.0f, (f32)mainSurfaceWidth, 0.0f, (f32)mainSurfaceHeight, -1.0f, 1.0f);

        ShaderProgramBind(&textRenderingState.program);
        ShaderProgramSetMat4("p", screenProjection);
        ShaderProgramSetSampler("texture0", 0);

        glBindVertexArray(textRenderingState.vertexBuffer.vao);

        f32 x = entry.pos.x;
        f32 y = entry.pos.y;
        const char* text = entry.text.GetCStr();
        const f32 textWidth = entry.textWidth;
        
        if (entry.hAlignment == FONT_HALIGN_CENTER) {
            x -= textWidth / 2.0f;
        }

        if (entry.underlineThinkness > 0.0f && entry.underlinePercent > 0.0f) {
            ShaderProgramSetInt("mode", 1);
            ShaderProgramSetVec4("color", glm::vec4(1, 1, 1, 1));

            f32 xpos = x;
            f32 ypos = y - entry.underlineThinkness - 1.0f;
            f32 w = textWidth * entry.underlinePercent;
            f32 h = entry.underlineThinkness;

            if (entry.hAlignment == FONT_HALIGN_CENTER) {
                xpos = x + textWidth / 2.0f - w / 2.0f;
            }

            f32 vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };

            glBindBuffer(GL_ARRAY_BUFFER, textRenderingState.vertexBuffer.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        ShaderProgramSetInt("mode", 0);
        ShaderProgramSetTexture(0, entry.font->textureHandle);

        for (i32 i = 0; text[i] != '\0'; i++) {
            i32 index = (i32)text[i];
            Glyph& ch = entry.font->glyphs[index];

            f32 xpos = x + ch.bearing.x;
            f32 ypos = y + (ch.size.y - ch.bearing.y);
            f32 w = (f32)ch.size.x;
            f32 h = (f32)ch.size.y;

            glm::vec2 uv0 = ch.uv0;
            glm::vec2 uv1 = ch.uv1;

            f32 vertices[6][4] = {
                { xpos,     ypos - h,   uv0.x, uv0.y },
                { xpos,     ypos,       uv0.x, uv1.y },
                { xpos + w, ypos,       uv1.x, uv1.y },

                { xpos,     ypos - h,   uv0.x, uv0.y },
                { xpos + w, ypos,       uv1.x, uv1.y },
                { xpos + w, ypos - h,   uv1.x, uv0.y }
            };

            //f32 vertices[6][4] = {
            //    { xpos,     ypos,           uv0.x, uv1.y },
            //    { xpos,     ypos + h,       uv0.x, uv0.y },
            //    { xpos + w, ypos + h,       uv1.x, uv0.y },

            //    { xpos,     ypos,           uv0.x, uv1.y },
            //    { xpos + w, ypos + h,       uv1.x, uv0.y },
            //    { xpos + w, ypos,           uv1.x, uv1.y }
            //};

            glBindBuffer(GL_ARRAY_BUFFER, textRenderingState.vertexBuffer.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            // bit shift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            x += (ch.advance >> 6);
        }

        glBindVertexArray(0);
    }

    void LeEngine::DrawText(SmallString text, glm::vec2 pos) {
        DrawText(text.GetCStr(), pos);
    }

    void LeEngine::DEBUGPushLine(glm::vec2 a, glm::vec2 b) {
        const i32 newCount = debugRenderingState.lines.GetCount() + 2;
        if (newCount * sizeof(DebugLineVertex) > debugRenderingState.vertexBufer.size) {
            ATTOINFO("To many debug lines!!");
            return;
        }

        DebugLineVertex v1;
        v1.position = a;
        v1.color = debugRenderingState.color;

        DebugLineVertex v2;
        v2.position = b;
        v2.color = debugRenderingState.color;

        debugRenderingState.lines.Add(v1);
        debugRenderingState.lines.Add(v2);
    }

    void LeEngine::DEBUGPushRay(Ray2D ray) {
        DEBUGPushLine(ray.origin, ray.origin + ray.direction);
    }

    void LeEngine::DEBUGPushCircle(glm::vec2 pos, f32 radius) {
        const i32 newCount = debugRenderingState.lines.GetCount() + 2 * 32;
        if (newCount * sizeof(DebugLineVertex) > debugRenderingState.vertexBufer.size) {
            ATTOINFO("To many debug lines!!");
            return;
        }

        f32 angle = 0.0f;
        constexpr f32 angleStep = 2.0f * glm::pi<f32>() / 32.0f;
        for (i32 i = 0; i < 32; i++) {
            glm::vec2 a = pos + glm::vec2(cos(angle), sin(angle)) * radius;
            glm::vec2 b = pos + glm::vec2(cos(angle + angleStep), sin(angle + angleStep)) * radius;

            DEBUGPushLine(a, b);

            angle += angleStep;
        }
    }

    void LeEngine::DEBUGPushBox(BoxBounds box) {
        glm::vec2 v1 = box.min;
        glm::vec2 v2 = box.max;
        glm::vec2 v3 = glm::vec2(box.min.x, box.max.y);
        glm::vec2 v4 = glm::vec2(box.max.x, box.min.y);
        DEBUGPushLine(v1, v3);
        DEBUGPushLine(v1, v4);
        DEBUGPushLine(v2, v3);
        DEBUGPushLine(v2, v4);
    }

    void LeEngine::DEBUGSubmit() {
        const u32 vertexCount = debugRenderingState.lines.GetCount();
        const u32 vertexSize = vertexCount * sizeof(DebugLineVertex);

        if (vertexCount == 0) {
            return;
        }

        glNamedBufferSubData(debugRenderingState.vertexBufer.vbo, 0, vertexSize, debugRenderingState.lines.GetData());

        ShaderProgramBind(&debugRenderingState.program);
        ShaderProgramSetMat4("p", cameraProjection * cameraView);
        glBindVertexArray(debugRenderingState.vertexBufer.vao);
        glDrawArrays(GL_LINES, 0, vertexCount);

        debugRenderingState.lines.Clear();
    }

    void LeEngine::EditorToggleConsole() {
        if (app->input->keys[KEY_CODE_F1]) {
            editorState.consoleOpen = true;
        }
        
        if (editorState.consoleOpen) {
            editorState.consoleScroll += app->deltaTime;
            if (editorState.consoleScroll > 1) {
                editorState.consoleScroll = 1;
            }
            
            f32 target = 0.9f;
            f32 amount = Lerp(0.0f, target, editorState.consoleScroll);

            DrawShapeRect(
                glm::vec2(0.0f, (1 - amount) * mainSurfaceHeight),
                glm::vec2(mainSurfaceWidth, mainSurfaceHeight)
            );
        }
        else {
            editorState.consoleScroll = 0.0f;
        }
    }

    VertexBuffer LeEngine::SubmitVertexBuffer(i32 sizeBytes, const void* data, VertexLayoutType layoutType, bool dyanmic) {
        VertexBuffer buffer = {};
        buffer.size = sizeBytes;

        glGenVertexArrays(1, &buffer.vao);
        glGenBuffers(1, &buffer.vbo);

        glBindVertexArray(buffer.vao);

        glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
        glBufferData(GL_ARRAY_BUFFER, buffer.size, data, dyanmic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

        switch (layoutType) {
            case VERTEX_LAYOUT_TYPE_SHAPE: {
                buffer.stride = sizeof(ShapeVertex);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
            } break;

            case VERTEX_LAYOUT_TYPE_SPRITE: {
                buffer.stride = sizeof(SpriteVertex);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, false, buffer.stride, (void*)(2 * sizeof(f32)));
                glVertexAttribPointer(2, 4, GL_FLOAT, false, buffer.stride, (void*)((2 + 2) * sizeof(f32)));
            } break;

            case VERTEX_LAYOUT_TYPE_FONT: {
                buffer.stride = sizeof(FontVertex);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
                glVertexAttribPointer(1, 2, GL_FLOAT, false, buffer.stride, (void*)(2 * sizeof(f32)));
            } break;

            case VERTEX_LAYOUT_TYPE_DEBUG_LINE: {
                buffer.stride = sizeof(DebugLineVertex);
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(0, 2, GL_FLOAT, false, buffer.stride, 0);
                glVertexAttribPointer(1, 4, GL_FLOAT, false, buffer.stride, (void*)(2 * sizeof(f32)));
            } break;

            default: {
                Assert(0, "");
            }
        }

        glBindVertexArray(0);

        return buffer;
    }

    ShaderProgram LeEngine::SubmitShaderProgram(const char* vertexSource, const char* fragmentSource) {
        ShaderProgram program = {};

        u32 vertexShader;
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        if (!GLCheckShaderCompilationErrors(vertexShader)) {
            return {};
        }

        u32 fragmentShader;
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        if (!GLCheckShaderCompilationErrors(fragmentShader)) {
            return {};
        }

        program.programHandle = glCreateProgram();
        glAttachShader(program.programHandle, vertexShader);
        glAttachShader(program.programHandle, fragmentShader);
        glLinkProgram(program.programHandle);
        if (!GLCheckShaderLinkErrors(program.programHandle)) {
            return {};
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }

    u32 LeEngine::SubmitTextureR8B8G8A8(i32 width, i32 height, byte* data, i32 wrapMode, bool generateMipMaps) {
        u32 textureHandle = 0;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        if (generateMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        return textureHandle;
    }

    u32 LeEngine::SubmitAudioClip(i32 sizeBytes, byte* data, i32 channels, i32 bitDepth, i32 sampleRate) {
        u32 alFormat = ALGetFormat(channels, bitDepth);

        u32 buffer = 0;
        alGenBuffers(1, &buffer);
        alBufferData(buffer, alFormat, data, (ALsizei)sizeBytes, (ALsizei)sampleRate);

        ALCheckErrors();

        return buffer;
    }

    void LeEngine::ALCheckErrors() {
        ALCenum error = alGetError();
        if (error != AL_NO_ERROR) {
            switch (error)
            {
            case AL_INVALID_NAME:
                ATTOERROR("AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL ");
                break;
            case AL_INVALID_ENUM:
                ATTOERROR("AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL ");
                break;
            case AL_INVALID_VALUE:
                ATTOERROR("AL_INVALID_VALUE: an invalid value was passed to an OpenAL ");
                break;
            case AL_INVALID_OPERATION:
                ATTOERROR("AL_INVALID_OPERATION: the requested operation is n");
                break;
            case AL_OUT_OF_MEMORY:
                ATTOERROR("AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out ");
                break;
            default:
                ATTOERROR("UNKNOWN AL ERROR: ");
            }
            ATTOERROR("");
        }
    }

    u32 LeEngine::ALGetFormat(u32 numChannels, u32 bitDepth) {
        b8 sterio = numChannels > 1;
        if (sterio) {
            if (bitDepth == 8) {
                return AL_FORMAT_STEREO8;
            }
            else if (bitDepth == 16) {
                return AL_FORMAT_STEREO16;
            }
        }
        else {
            if (bitDepth == 8) {
                return AL_FORMAT_MONO8;
            }
            else if (bitDepth == 16) {
                return AL_FORMAT_MONO16;
            }
        }

        ATTOERROR("Unsupported audio format");

        return 0;
    }

    bool LeEngine::GLCheckShaderCompilationErrors(u32 shader) {
        i32 success;
        char infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            ATTOERROR("ERROR::SHADER_COMPILATION_ERROR of type: ");
            ATTOERROR(infoLog);
            ATTOERROR("-- --------------------------------------------------- -- ");
        }

        return success;
    }

    bool LeEngine::GLCheckShaderLinkErrors(u32 program) {
        i32 success;
        char infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            ATTOERROR("ERROR::SHADER_LINKER_ERROR of type: ");
            ATTOERROR(infoLog);
            ATTOERROR("-- --------------------------------------------------- -- ");
        }

        return success;
    }

    void LeEngine::Win32OnDirectoryChanged(const char* directory, DirectoryChangeType changeType) {
        DirectoryChange change = {};
        change.path = directory;
        change.type = changeType;
        editorState.directoryLock.Lock();
        editorState.directoryChanges.Add(change);
        editorState.directoryLock.Unlock();
    }

    void LeEngine::UpdateAssets() {
        editorState.directoryLock.Lock();

        const i32 count = editorState.directoryChanges.GetCount();
        for (i32 i = 0; i < count; i++) {
            DirectoryChange change = editorState.directoryChanges[i];
            switch (change.type)
            {
            case DirectoryChangeType::FILE_ADDED: {
                ATTOINFO("File added: %s", change.path);
            } break;
            case DirectoryChangeType::FILE_REMOVED: {
                ATTOINFO("File removed: %s", change.path);
            } break;
            case DirectoryChangeType::FILE_MODIFIED: {
                ATTOINFO("File modified: %s", change.path);
                
                if (change.path.EndsWith(".png")) {
                    change.path.StripFileExtension();
                    change.path.RemovePathPrefix(basePathSprites.GetCStr());
                    AssetId id = AssetId::Create(change.path.GetCStr());
                    
                }

            } break;
            }
        }

        editorState.directoryChanges.Clear();

        editorState.directoryLock.Unlock();
    }

    void LeEngine::RegisterAssets() {
#if ATTO_EDITOR
        Win32WatchDirectory(basePathAssets.GetCStr());
#endif

        List<LargeString> texturePaths;
        FindAllFiles(basePathSprites.GetCStr(), ".png", texturePaths);
        FindAllFiles(basePathSprites.GetCStr(), ".jpg", texturePaths);
        FindAllFiles(basePathSprites.GetCStr(), ".bmp", texturePaths);

        List<LargeString> audioPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ogg", audioPaths);
        FindAllFiles(app->looseAssetPath.GetCStr(), ".wav", audioPaths);

        List<LargeString> fontPaths;
        FindAllFiles(app->looseAssetPath.GetCStr(), ".ttf", fontPaths);

        const i32 textureCount = texturePaths.GetNum();
        for (i32 texturePathIndex = 0; texturePathIndex < textureCount; ++texturePathIndex) {
            EngineAsset asset = {};
            asset.type = ASSET_TYPE_TEXTURE;

            LargeString path = texturePaths[texturePathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            path.RemovePathPrefix(basePathSprites.GetCStr());
            asset.id = AssetId::Create(path.GetCStr());

            asset.texture = TextureAsset::CreateDefault();

            engineAssets.Add(asset);

            ATTOTRACE("Found texture asset: %s", path.GetCStr());
        }

        const i32 audioCount = audioPaths.GetNum();
        for (i32 audioPathIndex = 0; audioPathIndex < audioCount; ++audioPathIndex) {
            EngineAsset asset = {};
            asset.type = ASSET_TYPE_AUDIO;

            LargeString path = audioPaths[audioPathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            asset.audio = AudioAsset::CreateDefault();

            engineAssets.Add(asset);

            ATTOTRACE("Found audio asset: %s", path.GetCStr());
        }

        const i32 fontCount = fontPaths.GetNum();
        for (i32 fontPathIndex = 0; fontPathIndex < fontCount; ++fontPathIndex) {
            EngineAsset asset = {};
            asset.type = ASSET_TYPE_FONT;

            LargeString path = fontPaths[fontPathIndex];
            path.BackSlashesToSlashes();
            asset.path = path;
            path.StripFileExtension();
            asset.id = AssetId::Create(path.GetCStr());

            asset.font = FontAsset::CreateDefault();

            engineAssets.Add(asset);

            ATTOTRACE("Found font asset: %s", path.GetCStr());
        }

        // TODO: Hard coded string !!
        std::ifstream spritesRegisterFile("assets/sprites.json");
        if (spritesRegisterFile.is_open()) {
            nlohmann::json j = nlohmann::json::parse(spritesRegisterFile);
            
            for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
                std::string spriteName = it.key();
                nlohmann::json spriteData = it.value();
                
                std::string texture = spriteData["texture"].get<std::string>();

                SpriteAsset sprite = SpriteAsset::CreateDefault();
                sprite.id = AssetId::Create(spriteName.c_str());
                sprite.textureId = TextureAssetId::Create(texture.c_str());
                sprite.frameCount = spriteData["frameCount"].get<i32>();
                sprite.frameSize.x = spriteData["frameSize"]["x"].get<f32>();
                sprite.frameSize.y = spriteData["frameSize"]["y"].get<f32>();

                std::string origin = spriteData["origin"].get<std::string>();
                if (origin == "CENTER") {
                    sprite.origin = SPRITE_ORIGIN_CENTER;
                }
                else if (origin == "BOTTOM_CENTER") {
                    sprite.origin = SPRITE_ORIGIN_BOTTOM_CENTER;
                }

                registeredSprites.Add(sprite);
            }

            spritesRegisterFile.close();
        }

    }

    bool LeEngine::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {
        textureAsset.generateMipMaps = false;

        void* pixelData = stbi_load(name, &textureAsset.width, &textureAsset.height, &textureAsset.channels, 4);

        if (!pixelData) {
            ATTOTRACE("Failed to load texture asset %s", name);
            return false;
        }

        glGenTextures(1, &textureAsset.textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureAsset.textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureAsset.width, textureAsset.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
        if (textureAsset.generateMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureAsset.generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(pixelData);

        return true;
    }

    bool LeEngine::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
        LargeString filename = LargeString::FromLiteral(name);
        if (filename.Contains(".ogg")) {
            if (LoadOGG(filename.GetCStr(), audioAsset)) {
                return true;
            }
        }
        else if (filename.Contains(".wav")) {
            if (LoadWAV(filename.GetCStr(), audioAsset)) {
                return true;
            }
        }
        else {
            ATTOTRACE("Unsupported audio file type: %s", filename.GetCStr());
        }

        return false;
    }

    bool LeEngine::LoadFontAsset(const char* filename, FontAsset& fontAsset) {
        FT_Library ft = {};
        if (FT_Init_FreeType(&ft)) {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            return false;
        }

        FT_Face face;
        if (FT_New_Face(ft, filename, 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            return false;
        }

        FT_Set_Pixel_Sizes(face, 0, fontAsset.fontSize);

        TileSheetGenerator tileSheet = {};

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }

            Glyph character = {};
            character.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            character.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            character.advance = face->glyph->advance.x;

            tileSheet.AddTile(character.size.x, character.size.y, face->glyph->bitmap.buffer, false);

            fontAsset.glyphs.Add(character);
        }

        List<byte> pixels;
        tileSheet.GenerateTiles(pixels, fontAsset.width, fontAsset.height);
        for (byte c = 0; c < 128; c++) {
            tileSheet.GetTileUV(c, fontAsset.glyphs[c].uv0, fontAsset.glyphs[c].uv1);
        }

        fontAsset.textureHandle = SubmitTextureR8B8G8A8(fontAsset.width, fontAsset.height, pixels.GetData(), GL_REPEAT, true);

        //Bitmap::Write(fontAsset.textureAsset.data.GetData(), fontAsset.textureAsset.width, fontAsset.textureAsset.height, "fontyboi.bmp");

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        return true;
    }

    bool LeEngine::LoadWAV(const char* filename, AudioAsset& audioAsset) {
        AudioFile<f32> audioFile;
        bool loaded = audioFile.load(filename);
        if (!loaded) {
            std::cout << "Failed to load audio file " << filename << std::endl;
            return false;
        }

        audioAsset.bitDepth = audioFile.getBitDepth();
        audioAsset.channels = audioFile.getNumChannels();
        audioAsset.sampleRate = audioFile.getSampleRate();

        std::vector<u8> loadedData;
        audioFile.savePCMToBuffer(loadedData);

        audioAsset.sizeBytes = (i32)loadedData.size();

        audioAsset.bufferHandle = SubmitAudioClip(audioAsset.sizeBytes, loadedData.data(), audioAsset.channels, audioAsset.bitDepth, audioAsset.sampleRate);
        
        return true;
    }

    bool LeEngine::LoadOGG(const char* filename, AudioAsset& audioAsset) {
        audioAsset.channels = 0;
        audioAsset.sampleRate = 0;
        audioAsset.bitDepth = 16;
        i16* loadedData = nullptr;
        i32 decoded = stb_vorbis_decode_filename(filename, &audioAsset.channels, &audioAsset.sampleRate, &loadedData);
        if (loadedData == nullptr) {
            std::cout << "Failed to load audio file " << filename << std::endl;
            return false;
        }

        audioAsset.sizeBytes = decoded * audioAsset.channels * sizeof(i16);

        audioAsset.bufferHandle = SubmitAudioClip(audioAsset.sizeBytes, (byte*)loadedData, audioAsset.channels, audioAsset.bitDepth, audioAsset.sampleRate);
        
        return true;
    }

    void PackedAssetFile::PutData(byte* data, i32 size) {
        const i32 s = storedData.GetNum();
        storedData.SetNum(s + size, true);
        std::memcpy(storedData.GetData() + s, data, size);
    }

    void PackedAssetFile::GetData(byte* data, i32 size) {
        Assert(currentOffset + size <= storedData.GetNum(), "PackedAssetFile::Get: currentOffset + size > storedData.GetNum()");
        byte* p = storedData.GetData() + currentOffset;
        std::memcpy(data, p, size);
        currentOffset += size;
    }

    void PackedAssetFile::SerializeAsset(TextureAsset& textureAsset) {
        Serialize(textureAsset.width);
        Serialize(textureAsset.height);
        Serialize(textureAsset.channels);
        Serialize(textureAsset.wrapMode);
        Serialize(textureAsset.generateMipMaps);
        //Serialize(textureAsset.data);
    }

    void PackedAssetFile::SerializeAsset(AudioAsset& audioAsset) {
        Serialize(audioAsset.channels);
        Serialize(audioAsset.sampleRate);
        Serialize(audioAsset.sizeBytes);
        Serialize(audioAsset.bitDepth);
    }

    void PackedAssetFile::SerializeAsset(FontAsset& fontAsset) {
        Serialize(fontAsset.fontSize);
        //SerializeAsset(fontAsset.textureAsset);
        Serialize(fontAsset.glyphs);
    }

    void PackedAssetFile::Reset() {
        currentOffset = 0;
    }

    bool PackedAssetFile::Save(const char* name) {
        std::ofstream file(name, std::ios::binary);
        if (!file.is_open()) {
            ATTOERROR("PackedAssetFile::Save -> Could not open file %s", name)
                return false;
        }

        file.write((char*)storedData.GetData(), storedData.GetNum());
        file.close();

        return true;
    }

    bool PackedAssetFile::Load(const char* name) {
        std::ifstream file(name, std::ios::binary);
        if (!file.is_open()) {
            ATTOERROR("PackedAssetFile::Load -> Could not open file %s", name);
            return false;
        }

        file.seekg(0, std::ios::end);
        const i32 size = (i32)file.tellg();
        file.seekg(0, std::ios::beg);

        storedData.SetNum(size, true);
        file.read((char*)storedData.GetData(), size);
        file.close();

        isLoading = true;

        return true;
    }

    void PackedAssetFile::Finished() {
        storedData.Clear();
    }

    //bool PackedAssetLoader::Begin() {
    //    texturePakFile.Load("assets/pixel_bois.pak");
    //    audioPakFile.Load("assets/noisy_bois.pak");
    //    fontPakFile.Load("assets/scribly_bois.pak");
    //    return true;
    //}
    //
    //bool PackedAssetLoader::LoadTextureAsset(const char* name, TextureAsset& textureAsset) {
    //    texturePakFile.SerializeAsset(textureAsset);
    //    return true;
    //}
    //bool PackedAssetLoader::LoadAudioAsset(const char* name, AudioAsset& audioAsset) {
    //    audioPakFile.SerializeAsset(audioAsset);
    //    return true;
    //}
    //bool PackedAssetLoader::LoadFontAsset(const char* name, FontAsset& fontAsset) {
    //    fontPakFile.SerializeAsset(fontAsset);
    //    return true;
    //}
    //void PackedAssetLoader::End() {
    //    texturePakFile.Finished();
    //    audioPakFile.Finished();
    //    fontPakFile.Finished();
    //}
    
}

