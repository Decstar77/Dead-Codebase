#pragma once

#include "AttoLib.h"
#include "AttoAsset.h"
#include "AttoLua.h"

namespace atto
{
    class Bitmap {
    public:
#pragma pack(push, 1)
        struct FileHeader {
            char type[2];
            u32 size;
            u16 reserved1;
            u16 reserved2;
            u32 offset;
        };

        struct InfoHeader {
            u32 size;
            i32 width;
            i32 height;
            u16 planes;
            u16 bitCount;
            u32 compression;
            u32 sizeImage;
            i32 xPelsPerMeter;
            i32 yPelsPerMeter;
            u32 clrUsed;
            u32 clrImportant;
        };
#pragma pack(pop)

        static bool Write(byte* pixels, u32 width, u32 height, const char* name);
    };

    class TileSheetGenerator {
    public:
        struct Tile {
            u32 width;
            u32 height;
            u32 xPos;
            u32 yPos;
            FixedList<byte, 2048> data;
            glm::vec2 uv0;
            glm::vec2 uv1;
            glm::vec2 boundingUV0;
            glm::vec2 boundingUV1;
        };

        void            AddTile(u32 width, u32 height, void* data, bool flipY = false);
        void            GenerateTiles(List<byte>& pixels, i32& width, i32& height);
        void            GetTileUV(i32 index, glm::vec2& uv0, glm::vec2& uv1);

        List<Tile>      tiles;
        u32             pixelStrideBytes = 4;

    private:
        void            FlipTile(Tile& tile);
    };
    
}


