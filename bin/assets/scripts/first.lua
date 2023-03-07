
require "assets.scripts.other"

dt = 0
worldWidth = 0
worlHeight = 0
key_s = false
key_w = false
key_e = false

function create_main_menu()
    
end

menuSelection = 0
menuSelectionUnderlinePercent = 0

function test_function()
    print("lua-ception!!!")
end

function update_main_menu()
    print(dt)
    atto_test(test_function)
end

-- function update_star(gs, obj)
    
-- end

-- function create_main_game(gs)
--     x = math.random(0, worldWidth)
--     y = math.random(0, worldHeight)
--     sprite = atto_get_sprite("star0");
--     func = update_star
--     atto_create_object(x, y, sprite, func)
-- end
