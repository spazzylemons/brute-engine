local actorList = {}

---@class Actor
local Actor = {}

function Actor:register(x, y)
    self.userdata = brute.actor.register(x, y)
end

function Actor:getPos()
    return brute.actor.getPos(self.userdata)
end

function Actor:setPos(x, y)
    brute.actor.setPos(self.userdata, x, y)
end

function Actor:getVel()
    return brute.actor.getVel(self.userdata)
end

function Actor:setVel(x, y)
    brute.actor.setVel(self.userdata, x, y)
end

function Actor:getZPos()
    return brute.actor.getZPos(self.userdata)
end

function Actor:setZPos(z)
    brute.actor.setZPos(self.userdata, z)
end

function Actor:getZVel()
    return brute.actor.getZVel(self.userdata)
end

function Actor:setZVel(z)
    brute.actor.setZVel(self.userdata, z)
end

function Actor:getAngle()
    return brute.actor.getAngle(self.userdata)
end

function Actor:setAngle(angle)
    brute.actor.setAngle(self.userdata, angle % (2 * math.pi))
end

function Actor:applyVelocity()
    brute.actor.applyVelocity(self.userdata)
end

function Actor:applyGravity()
    brute.actor.applyGravity(self.userdata)
end

local actorMetaTable = {
    __index = function(table, key)
        local class = rawget(table, 'class')
        if class ~= nil then
            local inclass = class[key]
            if inclass ~= nil then
                return inclass
            end
        end
        local inactor = Actor[key]
        if inactor ~= nil then
            return inactor
        end
        return rawget(table, key)
    end
}

actor = {}

function actor.spawn(class, x, y, angle)
    local obj = {}
    setmetatable(obj, actorMetaTable)

    obj:register(x, y)
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
end
