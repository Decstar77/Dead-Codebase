#include "AttoLib.h"
#include "AttoAsset.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_glfw_gl3.h>

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#include <fstream>
#include <json/json.hpp>


namespace atto
{
    void LeEngine::InitializeUIRendering(AppState* app) {
        uiRenderingState.nkGlfw = new nk_glfw();
        uiRenderingState.nkContext = nk_glfw3_init(uiRenderingState.nkGlfw, app->window, NK_GLFW3_DEFAULT);
        nk_glfw3_font_stash_begin(uiRenderingState.nkGlfw, &uiRenderingState.nkFontAtlas);
        nk_glfw3_font_stash_end(uiRenderingState.nkGlfw);
    }

    void LeEngine::DrawUINewFrame(AppState* app) {
        nk_glfw3_new_frame(uiRenderingState.nkGlfw);
    }

    void LeEngine::DrawUIRender(AppState* app) {
        nk_glfw3_render(uiRenderingState.nkGlfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    }

    void LeEngine::ShutdownUIRendering(AppState* app) {
        nk_glfw3_shutdown(uiRenderingState.nkGlfw);
    }
    
    void LeEngine::DrawUIDisplayJson(nlohmann::json j) {
        //nk_context* ctx = uiRenderingState.nkContext;
        //// Loop through JSON object and create Nuklear widgets
        //if (j.is_object()) {
        //    if (nk_tree_push_id(ctx, NK_TREE_NODE, j.type_name().c_str(), NK_MINIMIZED, j.dump().c_str(), j.dump().length())) {
        //        for (auto& element : j.items()) {
        //            std::string key = element.key();
        //            nlohmann::json value = element.value();
        //            if (value.is_string()) {
        //                nk_tree_push_id(ctx, NK_TREE_NODE, key.c_str(), NK_LEAF, value.get<std::string>().c_str(), value.get<std::string>().length());
        //                nk_tree_pop(ctx);
        //            }
        //            else if (value.is_number()) {
        //                nk_tree_push_id(ctx, NK_TREE_NODE, key.c_str(), NK_LEAF, std::to_string(value.get<double>()).c_str(), std::to_string(value.get<double>()).length());
        //                nk_tree_pop(ctx);
        //            }
        //            else if (value.is_boolean()) {
        //                nk_tree_push_id(ctx, NK_TREE_NODE, key.c_str(), NK_LEAF, value.get<bool>() ? "true" : "false", value.get<bool>() ? 4 : 5);
        //                nk_tree_pop(ctx);
        //            }
        //            else if (value.is_array() || value.is_object()) {
        //                nk_tree_push_id(ctx, NK_TREE_TAB, key.c_str(), NK_MAXIMIZED);
        //                create_json_tree(ctx, value);
        //                nk_tree_pop(ctx);
        //            }
        //        }
        //        nk_tree_pop(ctx);
        //    }
        //}
        //else if (j.is_array()) {
        //    int index = 0;
        //    for (auto& element : j) {
        //        std::string key = "[" + std::to_string(index) + "]";
        //        if (element.is_string()) {
        //            nk_tree_push_id(ctx, NK_TREE_NODE, key.c_str(), NK_LEAF, element.get<std::string>().c_str(), element.get<std::string>().length());
        //            nk_tree_pop(ctx);
        //        }
        //        else if (element.is_number()) {
        //            nk_tree_push_id(ctx, NK_TREE_NODE, key.c_str(), NK_LEAF, std::to_string(element.get<double>()).c_str(), std::to_string(element.get<double>()).length());
        //            nk_tree_pop(ctx);
        //        }
        //        else if (element.is_boolean()) {
        //            nk_tree_push_id(ctx, NK_TREE_NODE, key.c_str(), NK_LEAF, element.get<bool>() ? "true" : "false", element.get<bool>() ? 4 : 5);
        //            nk_tree_pop(ctx);
        //        }
        //        else if (element.is_array() || element.is_object()) {
        //            nk_tree_push_id(ctx, NK_TREE_TAB, key.c_str(), NK_MAXIMIZED);
        //            create_json_tree(ctx, element);
        //            nk_tree_pop(ctx);
        //        }
        //        index++;
        //    }
        //}
    }

    void LeEngine::DrawUIDemoJSON() {
        //std::ifstream spritesRegisterFile("assets/sprites.json");
        //if (spritesRegisterFile.is_open()) {
        //    nlohmann::json j = nlohmann::json::parse(spritesRegisterFile);
        //}

        nk_context* ctx = uiRenderingState.nkContext;
        struct nk_rect win_rect = nk_rect(0, 0, 400, 400);

        if (nk_begin(ctx, "JSON Viewer", win_rect, NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
            if (nk_tree_push(ctx, NK_TREE_NODE, "Tree", NK_MINIMIZED)) {
                // draw tree items here
                nk_layout_row_dynamic(ctx, 8, 1);
                nk_label(ctx, "Item 1", NK_TEXT_LEFT);
                nk_label(ctx, "Item 2", NK_TEXT_LEFT);
                nk_label(ctx, "Item 3", NK_TEXT_LEFT);

                nk_tree_pop(ctx);
            }

            //DrawUIDisplayJson(j);
        }
        nk_end(ctx);
        
        //struct nk_canvas canvas;
        //canvas_begin(&ctx, &canvas, 0, 0, 0, width, height, nk_rgb(250, 250, 250));

        if (nk_begin(ctx, "Testing window", win_rect, NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {
            //struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
            //struct nk_rect total_space = nk_window_get_content_region(ctx);
            //struct nk_color grid_color = nk_rgb(255, 255, 255);
            TextureAsset* textureAsset = LoadTextureAsset(TextureAssetId::Create("17072771891602062778gol1"));

            SpriteAsset* spriteAsset = GetSpriteAsset(AssetId::Create("unit_basic_man"));
            if (spriteAsset->texture != nullptr) {
                u32 handle = spriteAsset->texture->textureHandle;
                struct nk_image img = nk_image_id(handle);
                nk_layout_row_dynamic(ctx, 32, 1);
                if (nk_button_image(ctx, img)) {

                }
            }

            //f32 x = 0;
            //f32 y = 0;
            //nk_layout_space_begin(ctx, NK_STATIC, total_space.h, 1);
            //struct nk_rect size = nk_layout_space_bounds(ctx);
            //nk_stroke_line(canvas, x + size.x, size.y, x + size.x, size.y + size.h, 1.0f, grid_color);
            
        }
        nk_end(ctx);
    }

    void LeEngine::DrawUIDemoWindow() {
        nk_colorf bg;
        bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
        if (nk_begin(uiRenderingState.nkContext, "Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
            NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {
            enum { EASY, HARD };
            static int op = EASY;
            static int property = 20;
            nk_layout_row_static(uiRenderingState.nkContext, 30, 80, 1);
            if (nk_button_label(uiRenderingState.nkContext, "button"))
                fprintf(stdout, "button pressed\n");

            nk_layout_row_dynamic(uiRenderingState.nkContext, 30, 2);
            if (nk_option_label(uiRenderingState.nkContext, "easy", op == EASY)) op = EASY;
            if (nk_option_label(uiRenderingState.nkContext, "hard", op == HARD)) op = HARD;

            nk_layout_row_dynamic(uiRenderingState.nkContext, 25, 1);
            nk_property_int(uiRenderingState.nkContext, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(uiRenderingState.nkContext, 20, 1);
            nk_label(uiRenderingState.nkContext, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(uiRenderingState.nkContext, 25, 1);
            if (nk_combo_begin_color(uiRenderingState.nkContext, nk_rgb_cf(bg), nk_vec2(nk_widget_width(uiRenderingState.nkContext), 400))) {
                nk_layout_row_dynamic(uiRenderingState.nkContext, 120, 1);
                bg = nk_color_picker(uiRenderingState.nkContext, bg, NK_RGBA);
                nk_layout_row_dynamic(uiRenderingState.nkContext, 25, 1);
                bg.r = nk_propertyf(uiRenderingState.nkContext, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
                bg.g = nk_propertyf(uiRenderingState.nkContext, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
                bg.b = nk_propertyf(uiRenderingState.nkContext, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
                bg.a = nk_propertyf(uiRenderingState.nkContext, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
                nk_combo_end(uiRenderingState.nkContext);
            }
        }
        nk_end(uiRenderingState.nkContext);

        //if (nk_window_is_any_hovered(uiRenderingState.nkContext)) {
        //    ATTOINFO("Hovered");
        //}
    }
}


