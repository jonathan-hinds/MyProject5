// File: GE_Damage.cpp
#include "GE_Damage.h"
#include "AttributeSet.h"
#include "../../Attributes/WoWAttributeSet.h" // Make sure this path is correct

UGE_Damage::UGE_Damage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    
    // Add modifier for health attribute
    FGameplayModifierInfo ModifierInfo;
    ModifierInfo.Attribute = UWoWAttributeSet::GetHealthAttribute(); // Use your attribute's getter
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    
    // Set up this effect to use SetByCaller for magnitude
    // Create the SetByCaller struct
    FSetByCallerFloat SetByCallerDamage;
    SetByCallerDamage.DataTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
    
    // Create the magnitude using the struct
    FGameplayEffectModifierMagnitude MagnitudeByCaller(SetByCallerDamage);
    
    // Assign the magnitude to the modifier
    ModifierInfo.ModifierMagnitude = MagnitudeByCaller;
    
    // Add the modifier to the GameplayEffect
    Modifiers.Add(ModifierInfo);
}