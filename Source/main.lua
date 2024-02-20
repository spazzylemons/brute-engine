import "actor.lua"

local Player = {}

local DEADZONE = 10
local TURNSPEED = 0.005

local DEG_0   = 0.0
local DEG_45  = math.pi / 4
local DEG_90  = math.pi / 2
local DEG_135 = (3 * math.pi) / 4
local DEG_180 = math.pi
local DEG_225 = -DEG_135
local DEG_270 = -DEG_90
local DEG_315 = -DEG_45

local dummies = {}

local function getAnalogStrength()
    local angle = playdate.getCrankPosition()
    if angle <= 180 then
        if angle < 90 - DEADZONE then
            return (90 - DEADZONE) - angle
        elseif angle > 90 + DEADZONE then
            return (90 - DEADZONE) - angle
        else
            return 0
        end
    else
        return 0
    end
end

function Player:init()
    -- TODO set norender flag? And/or make rendering engine skip viewpoint actor
    -- when rendering.
end

function Player:moveCrank()
    self:setAngle(self:getAngle() + getAnalogStrength() * TURNSPEED)

    local angle = nil
    if playdate.buttonIsPressed(playdate.kButtonLeft) then
        if playdate.buttonIsPressed(playdate.kButtonUp) then
            angle = DEG_45
        elseif playdate.buttonIsPressed(playdate.kButtonDown) then
            angle = DEG_135
        else
            angle = DEG_90
        end
    elseif playdate.buttonIsPressed(playdate.kButtonRight) then
        if playdate.buttonIsPressed(playdate.kButtonUp) then
            angle = DEG_315
        elseif playdate.buttonIsPressed(playdate.kButtonDown) then
            angle = DEG_225
        else
            angle = DEG_270
        end
    elseif playdate.buttonIsPressed(playdate.kButtonUp) then
        angle = DEG_0
    elseif playdate.buttonIsPressed(playdate.kButtonDown) then
        angle = DEG_180
    else
        return 0, 0
    end

    angle += self:getAngle()
    return math.sin(angle) * -6, math.cos(angle) * 6
end

function Player:moveDPad()
    if playdate.buttonIsPressed(playdate.kButtonLeft) then
        self:setAngle(self:getAngle() + 0.05)
    elseif playdate.buttonIsPressed(playdate.kButtonRight) then
        self:setAngle(self:getAngle() - 0.05)
    end

    local angle = self:getAngle()
    if playdate.buttonIsPressed(playdate.kButtonUp) then
        return math.sin(angle) * -6, math.cos(angle) * 6
    elseif playdate.buttonIsPressed(playdate.kButtonDown) then
        return math.sin(angle) * 6, math.cos(angle) * -6
    else
        return 0, 0
    end
end

function Player:update()
    local dx, dy
    if playdate.isCrankDocked() then
        dx, dy = self:moveDPad()
    else
        dx, dy = self:moveCrank()
    end

    if playdate.buttonJustPressed(playdate.kButtonA) then
        local x, y = self:getPos()
        dummies[#dummies+1] = actor.spawn({}, x, y, self:getAngle())
    end

    if playdate.buttonJustPressed(playdate.kButtonB) then
        for i = 1, #dummies do
            dummies[i]:despawn()
        end
        dummies = {}
    end

    local vx, vy = self:getVel()
    vx = (vx + dx * 0.25) * 0.8
    vy = (vy + dy * 0.25) * 0.8
    self:setVel(vx, vy)

    self:applyVelocity()
    self:applyGravity()
end

brute.init()

local player = actor.spawn(Player, 0, 0, 0)

function playdate.update()
    actor.update()
    brute.render.draw(player)
    playdate.drawFPS(0, 0)
end

function playdate.gameWillTerminate()
    collectgarbage()
    brute.quit()
end
