-- Notify_CameraShake.lua
-- Triggers a camera shake effect

NotifyClass = {
    DisplayName = "Camera Shake",
    Description = "Triggers a camera shake effect",

    Properties = {
        { Name = "Intensity", Type = "Float", Default = 1.0 },
        { Name = "Duration", Type = "Float", Default = 0.5 },
        { Name = "Radius", Type = "Float", Default = 1000.0 }
    }
}

function NotifyClass:Notify(meshComp, time)
    print(string.format("[Notify_CameraShake] Triggered at time %.2f", time))
    print(string.format("  Intensity: %.2f", self.Intensity or 1.0))
    print(string.format("  Duration: %.2f", self.Duration or 0.5))
    print(string.format("  Radius: %.2f", self.Radius or 1000.0))

    local actor = meshComp:GetOwner()
    if actor then
        local location = actor:GetActorLocation()

        -- TODO: Actual camera shake logic
        -- GetWorld():TriggerCameraShake(location, self.Intensity, self.Duration, self.Radius)
    end
end

return NotifyClass
