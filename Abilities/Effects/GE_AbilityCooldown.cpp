// GE_AbilityCooldown.cpp
#include "GE_AbilityCooldown.h"

UGE_AbilityCooldown::UGE_AbilityCooldown()
{
    // Set the effect to have a duration
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    
    // This is the simplest possible setup for a cooldown effect
    // You'll set the actual duration at runtime via SetByCaller
    DurationMagnitude = FScalableFloat(5.0f); // Default duration
    
    // Add cooldown tag to the effect
    InheritableOwnedTagsContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Cooldown")));
}