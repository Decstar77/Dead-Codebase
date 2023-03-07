#include "AttoRendering.h"
#include "AttoLib.h"

#include <fstream>

namespace atto 
{
    bool Bitmap::Write(byte* pixels, u32 width, u32 height, const char* name) {
        const u32 pixelSize = width * height * 4;

        FileHeader fileHeader = {};
        fileHeader.type[0] = 'B';
        fileHeader.type[1] = 'M';
        fileHeader.size = sizeof(FileHeader) + sizeof(InfoHeader) + pixelSize;
        fileHeader.offset = sizeof(FileHeader) + sizeof(InfoHeader);

        InfoHeader infoHeader = {};
        infoHeader.size = sizeof(InfoHeader);
        infoHeader.width = width;
        infoHeader.height = height;
        infoHeader.planes = 1;
        infoHeader.bitCount = 32;
        infoHeader.compression = 0;
        infoHeader.sizeImage = 0;
        infoHeader.xPelsPerMeter = 3200;
        infoHeader.yPelsPerMeter = 3200;
        infoHeader.clrUsed = 0;
        infoHeader.clrImportant = 0;

        std::ofstream file(name, std::ios::binary);
        if (!file.is_open()) {
            ATTOERROR("Could not open file %s", name);
            return false;
        }

        file.write((char*)&fileHeader, sizeof(FileHeader));
        file.write((char*)&infoHeader, sizeof(InfoHeader));
        file.write((char*)pixels, pixelSize);
        file.close();

        return true;
    }

    void TileSheetGenerator::AddTile(u32 width, u32 height, void* data, bool flipY) {
        Tile& tile = tiles.Alloc();
        tile.width = width;
        tile.height = height;
        const u32 size = width * height;
        Assert(size < (u32)tile.data.GetCapcity(), "");
        tile.data.SetCount(size);
        std::memcpy(tile.data.GetData(), data, size);
        if (flipY) {
            FlipTile(tile);
        }
    }

    void TileSheetGenerator::GenerateTiles(List<byte>& pixels, i32& outWidth, i32& outHeight) {
        u32 tileWidth = 0;
        u32 tileHeight = 0;

        const i32 tileCount = tiles.GetNum();
        for (i32 tileIndex = 0; tileIndex < tileCount; tileIndex++) {
            Tile& tile = tiles[tileIndex];
            tileWidth = glm::max(tileWidth, tile.width);
            tileHeight = glm::max(tileHeight, tile.height);
        }

        const u32 widthInTiles = (u32)(glm::sqrt((f32)tileCount) + 1);
        const u32 heightInTiles = widthInTiles;
        const u32 widthInPixels = tileWidth * widthInTiles;
        const u32 heightInPixels = tileHeight * heightInTiles;
        const u32 totalSizeBytes = widthInPixels * heightInPixels * pixelStrideBytes;

        f32 xp = 0;
        f32 yp = 0;
        for (i32 tileIndex = 0; tileIndex < tileCount; tileIndex++) {
            Tile& tile = tiles[tileIndex];
            f32 wp = (f32)widthInPixels;
            f32 hp = (f32)heightInPixels;
            tile.uv0.x = xp / wp;
            tile.uv0.y = yp / hp;
            tile.uv1.x = (xp + tile.width) / wp;
            tile.uv1.y = (yp + tile.height) / hp;
            tile.boundingUV0.x = xp / wp;
            tile.boundingUV0.y = yp / hp;
            tile.boundingUV1.x = (xp + tileWidth) / wp;
            tile.boundingUV1.y = (yp + tileHeight) / hp;
            tile.xPos = (u32)xp;
            tile.yPos = (u32)yp;
            xp += tileWidth;
            if (xp >= wp) {
                xp = 0;
                yp += tileHeight;
            }
        }

        pixels.SetNum(totalSizeBytes, true);
        std::memset(pixels.GetData(), 0, totalSizeBytes);
        for (i32 tileIndex = 0; tileIndex < tileCount; tileIndex++) {
            Tile& tile = tiles[tileIndex];

            const i32 x = tile.xPos;
            const i32 y = tile.yPos;
            const i32 w = tile.width;
            const i32 h = tile.height;

            Assert(x + w <= (i32)widthInPixels, "x + width > this->width");
            Assert(y + h <= (i32)heightInPixels, "y + height > this->height");

            i32 dy = 0;
            for (i32 yIndex = y; yIndex < y + h; yIndex++, dy++) {
                i32 dx = 0;
                for (i32 xIndex = x; xIndex < x + w; xIndex++, dx++) {
                    const i32 index = dy * w + dx;
                    const u8 r = tile.data[index];

                    const i32 pixelIndex = (yIndex * widthInPixels + xIndex) * 4;

                    pixels[pixelIndex + 0] = r;
                    pixels[pixelIndex + 1] = r;
                    pixels[pixelIndex + 2] = r;
                    pixels[pixelIndex + 3] = 255;
                }
            }
        }

        outWidth = (i32)widthInPixels;
        outHeight = (i32)heightInPixels;
    }

    void TileSheetGenerator::GetTileUV(i32 index, glm::vec2& uv0, glm::vec2& uv1) {
        uv0 = tiles[index].uv0;
        uv1 = tiles[index].uv1;
    }

    void TileSheetGenerator::FlipTile(Tile& tile) {
        const u32 width = tile.width;
        const u32 height = tile.height;
        const u32 size = width * height;
        List<byte> temp;
        temp.SetNum(size);
        for (u32 y = 0; y < height; y++) {
            for (u32 x = 0; x < width; x++) {
                const u32 index = y * width + x;
                const u32 flippedIndex = (height - y - 1) * width + x;
                temp[index] = tile.data[flippedIndex];
            }
        }
        std::memcpy(tile.data.GetData(), temp.GetData(), size);
    }

    void LeEngine::InitializeShapeRendering() {
        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;

            uniform mat4 p;

            void main() {
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            uniform int  mode;
            uniform vec4 color;
            uniform vec4 shapePosAndSize;
            uniform vec4 shapeRadius;

            // from http://www.iquilezles.org/www/articles/distfunctions/distfunctions

            float CircleSDF(vec2 r, vec2 p, float rad) {
                return 1 - max(length(p - r) - rad, 0);
            }

            float BoxSDF(vec2 r, vec2 p, vec2 s) {
                return 1 - length(max(abs(p - r) - s, 0));
            }
            
            float RoundedBoxSDF(vec2 r, vec2 p, vec2 s, float rad) {
                return 1 - (length(max(abs(p - r) - s + rad, 0)) - rad);
            }

            void main() {
                vec2 s = shapePosAndSize.zw;
                vec2 r = shapePosAndSize.xy;
                vec2 p = gl_FragCoord.xy;

                if (mode == 0) {
                    FragColor = color;
                } else if (mode == 1) {
                    float d = CircleSDF(r, p, shapeRadius.x);
                    d = clamp(d, 0.0, 1.0);
                    FragColor = color * d; //vec4(color.xyz, color.w * d);
                } else if (mode == 2) {
                    float d = RoundedBoxSDF(r, p, s / 2, shapeRadius.x);
                    d = clamp(d, 0.0, 1.0);
                    FragColor = vec4(color.xyz, color.w * d);
                } else {
                    FragColor = vec4(1, 0, 1, 1);
                }
            }
        )";

        shapeRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);
        shapeRenderingState.vertexBuffer = SubmitVertexBuffer(sizeof(ShapeVertex) * 32, nullptr, VERTEX_LAYOUT_TYPE_SHAPE, true);

        ATTOTRACE("Completed shape rendering initialization");
    }

    void LeEngine::InitializeSpriteRendering() {
        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec2 texCoord;
            layout (location = 2) in vec4 color;

            out vec2 vertexTexCoord;
            out vec4 vertexColor;

            uniform mat4 p;

            void main() {
                vertexTexCoord = texCoord;
                vertexColor = color;
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 vertexTexCoord;
            in vec4 vertexColor;

            uniform sampler2D texture0;

            vec2 uv_cstantos( vec2 uv, vec2 res ) {
                vec2 pixels = uv * res;

                // Updated to the final article
                vec2 alpha = 0.7 * fwidth(pixels);
                vec2 pixels_fract = fract(pixels);
                vec2 pixels_diff = clamp( .5 / alpha * pixels_fract, 0, .5 ) +
                                   clamp( .5 / alpha * (pixels_fract - 1) + .5, 0, .5 );
                pixels = floor(pixels) + pixels_diff;
                return pixels / res;
            }

            vec2 uv_klems( vec2 uv, ivec2 texture_size ) {
            
                vec2 pixels = uv * texture_size + 0.5;
    
                // tweak fractional value of the texture coordinate
                vec2 fl = floor(pixels);
                vec2 fr = fract(pixels);
                vec2 aa = fwidth(pixels) * 0.75;

                fr = smoothstep( vec2(0.5) - aa, vec2(0.5) + aa, fr);
    
                return (fl + fr - 0.5) / texture_size;
            }

            vec2 uv_iq( vec2 uv, ivec2 texture_size ) {
                vec2 pixel = uv * texture_size;

                vec2 seam = floor(pixel + 0.5);
                vec2 dudv = fwidth(pixel);
                pixel = seam + clamp( (pixel - seam) / dudv, -0.5, 0.5);
    
                return pixel / texture_size;
            }

            vec2 uv_fat_pixel( vec2 uv, ivec2 texture_size ) {
                vec2 pixel = uv * texture_size;

                vec2 fat_pixel = floor(pixel) + 0.5;
                // subpixel aa algorithm (COMMENT OUT TO COMPARE WITH POINT SAMPLING)
                fat_pixel += 1 - clamp((1.0 - fract(pixel)) * 3, 0, 1);
        
                return fat_pixel / texture_size;
            }

            vec2 uv_aa_smoothstep( vec2 uv, vec2 res, float width ) {
                uv = uv * res;
                vec2 uv_floor = floor(uv + 0.5);
                vec2 uv_fract = fract(uv + 0.5);
                vec2 uv_aa = fwidth(uv) * width * 0.5;
                uv_fract = smoothstep(
                    vec2(0.5) - uv_aa,
                    vec2(0.5) + uv_aa,
                    uv_fract
                    );
    
                return (uv_floor + uv_fract - 0.5) / res;
            }


            void main() {
                ivec2 tSize = textureSize(texture0, 0);
                //vec2 uv = uv_cstantos(vertexTexCoord, vec2(tSize.x, tSize.y));
                //vec2 uv = uv_iq(vertexTexCoord, tSize);
                vec2 uv = uv_fat_pixel(vertexTexCoord, tSize);
                //vec2 uv = uv_aa_smoothstep(vertexTexCoord, vec2(tSize.x, tSize.y), 1);
                vec4 sampled = texture(texture0, uv);
                //vec4 sampled = texture(texture0, vertexTexCoord);
                //sampled.rgb *= sampled.a;
                //if (sampled.a < 1) discard;
                FragColor = sampled;
            }
        )";

        spriteRenderingState.color = glm::vec4(1, 1, 1, 1);
        spriteRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);
        spriteRenderingState.vertexBuffer = SubmitVertexBuffer(sizeof(SpriteVertex) * 6, nullptr, VERTEX_LAYOUT_TYPE_SPRITE, true);

        ATTOTRACE("Completed sprite rendering initialization");
    }

    void LeEngine::InitializeTextRendering() {
        textRenderingState.color = glm::vec4(1, 1, 1, 1);
        textRenderingState.underlinePercent = 1.0f;

        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec2 texCoord;

            out vec2 vertexTexCoord;

            uniform mat4 p;

            void main() {
                vertexTexCoord = texCoord;
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec2 vertexTexCoord;

            uniform int mode;
            uniform vec4 color;
            uniform sampler2D texture0;

            void main() {
                if (mode == 0) { // Text rendering
                    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture0, vertexTexCoord).r);
                    FragColor = vec4(1.0) * sampled * sampled.a;
                } else if (mode == 1 ) {
                    FragColor = color;
                }
            }
        )";

        textRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);
        textRenderingState.vertexBuffer = SubmitVertexBuffer(sizeof(FontVertex) * 6, nullptr, VERTEX_LAYOUT_TYPE_FONT, true);
        textRenderingState.font = LoadFontAsset(FontAssetId::Create("assets/fonts/arial"));
        
        ATTOTRACE("Completed text rendering initialization");
    }

    void LeEngine::InitializeDebugRendering() {
        debugRenderingState.color = glm::vec4(0, 1, 0, 1);

        const char* vertexShaderSource = R"(
            #version 330 core

            layout (location = 0) in vec2 position;
            layout (location = 1) in vec4 color;

            out vec4 vertexColor;

            uniform mat4 p;

            void main() {
                gl_Position = p * vec4(position.x, position.y, 0.0, 1.0);
                vertexColor = color;
            }
        )";

        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;

            in vec4 vertexColor;

            void main() {
                FragColor = vertexColor;
            }
        )";

        debugRenderingState.program = SubmitShaderProgram(vertexShaderSource, fragmentShaderSource);

        debugRenderingState.vertexBufer = SubmitVertexBuffer(
            debugRenderingState.lines.GetCapcity() * sizeof(DebugLineVertex), nullptr,
            VERTEX_LAYOUT_TYPE_DEBUG_LINE, true
        );

        ATTOTRACE("Completed debug rendering initialization");
    }
}