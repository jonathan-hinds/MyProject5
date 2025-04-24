#include "WoWEnemyAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "../Character/WoWEnemyCharacter.h"
#include "../Character/WoWPlayerCharacter.h"
#include "../States/WoWPlayerState.h"
#include "../AI/WoWEnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"

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
    
    UE_LOG(LogTemp, Warning, TEXT("Enemy Attack Ability activated by %s"), *SourceActor->GetName());
    
    // Find target via AI blackboard if this is an AI character
    AActor* TargetActor = nullptr;
    AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(SourceActor);
    if (EnemyCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Source is a WoWEnemyCharacter"));
        
        // Get the AI controller
        AWoWEnemyController* EnemyController = Cast<AWoWEnemyController>(EnemyCharacter->GetController());
        if (EnemyController && EnemyController->GetBlackboardComponent())
        {
            UE_LOG(LogTemp, Warning, TEXT("Found EnemyController with Blackboard"));
            
            // Get target from blackboard - use the same key name as in BTTask_PerformAttack
            TargetActor = Cast<AActor>(EnemyController->GetBlackboardComponent()->GetValueAsObject("TargetActor"));
            
            if (TargetActor)
            {
                UE_LOG(LogTemp, Warning, TEXT("Found target from blackboard: %s"), *TargetActor->GetName());
            }
        }
    }
    
    // Fallback to finding nearest player if no target found via blackboard
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("No target from blackboard, searching for nearest player"));
        
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
        
        if (TargetActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("Found nearest player target: %s"), *TargetActor->GetName());
        }
    }
    
    // If we have a target, calculate and apply damage based on stats
    if (TargetActor)
    {
        // Calculate damage based on enemy stats
        float DamageAmount = 10.0f; // Default fallback value
        
        AWoWEnemyCharacter* EnemySource = Cast<AWoWEnemyCharacter>(SourceActor);
        if (EnemySource)
        {
            // Use the enemy's base damage calculation
            DamageAmount = EnemySource->CalculateBaseDamage() * DamageCoefficient;
            UE_LOG(LogTemp, Warning, TEXT("Calculated damage: %.2f"), DamageAmount);
        }
        
        // Apply the damage
        ApplyDamageToTarget(Handle, ActorInfo, ActivationInfo, TargetActor, DamageAmount);
        
        // Apply cooldown
        if (CooldownDuration > 0.0f)
        {
            FGameplayTagContainer CooldownTags;
            CooldownTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
            
            // Just call the base ApplyCooldown method
            ApplyCooldown(Handle, ActorInfo, ActivationInfo);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No target found for attack!"));
    }
    
    // End the ability
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWoWEnemyAttackAbility::ApplyDamageToTarget(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, AActor* TargetActor, float DamageAmount)
{
    // Debug: Log the target and damage amount
    UE_LOG(LogTemp, Warning, TEXT("==== ENEMY ATTACK DEBUG ===="));
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
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is not a WoWCharacterBase!"));
    }
    
    // Get the correct ASC based on target type
    UAbilitySystemComponent* TargetASC = nullptr;
    
    // Check if it's a player character first
    AWoWPlayerCharacter* PlayerTarget = Cast<AWoWPlayerCharacter>(TargetActor);
    if (PlayerTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is a WoWPlayerCharacter, getting ASC from PlayerState"));
        AWoWPlayerState* PS = PlayerTarget->GetPlayerState<AWoWPlayerState>();
        if (PS)
        {
            TargetASC = PS->GetAbilitySystemComponent();
            UE_LOG(LogTemp, Warning, TEXT("Got ASC from PlayerState: %s"), 
                TargetASC ? TEXT("Success") : TEXT("Failed"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get PlayerState from target"));
        }
    }
    else
    {
        // For non-player targets, use the global function
        UE_LOG(LogTemp, Warning, TEXT("Target is not a player, getting ASC directly"));
        TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
    }
    
    if (!TargetASC)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get AbilitySystemComponent from target %s"), *TargetActor->GetName());
        return;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Found AbilitySystemComponent on target %s"), *TargetActor->GetName());
    }
    
    // Debug the ability system component owner
    AActor* ASCOwner = TargetASC->GetOwner();
    if (ASCOwner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ASC Owner: %s"), *ASCOwner->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ASC has no owner!"));
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
            UE_LOG(LogTemp, Error, TEXT("Data.Damage tag is NOT valid! Create this tag in project settings."));
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
                
                if (FMath::IsNearlyEqual(TargetCharacter->GetHealth(), TargetCharacter->GetMaxHealth()))
                {
                    UE_LOG(LogTemp, Error, TEXT("HEALTH DIDN'T CHANGE! Check if GE_Damage is set up correctly."));
                }
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
    
    UE_LOG(LogTemp, Warning, TEXT("==== END ENEMY ATTACK DEBUG ===="));
}

void UWoWEnemyAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}