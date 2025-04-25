// WoWAutoAttackAbility.cpp
#include "WoWAutoAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "../Components/TargetingComponent.h"
#include "../Character/WoWCharacterBase.h"
#include "../Character/WoWPlayerCharacter.h"
#include "../States/WoWPlayerState.h"
#include "../Attributes/WoWAttributeSet.h"
#include "GameplayEffectTypes.h"

UWoWAutoAttackAbility::UWoWAutoAttackAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    
    // Set default values for base weapon damage
    DamageRange = FVector2D(5.0f, 10.0f);
    
    // Set tags
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
}

void UWoWAutoAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Get the owner actor
    AActor* OwnerActor = ActorInfo->AvatarActor.Get();
    if (!OwnerActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Get the targeting component
    UTargetingComponent* TargetingComp = OwnerActor->FindComponentByClass<UTargetingComponent>();
    if (!TargetingComp || !TargetingComp->HasValidTarget())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Get the target
    AActor* TargetActor = TargetingComp->GetCurrentTarget();
    
    // Check if in range before attacking - silent fail if out of range
    float MeleeRange = 200.0f; // Should match the value in TargetingComponent
    float DistanceToTarget = FVector::Dist(OwnerActor->GetActorLocation(), TargetActor->GetActorLocation());
    if (DistanceToTarget > MeleeRange)
    {
        // Too far away, just end the ability without an error
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    
    // Calculate damage - WoW style based on character stats
    AWoWCharacterBase* OwnerCharacter = Cast<AWoWCharacterBase>(OwnerActor);
    float DamageAmount = 0.0f;
    
    if (OwnerCharacter && OwnerCharacter->GetAttributeSet())
    {
        UWoWAttributeSet* AttributeSet = Cast<UWoWAttributeSet>(OwnerCharacter->GetAttributeSet());
        if (AttributeSet)
        {
            // Base weapon damage
            float WeaponBaseDamage = FMath::RandRange(DamageRange.X, DamageRange.Y);
            
            // Get attack power from strength (for melee classes)
            float AttackPower = AttributeSet->GetStrength() * 2.0f;
            
            // Apply WoW-like formula: WeaponDamage + (AP / 14 * WeaponSpeed)
            float WeaponSpeed = 2.0f; // Assume a 2.0 speed weapon
            DamageAmount = WeaponBaseDamage + (AttackPower / 14.0f);
            
            // Optional: Check for critical strike based on character's crit chance
            float CritChance = AttributeSet->GetCriticalStrikeChance();
            bool IsCriticalHit = (FMath::RandRange(0.0f, 100.0f) <= CritChance);
            
            if (IsCriticalHit)
            {
                DamageAmount *= 2.0f; // Double damage on critical hit
                
                // Optional: Display a "Critical Hit!" message
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, 
                        FString::Printf(TEXT("Critical Hit! %.0f damage"), DamageAmount));
                }
            }
            else if (GEngine)
            {
                // Optional: Display normal hit message
                GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, 
                    FString::Printf(TEXT("Hit for %.0f damage"), DamageAmount));
            }
        }
        else
        {
            // Fallback to base damage if attribute set not found
            DamageAmount = FMath::RandRange(DamageRange.X, DamageRange.Y);
        }
    }
    else
    {
        // Fallback to base damage if character not found
        DamageAmount = FMath::RandRange(DamageRange.X, DamageRange.Y);
    }
    
    // Apply damage to target
    ApplyDamageToTarget(Handle, ActorInfo, ActivationInfo, TargetActor, DamageAmount);
    
    // End ability
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWoWAutoAttackAbility::ApplyDamageToTarget(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, AActor* TargetActor, float DamageAmount)
{
    if (!TargetActor || !DamageEffect)
    {
        return;
    }
    
    // Get target's ASC using the interface approach
    UAbilitySystemComponent* TargetASC = nullptr;
    
    // Try to get ASC from the interface
    IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor);
    if (ASCInterface)
    {
        TargetASC = ASCInterface->GetAbilitySystemComponent();
    }
    
    if (!TargetASC)
    {
        return;
    }
    
    // Create effect context
    FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
    EffectContext.AddSourceObject(ActorInfo->AvatarActor.Get());
    
    // Create effect spec
    FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(DamageEffect, 1, EffectContext);
    
    if (SpecHandle.IsValid())
    {
        // Set damage value using "Data.Damage" tag
        FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
        SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -DamageAmount);
        
        // Apply effect to target
        ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
    }
}