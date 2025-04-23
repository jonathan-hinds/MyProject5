#include "WoWEnemyAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "../Character/WoWEnemyCharacter.h"

UWoWEnemyAttackAbility::UWoWEnemyAttackAbility()
{
    // Default values
    AttackRange = 150.0f;
    DamageCoefficient = 1.0f;
    CooldownDuration = 2.0f;
    
    // Set instant cast policy
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UWoWEnemyAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Try to find a target in range
    AActor* SourceActor = ActorInfo->AvatarActor.Get();
    if (!SourceActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Get target from TriggerEventData if provided
    AActor* TargetActor = nullptr;
    if (TriggerEventData && TriggerEventData->Target)
    {
        TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
    }
    else
    {
        // Find closest player character in range
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);
        
        float ClosestDistance = AttackRange;
        for (AActor* Actor : FoundActors)
        {
            APawn* ActorAsPawn = Cast<APawn>(Actor);
            if (Actor != SourceActor && ActorAsPawn && ActorAsPawn->IsPlayerControlled())
            {
                float Distance = FVector::Distance(SourceActor->GetActorLocation(), Actor->GetActorLocation());
                if (Distance <= ClosestDistance)
                {
                    ClosestDistance = Distance;
                    TargetActor = Actor;
                }
            }
        }
    }
    
    // If we have a target, calculate and apply damage based on stats
    if (TargetActor)
    {
        // Calculate damage based on enemy stats
        float DamageAmount = 10.0f; // Default fallback value
        
        AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(SourceActor);
        if (EnemyCharacter)
        {
            // Use the enemy's base damage calculation
            DamageAmount = EnemyCharacter->CalculateBaseDamage() * DamageCoefficient;
        }
        
        // Apply the damage
        ApplyDamageToTarget(Handle, ActorInfo, ActivationInfo, TargetActor, DamageAmount);
        
        // Apply cooldown - UE5.4 version
        if (CooldownDuration > 0.0f)
        {
            FGameplayTagContainer CooldownTags;
            CooldownTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
            
            // Just call the base ApplyCooldown method with correct parameters
            ApplyCooldown(Handle, ActorInfo, ActivationInfo);
        }
    }
    
    // End the ability
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWoWEnemyAttackAbility::ApplyDamageToTarget(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, AActor* TargetActor, float DamageAmount)
{
    // Debug: Log the target and damage amount
    UE_LOG(LogTemp, Warning, TEXT("Attempting to apply %.2f damage to %s"), DamageAmount, *TargetActor->GetName());
    
    if (!TargetActor || !DamageEffect)
    {
        UE_LOG(LogTemp, Error, TEXT("Missing target actor or damage effect!"));
        return;
    }
    
    // Try to cast target to character for health checking
    AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(TargetActor);
    if (TargetCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target's health before damage: %.2f / %.2f"), 
            TargetCharacter->GetHealth(), TargetCharacter->GetMaxHealth());
    }
    
    // Use the globals function to get ASC
    UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
    if (!TargetASC)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get AbilitySystemComponent from target %s"), *TargetActor->GetName());
        return;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Found AbilitySystemComponent on target %s"), *TargetActor->GetName());
    }
    
    // Create the effect context
    FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
    EffectContext.AddSourceObject(ActorInfo->AvatarActor.Get());
    
    // Create the effect spec
    FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(DamageEffect, 1, EffectContext);
    
    if (SpecHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Created valid GameplayEffectSpec"));
        
        // Set the damage value using set by caller
        FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
        
        // Debug: Check if the tag exists
        if (DamageTag.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Data.Damage tag is valid"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Data.Damage tag is NOT valid!"));
        }
        
        // Make damage negative since we're reducing health
        SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -DamageAmount);
        UE_LOG(LogTemp, Warning, TEXT("Set damage magnitude to %.2f"), -DamageAmount);
        
        // Apply the effect to the target
        FActiveGameplayEffectHandle AppliedEffect = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
        
        // Check if effect was applied successfully
        if (AppliedEffect.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Successfully applied damage effect"));
            
            // Check health after damage
            if (TargetCharacter)
            {
                UE_LOG(LogTemp, Warning, TEXT("Target's health after damage: %.2f / %.2f"), 
                    TargetCharacter->GetHealth(), TargetCharacter->GetMaxHealth());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to apply damage effect!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create valid GameplayEffectSpec!"));
    }
}

void UWoWEnemyAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}