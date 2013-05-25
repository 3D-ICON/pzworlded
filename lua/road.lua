function params()
    return 'width'
end

function region()
    --print('road.lua:region')
    local width = tonumber(script:value('width'))
    return script:paths()[1]:stroke(width + 0.01):region()
end

function run()
    local width = tonumber(script:value('width'))
    
    -- print('road.lua:run')
    painter:setTile(world:tile('blends_street_01', 80))
    painter:setLayer(world:tileLayer('0_Floor'))
    painter:strokePath(script:paths()[1], width + 0.01)

    --painter:setTile(world:tile('street_trafficlines_01', 0))
    painter:setLayer(world:tileLayer('0_Walls'))
    painter:tracePath(script:paths()[1],0)
end