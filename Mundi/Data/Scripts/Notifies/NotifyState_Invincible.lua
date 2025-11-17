-- NotifyState_Invincible.lua
-- Makes the character invincible during the animation

NotifyStateClass = {
    DisplayName = "Invincible State",
    Description = "Makes character invincible during animation",

    Properties = {
        { Name = "EffectColor", Type = "Color", Default = {R = 1.0, G = 1.0, B = 0.0, A = 0.5} },
        { Name = "ShowEffect", Type = "Bool", Default = true }
    }
}

function NotifyStateClass:NotifyBegin(meshComp, time)
    print(string.format("[NotifyState_Invincible] Begin at time %.2f", time))

    local actor = meshComp:GetOwner()
    if actor then
        -- Store original state
        self.OriginalInvincible = actor.bIsInvincible or false

        -- Set invincible
        actor.bIsInvincible = true
        print("  Character is now INVINCIBLE")

        if self.ShowEffect then
            -- TODO: Spawn visual effect
            -- self.EffectComponent = SpawnVisualEffect(actor, self.EffectColor)
            print(string.format("  Effect Color: (%.2f, %.2f, %.2f, %.2f)",
                self.EffectColor.R, self.EffectColor.G, self.EffectColor.B, self.EffectColor.A))
        end
    end
end

function NotifyStateClass:NotifyTick(meshComp, time, deltaTime)
    -- Update effect per frame if needed
    -- print(string.format("[NotifyState_Invincible] Tick at time %.2f", time))
end

function NotifyStateClass:NotifyEnd(meshComp, time)
    print(string.format("[NotifyState_Invincible] End at time %.2f", time))

    local actor = meshComp:GetOwner()
    if actor then
        -- Restore original state
        actor.bIsInvincible = self.OriginalInvincible or false
        print("  Character invincibility ENDED")

        if self.ShowEffect then
            -- TODO: Destroy visual effect
            -- if self.EffectComponent then
            --     self.EffectComponent:Destroy()
            --     self.EffectComponent = nil
            -- end
        end
    end
end

return NotifyStateClass
