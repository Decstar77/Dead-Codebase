local vec2 = require ("assets.scripts.vec2")

-- engine is set before init
-- atto functions are the binding functions

function id_from_string(str)
    return atto_id_from_string(str)
end

function audio_play(id, params)
    atto_audio_play(engine, id, params.looping or false, params.volume or 1.0)
end

function draw_shape_set_color(r, g, b, a)
    atto_draw_shape_set_color(engine, r, g, b, a)
end

function draw_shape_rect(blX, blY, trX, trY)
    atto_draw_shape_rect(engine, r, g, b, a)
end

function draw_shape_rect_center(cx, cy, w, h)
    atto_draw_shape_rect_cd(engine, cx, cy, w, h)
end

function draw_shape_circle(cx, cy,r)
    atto_draw_shape_circle(engine, cx, cy, r)
end

function draw_shape_round_rect(blX, blY, trX, trY, r)
    atto_draw_shape_round_rect(engine, r, g, b, a, r)
end

function draw_sprite(id, x, y, frameIndex)
    atto_draw_sprite(engine, id, x, y, frameIndex)
end

-- sound = id_from_string("assets/sounds/music/battle_1")
-- audio_play(sound, {})

player = {
    pos = vec2.new(0,0),
    vel = vec2.new(0,0)
}

function start()

end

function update()

end

function render()
end