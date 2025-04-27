// WoWAutoAttackAbility.cpp
#include "WoWAutoAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "../Components/TargetingComponent.h"
#include "../Character/WoWCharacterBase.h"
#include "../Character/WoWEnemyCharacter.h"
#include "../Attributes/WoWAttributeSet.h"
#include "GameplayEffectTypes.h"

UWoWAutoAttackAbility::UWoWAutoAttackAbility()
{
    // Use InstancedPerActor for proper state management
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    
    AttackRange = 200.0f;
    AttackSpeed = 1.5f; // Aligned with GCD
    
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
    
    AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    if (!AvatarActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Check if auto-attack is already active
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Ability.AutoAttack.Active"))))
    {
        // Already auto-attacking, toggle off
        GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
        
        // Remove the active tag
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Ability.AutoAttack.Active")));
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Auto-attack stopped"));
        }
        
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    
    // Check if we have a valid enemy target before starting
    AActor* TargetActor = GetCurrentTarget();
    if (!TargetActor || !CanAttackTarget(TargetActor))
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("No valid enemy target"));
        }
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    
    // Start auto-attack
    ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Ability.AutoAttack.Active")));
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Auto-attack started"));
    }
    
    // Perform the first attack immediately
    PerformAutoAttack();
    
    // Set up timer for subsequent attacks
    GetWorld()->GetTimerManager().SetTimer(
        AutoAttackTimerHandle,
        this,
        &UWoWAutoAttackAbility::PerformAutoAttack,
        AttackSpeed,
        true // Loop
    );
}

void UWoWAutoAttackAbility::PerformAutoAttack()
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    AActor* TargetActor = GetCurrentTarget();
    
    // Check if target is still valid and is an enemy
    if (!AvatarActor || !TargetActor || !IsValid(TargetActor) || !CanAttackTarget(TargetActor))
    {
        // No valid target or target is not an enemy, end auto-attack
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
        return;
    }
    
    float DistanceToTarget = FVector::Dist(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
    if (DistanceToTarget > AttackRange)
    {
        // Target out of range - don't end ability, just skip this attack
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, TEXT("Auto-attack target out of range"));
        }
        return;
    }
    
    // Calculate damage using character attributes
    AWoWCharacterBase* OwnerCharacter = Cast<AWoWCharacterBase>(AvatarActor);
    float DamageAmount = 0.0f;
    
    if (OwnerCharacter && OwnerCharacter->GetAttributeSet())
    {
        UWoWAttributeSet* AttributeSet = Cast<UWoWAttributeSet>(OwnerCharacter->GetAttributeSet());
        if (AttributeSet)
        {
            // Get attack power from strength
            float AttackPower = AttributeSet->GetStrength() * 2.0f;
            
            // Base weapon damage
            float BaseWeaponDamage = 5.0f + (AttributeSet->GetStrength() * 0.5f);
            
            // Calculate actual damage
            DamageAmount = BaseWeaponDamage + (AttackPower / 14.0f);
            
            // Check for critical strike
            float CritChance = AttributeSet->GetCriticalStrikeChance();
            bool IsCriticalHit = (FMath::RandRange(0.0f, 100.0f) <= CritChance);
            
            if (IsCriticalHit)
            {
                DamageAmount *= 2.0f;
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, 
                        FString::Printf(TEXT("Critical Hit! %.0f damage"), DamageAmount));
                }
            }
            else if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, 
                    FString::Printf(TEXT("Hit for %.0f damage"), DamageAmount));
            }
        }
    }
    
    // Apply damage
    ApplyDamageToTarget(TargetActor, DamageAmount);
}

void UWoWAutoAttackAbility::ApplyDamageToTarget(AActor* TargetActor, float DamageAmount)
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
    FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
    EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
    
    // Create effect spec
    FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(DamageEffect, 1, EffectContext);
    
    if (SpecHandle.IsValid())
    {
        // Set damage value using "Data.Damage" tag
        FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
        SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -DamageAmount);
        
        // Apply effect to target
        GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
    }
}

void UWoWAutoAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    // Clear the timer
    GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
    
    // Remove active tag
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (ASC)
    {
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Ability.AutoAttack.Active")));
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AActor* UWoWAutoAttackAbility::GetCurrentTarget() const
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        return nullptr;
    }
    
    UTargetingComponent* TargetingComp = AvatarActor->FindComponentByClass<UTargetingComponent>();
    if (TargetingComp)
    {
        return TargetingComp->GetCurrentTarget();
    }
    
    return nullptr;
}

bool UWoWAutoAttackAbility::CanAttackTarget(AActor* Target) const
{
    if (!Target)
    {
        return false;
    }
    
    // Check if target is an enemy character
    AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(Target);
    return EnemyCharacter != nullptr;
}

bool UWoWAutoAttackAbility::IsAutoAttackActive(UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        return false;
    }
    
    return ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Ability.AutoAttack.Active")));
}