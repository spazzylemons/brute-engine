local actorList = {}

local actorData = setmetatable({}, {__mode = 'k'})

local oldActorIndex = brute.classes.actor.__index

function brute.classes.actor.__index(table, key)
    local data = actorData[table]
    local value = data[key]
    if value ~= nil then
        return value
    end
    value = data.class
    if value ~= nil then
        value = value[key]
        if value ~= nil then
            return value
        end
    end
    return oldActorIndex[key]
end

brute.classes.actor.__gc = oldActorIndex.free

local oldDespawn = oldActorIndex.despawn
function oldActorIndex:despawn()
    self.scheduleDespawn = true
end

function brute.classes.actor.__newindex(table, key, value)
    local data = actorData[table]
    data[key] = value
end

actor = {}

function actor.spawn(class, x, y, angle)
    local obj = brute.newActor(x, y)
    actorData[obj] = {}

    obj:setAngle(angle)
    obj.class = class

    if obj.init ~= nil then
        obj:init()
    end

    actorList[#actorList+1] = obj

    return obj
end

function actor.update()
    for i = 1, #actorList do
        local obj = actorList[i]
        if obj.update ~= nil then
            obj:update()
        end
    end

    -- handle despawns
    local newActorList = {}
    for i = 1, #actorList do
        local obj = actorList[i]
        if obj.scheduleDespawn then
            -- remove object
            actorData[obj] = nil
            oldDespawn(obj)
        else
            newActorList[#newActorList+1] = obj
        end
    end
    actorList = newActorList
end
