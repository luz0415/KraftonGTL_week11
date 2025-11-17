-- Notify_SpawnParticle.lua
-- Spawns a particle effect at the actor's location

NotifyClass = {
    DisplayName = "Spawn Particle",
    Description = "Spawns a particle effect at actor location",

    Properties = {
        { Name = "ParticleSystem", Type = "String", Default = "Data/Effects/DefaultParticle.vfx" },
        { Name = "AttachToBone", Type = "String", Default = "" },
        { Name = "Offset", Type = "Vector", Default = {X = 0, Y = 0, Z = 0} },
        { Name = "Scale", Type = "Float", Default = 1.0 }
    }
}

function NotifyClass:Notify(meshComp, time)
    print(string.format("[Notify_SpawnParticle] Triggered at time %.2f", time))
    print(string.format("  ParticleSystem: %s", self.ParticleSystem or "None"))
    print(string.format("  AttachToBone: %s", self.AttachToBone or "None"))
    print(string.format("  Scale: %.2f", self.Scale or 1.0))

    local actor = meshComp:GetOwner()
    if actor then
        local spawnLocation = actor:GetActorLocation()

        if self.AttachToBone and self.AttachToBone ~= "" then
            -- Spawn attached to bone
            print(string.format("  Attaching to bone: %s", self.AttachToBone))
        else
            -- Spawn at actor location with offset
            if self.Offset then
                spawnLocation.X = spawnLocation.X + self.Offset.X
                spawnLocation.Y = spawnLocation.Y + self.Offset.Y
                spawnLocation.Z = spawnLocation.Z + self.Offset.Z
            end
            print(string.format("  Spawn Location: (%.1f, %.1f, %.1f)", spawnLocation.X, spawnLocation.Y, spawnLocation.Z))
        end

        -- TODO: Actual particle spawning logic
        -- SpawnParticleSystem(self.ParticleSystem, spawnLocation, self.Scale)
    end
end

return NotifyClass
