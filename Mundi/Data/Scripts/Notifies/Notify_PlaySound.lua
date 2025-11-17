-- Notify_PlaySound.lua
-- Plays a sound at the actor's location

NotifyClass = {
    DisplayName = "Play Sound",
    Description = "Plays a sound at actor location",

    -- Properties that can be edited in the Timeline UI
    Properties = {
        { Name = "SoundPath", Type = "String", Default = "Data/Sounds/Default.wav" },
        { Name = "Volume", Type = "Float", Default = 1.0 },
        { Name = "Pitch", Type = "Float", Default = 1.0 }
    }
}

function NotifyClass:Notify(meshComp, time)
    print(string.format("[Notify_PlaySound] Triggered at time %.2f", time))
    print(string.format("  SoundPath: %s", self.SoundPath or "None"))
    print(string.format("  Volume: %.2f", self.Volume or 1.0))
    print(string.format("  Pitch: %.2f", self.Pitch or 1.0))

    local actor = meshComp:GetOwner()
    if actor then
        local location = actor:GetActorLocation()
        print(string.format("  Actor Location: (%.1f, %.1f, %.1f)", location.X, location.Y, location.Z))

        -- TODO: Actual sound playing logic when UAudioComponent is bound
        -- local audioComp = actor:FindComponentByClass("UAudioComponent")
        -- if audioComp then
        --     audioComp:PlaySoundAtLocation(location, self.SoundPath, self.Volume, self.Pitch)
        -- end
    end
end

return NotifyClass
