-- NotifyState_TrailEffect.lua
-- Spawns a trail effect on a weapon during the animation

NotifyStateClass = {
    DisplayName = "Trail Effect",
    Description = "Spawns a trail effect on weapon during animation",

    Properties = {
        { Name = "SocketName", Type = "String", Default = "WeaponSocket" },
        { Name = "TrailColor", Type = "Color", Default = {R = 1.0, G = 0.2, B = 0.2, A = 1.0} },
        { Name = "TrailWidth", Type = "Float", Default = 10.0 },
        { Name = "FadeTime", Type = "Float", Default = 0.5 }
    }
}

function NotifyStateClass:NotifyBegin(meshComp, time)
    print(string.format("[NotifyState_TrailEffect] Begin at time %.2f", time))
    print(string.format("  SocketName: %s", self.SocketName or "WeaponSocket"))
    print(string.format("  TrailWidth: %.2f", self.TrailWidth or 10.0))

    local actor = meshComp:GetOwner()
    if actor then
        -- TODO: Spawn trail effect component
        -- self.TrailComponent = SpawnTrailEffect(meshComp, self.SocketName, self.TrailColor, self.TrailWidth)
        print("  Trail effect STARTED")
    end
end

function NotifyStateClass:NotifyTick(meshComp, time, deltaTime)
    -- Update trail position per frame
    -- if self.TrailComponent then
    --     self.TrailComponent:UpdateTrail(deltaTime)
    -- end
end

function NotifyStateClass:NotifyEnd(meshComp, time)
    print(string.format("[NotifyState_TrailEffect] End at time %.2f", time))

    -- Stop spawning new trail segments, but let existing ones fade
    -- if self.TrailComponent then
    --     self.TrailComponent:StopTrail(self.FadeTime)
    --     self.TrailComponent = nil
    -- end
    print("  Trail effect ENDED")
end

return NotifyStateClass
