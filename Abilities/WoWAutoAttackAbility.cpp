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
    WeaponBaseSpeed = 10.0f;  // Base weapon speed is 10 seconds (for 1 agility)
    MinAttackSpeed = 1.5f;    // Minimum attack speed with maximum haste
    
    // Set tags
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
}

void UWoWAutoAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
                                          const FGameplayAbilityActorInfo* ActorInfo, 
                                          const FGameplayAbilityActivationInfo ActivationInfo, 
                                          const FGameplayEventData* TriggerEventData)
{
    // Log to verify activation happens only once per request
    UE_LOG(LogTemp, Warning, TEXT("Auto-attack ability activating - Role: %s"), 
           ActorInfo->IsLocallyControlled() ? TEXT("Local") : TEXT("Remote"));

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
    FGameplayTag AutoAttackTag = FGameplayTag::RequestGameplayTag(FName("Ability.AutoAttack.Active"));
    
    // If we already have the tag, we're toggling OFF
    if (ASC && ASC->HasMatchingGameplayTag(AutoAttackTag))
    {
        // Stop auto-attack
        GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
        ASC->RemoveLooseGameplayTag(AutoAttackTag);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Auto-attack stopped"));
        }
        
        // Find targeting component to clean up state
        UTargetingComponent* TargetingComp = AvatarActor->FindComponentByClass<UTargetingComponent>();
        if (TargetingComp)
        {
            TargetingComp->StopAutoAttack();
        }
        
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    
    // Add the active tag to mark auto-attack as on
    ASC->AddLooseGameplayTag(AutoAttackTag);
    
    // Find targeting component to validate target
    UTargetingComponent* TargetingComp = AvatarActor->FindComponentByClass<UTargetingComponent>();
    if (!TargetingComp || !TargetingComp->HasValidTarget())
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("No valid target for auto-attack"));
        }
        
        // Clean up and end
        ASC->RemoveLooseGameplayTag(AutoAttackTag);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Get the owner's haste multiplier
    float HasteMultiplier = 1.0f;
    AWoWCharacterBase* OwnerCharacter = Cast<AWoWCharacterBase>(AvatarActor);
    if (OwnerCharacter)
    {
        HasteMultiplier = OwnerCharacter->GetHasteMultiplier();
    }
    
    // Apply the haste multiplier to the base weapon speed
    float AdjustedAttackSpeed = WeaponBaseSpeed * HasteMultiplier;
    
    // Ensure we don't go below minimum attack speed
    AdjustedAttackSpeed = FMath::Max(AdjustedAttackSpeed, MinAttackSpeed);
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, 
            FString::Printf(TEXT("Attack Speed: %.2fs (Haste: %.2f)"), 
                           AdjustedAttackSpeed, HasteMultiplier));
    }
    
    // Perform the first attack immediately, but only on authority
    if (ActorInfo->IsNetAuthority())
    {
        PerformAutoAttack();
    }
    
    // Set up timer for subsequent attacks
    GetWorld()->GetTimerManager().SetTimer(
        AutoAttackTimerHandle,
        this,
        &UWoWAutoAttackAbility::PerformAutoAttack,
        AdjustedAttackSpeed,
        true // Loop
    );
}

void UWoWAutoAttackAbility::PerformAutoAttack()
{
    // Add a static counter to track executions
    static int32 AutoAttackExecutionCount = 0;
    int32 ThisAttackNumber = ++AutoAttackExecutionCount;
    
    // Only perform auto-attack on authority to prevent duplicate damage
    if (!IsLocallyControlled() && !GetAvatarActorFromActorInfo()->HasAuthority())
    {
        UE_LOG(LogTemp, Verbose, TEXT("Auto-attack #%d skipped - not authority"), ThisAttackNumber);
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Auto-attack #%d executing on %s"), 
           ThisAttackNumber, 
           GetAvatarActorFromActorInfo()->HasAuthority() ? TEXT("server") : TEXT("client"));
    
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    
    // Find targeting component
    UTargetingComponent* TargetingComp = AvatarActor ? AvatarActor->FindComponentByClass<UTargetingComponent>() : nullptr;
    if (!TargetingComp)
    {
        UE_LOG(LogTemp, Error, TEXT("Auto-attack #%d failed - no targeting component"), ThisAttackNumber);
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
        return;
    }
    
    // Get current target
    AActor* TargetActor = TargetingComp->GetCurrentTarget();
    
    // Check if target is still valid and is an enemy
    if (!TargetActor || !IsValid(TargetActor) || !CanAttackTarget(TargetActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("Auto-attack #%d ended - no valid target"), ThisAttackNumber);
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
        return;
    }
    
    float DistanceToTarget = FVector::Dist(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
    if (DistanceToTarget > AttackRange)
    {
        UE_LOG(LogTemp, Verbose, TEXT("Auto-attack #%d skipped - target out of range"), ThisAttackNumber);
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
                    FString::Printf(TEXT("[%d] Hit for %.0f damage"), ThisAttackNumber, DamageAmount));
            }
        }
    }
    
    // Apply damage - log before and after to track execution flow
    UE_LOG(LogTemp, Warning, TEXT("Auto-attack #%d applying %.0f damage"), ThisAttackNumber, DamageAmount);
    ApplyDamageToTarget(TargetActor, DamageAmount);
    UE_LOG(LogTemp, Warning, TEXT("Auto-attack #%d damage application complete"), ThisAttackNumber);
    
    // Update targeting component's attack-in-progress state
    if (TargetingComp)
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick([TargetingComp]() {
            TargetingComp->ClearAttackInProgressFlag();
        });
    }
}

void UWoWAutoAttackAbility::ApplyDamageToTarget(AActor* TargetActor, float DamageAmount)
{
    // Add unique ID to track each damage application
    static int32 DamageApplicationCounter = 0;
    int32 ThisDamageID = ++DamageApplicationCounter;
    
    UE_LOG(LogTemp, Warning, TEXT("Damage Application #%d START - %.0f damage to %s"), 
           ThisDamageID, DamageAmount, *TargetActor->GetName());
    
    if (!TargetActor || !DamageEffect)
    {
        UE_LOG(LogTemp, Error, TEXT("Damage Application #%d FAILED - invalid target or effect"), ThisDamageID);
        return;
    }
    
    // Final authority check
    if (!GetAvatarActorFromActorInfo()->HasAuthority())
    {
        UE_LOG(LogTemp, Error, TEXT("Damage Application #%d SKIPPED - not server"), ThisDamageID);
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
        UE_LOG(LogTemp, Error, TEXT("Damage Application #%d FAILED - no target ASC"), ThisDamageID);
        return;
    }
    
    // Create effect context
    FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
    EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
    
    // Create effect spec
    FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
        DamageEffect, 1, EffectContext);
    
    if (SpecHandle.IsValid())
    {
        // Set damage value using "Data.Damage" tag
        FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
        SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -DamageAmount);
        
        UE_LOG(LogTemp, Warning, TEXT("Damage Application #%d applying effect to target"), ThisDamageID);
        
        // Apply effect to target and capture result
        FActiveGameplayEffectHandle ActiveHandle = GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
            *SpecHandle.Data.Get(), TargetASC);
            
        if (ActiveHandle.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Damage Application #%d SUCCESS"), ThisDamageID);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Damage Application #%d FAILED - effect not applied"), ThisDamageID);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Damage Application #%d FAILED - invalid effect spec"), ThisDamageID);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Damage Application #%d COMPLETE"), ThisDamageID);
}

void UWoWAutoAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, 
                                    const FGameplayAbilityActorInfo* ActorInfo, 
                                    const FGameplayAbilityActivationInfo ActivationInfo, 
                                    bool bReplicateEndAbility, 
                                    bool bWasCancelled)
{
    UE_LOG(LogTemp, Warning, TEXT("Auto-attack ability ending - Was Cancelled: %s"), 
           bWasCancelled ? TEXT("Yes") : TEXT("No"));
    
    // Clear the timer
    GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
    
    // Remove active tag
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (ASC)
    {
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Ability.AutoAttack.Active")));
    }
    
    // Find and update targeting component
    AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    if (AvatarActor)
    {
        UTargetingComponent* TargetingComp = AvatarActor->FindComponentByClass<UTargetingComponent>();
        if (TargetingComp)
        {
            // Ensure attack-in-progress flag is cleared
            TargetingComp->ClearAttackInProgressFlag();
            
            // If the ability was cancelled, also update auto-attack state
            if (bWasCancelled)
            {
                TargetingComp->StopAutoAttack();
            }
        }
    }
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    UE_LOG(LogTemp, Warning, TEXT("Auto-attack ability ended"));
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