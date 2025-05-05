// WoWGameplayAbilityBase.cpp
#include "WoWGameplayAbilityBase.h"
#include "../Components/EffectApplicationComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "../Data/EffectDataAsset.h"
#include "../Character/WoWCharacterBase.h"
#include "../Character/WoWPlayerCharacter.h"
#include "../Components/TargetingComponent.h"
#include "../Attributes/WoWAttributeSet.h" // Critical include!

UWoWGameplayAbilityBase::UWoWGameplayAbilityBase()
{
    AbilityID = 0;
    bAbilityDataLoaded = false;
    
    // Find or create a reference to the cooldown effect
    static ConstructorHelpers::FClassFinder<UGameplayEffect> DefaultCooldownGEClass(TEXT("/Game/Abilities/Effects/GE_AbilityCooldown_Base"));
    if (DefaultCooldownGEClass.Succeeded())
    {
        CooldownGameplayEffect = DefaultCooldownGEClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("Found default cooldown effect: %s"), 
            *CooldownGameplayEffect->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find default cooldown effect at path: /Game/Abilities/Effects/GE_AbilityCooldown_Base"));
    }
    
    // Set up default instancing policy
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UWoWGameplayAbilityBase::GetAbilityData(FAbilityTableRow& OutAbilityData) const
{
    if (bAbilityDataLoaded)
    {
        return true;
    }
    
    // If not already loaded, try to load it
    if (AbilityDataAsset)
    {
        bool bFound = AbilityDataAsset->GetAbilityDataByID(AbilityID, OutAbilityData);
        if (!bFound)
        {
            // NEW: Debug the data table contents
            TArray<FAbilityTableRow*> AllRows;
            AbilityDataAsset->AbilityDataTable->GetAllRows(TEXT("GetAbilityData"), AllRows);
            
            UE_LOG(LogTemp, Warning, TEXT("Available ability IDs in data table:"));
            for (FAbilityTableRow* Row : AllRows)
            {
                UE_LOG(LogTemp, Warning, TEXT("  ID: %d, Name: %s"), Row->AbilityID, *Row->DisplayName);
            }
        }
        return bFound;
    }
    
    return false;
}

void UWoWGameplayAbilityBase::InitializeFromAbilityData(int32 InAbilityID)
{
    AbilityID = InAbilityID;
    bAbilityDataLoaded = false;
    
    // Try to load the ability data immediately
    FAbilityTableRow AbilityData;
    GetAbilityData(AbilityData);
}

UEffectApplicationComponent* UWoWGameplayAbilityBase::GetEffectComponent() const
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        return nullptr;
    }
    
    // Try to find existing component
    UEffectApplicationComponent* EffectComp = AvatarActor->FindComponentByClass<UEffectApplicationComponent>();
    
    // If no component found, create one
    if (!EffectComp)
    {
        // We need to cast away const here
        UWoWGameplayAbilityBase* MutableThis = const_cast<UWoWGameplayAbilityBase*>(this);
        AActor* MutableActor = const_cast<AActor*>(AvatarActor);
        
        EffectComp = NewObject<UEffectApplicationComponent>(MutableActor);
        EffectComp->RegisterComponent();
    }
    
    return EffectComp;
}

void UWoWGameplayAbilityBase::EndAbility(const FGameplayAbilitySpecHandle Handle, 
                                       const FGameplayAbilityActorInfo* ActorInfo, 
                                       const FGameplayAbilityActivationInfo ActivationInfo, 
                                       bool bReplicateEndAbility, bool bWasCancelled)
{
    UE_LOG(LogTemp, Warning, TEXT("===== ENDING ABILITY %d ====="), AbilityID);
    UE_LOG(LogTemp, Warning, TEXT("Was Cancelled: %s"), bWasCancelled ? TEXT("YES") : TEXT("NO"));
    
    if (!bWasCancelled && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        // Get ability data for cooldown info
        FAbilityTableRow AbilityData;
        if (GetAbilityData(AbilityData) && AbilityData.Cooldown > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Applying cooldown of %.1f seconds to ability ID: %d"), 
                AbilityData.Cooldown, AbilityID);
            
            if (CooldownGameplayEffect)
            {
                UE_LOG(LogTemp, Warning, TEXT("Using cooldown effect: %s"), *CooldownGameplayEffect->GetName());
                
                // Use GAS's built-in cooldown handling
                FGameplayTagContainer CooldownTags;
                FGameplayTag CooldownTag = GetCooldownTag();
                CooldownTags.AddTag(CooldownTag);
                
                UE_LOG(LogTemp, Warning, TEXT("Cooldown Tag: %s"), *CooldownTag.ToString());
                
                // Create the effect spec
                FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
                    CooldownGameplayEffect, GetAbilityLevel());
                
                if (SpecHandle.IsValid())
                {
                    UE_LOG(LogTemp, Warning, TEXT("Created valid spec handle"));
                    
                    // DEBUGGING: Check if the spec's GameplayEffect is valid
                    if (SpecHandle.Data->Def)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Effect definition is valid: %s"), *SpecHandle.Data->Def->GetName());
                        UE_LOG(LogTemp, Warning, TEXT("Duration Policy: %d"), (int32)SpecHandle.Data->Def->DurationPolicy);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Effect definition is NULL!"));
                    }
                    
                    // Set the cooldown duration
                    FGameplayTag DurationTag = FGameplayTag::RequestGameplayTag(FName("Data.Cooldown"));
                    SpecHandle.Data->SetSetByCallerMagnitude(DurationTag, AbilityData.Cooldown);
                    
                    UE_LOG(LogTemp, Warning, TEXT("Set cooldown duration to %.1f using tag: %s"), 
                        AbilityData.Cooldown, *DurationTag.ToString());
                    
                    // Add the cooldown tags
                    SpecHandle.Data->DynamicGrantedTags.AppendTags(CooldownTags);
                    
                    UE_LOG(LogTemp, Warning, TEXT("Added cooldown tags to spec:"));
                    for (const FGameplayTag& Tag : SpecHandle.Data->DynamicGrantedTags)
                    {
                        UE_LOG(LogTemp, Warning, TEXT(" - %s"), *Tag.ToString());
                    }
                    
                    // Apply the cooldown
                    FActiveGameplayEffectHandle ActiveGEHandle = ApplyGameplayEffectSpecToOwner(
                        Handle, ActorInfo, ActivationInfo, SpecHandle);
                    
                    if (ActiveGEHandle.IsValid())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Successfully applied cooldown effect with handle: %s"), 
                            *ActiveGEHandle.ToString());
                            
                        // Set a debug timer to check if the cooldown is removed properly
                        UWorld* World = GetWorld();
                        if (World)
                        {
                            FTimerHandle DebugTimerHandle;
                            FTimerDelegate TimerDelegate;
                            
                            // Store a weak pointer to the ASC
                            TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ActorInfo->AbilitySystemComponent.Get();
                            int32 StoredAbilityID = AbilityID;
                            FGameplayTag StoredCooldownTag = CooldownTag;
                            
                            TimerDelegate.BindLambda([StoredAbilityID, StoredCooldownTag, WeakASC]()
                            {
                                if (WeakASC.IsValid())
                                {
                                    bool bStillHasTag = WeakASC->HasMatchingGameplayTag(StoredCooldownTag);
                                    UE_LOG(LogTemp, Warning, TEXT("Cooldown timer check for ability %d: Has tag: %s"), 
                                        StoredAbilityID, bStillHasTag ? TEXT("YES") : TEXT("NO"));
                                        
                                    // Log all active GE handles
                                    TArray<FActiveGameplayEffectHandle> ActiveHandles = WeakASC->GetActiveEffects(FGameplayEffectQuery());
                                    UE_LOG(LogTemp, Warning, TEXT("Number of active effects: %d"), ActiveHandles.Num());
                                }
                            });
                            
                            // Check halfway through cooldown
                            World->GetTimerManager().SetTimer(DebugTimerHandle, TimerDelegate, 
                                AbilityData.Cooldown / 2.0f, false);
                                
                            // Check again after cooldown should have expired
                            FTimerHandle EndTimerHandle;
                            World->GetTimerManager().SetTimer(EndTimerHandle, TimerDelegate, 
                                AbilityData.Cooldown + 0.5f, false);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to apply cooldown effect to ability %s"), 
                            *AbilityData.DisplayName);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to create valid cooldown effect spec"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No cooldown GameplayEffect class set for ability %s"), 
                    *AbilityData.DisplayName);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No cooldown to apply for this ability or failed to get ability data"));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Calling Super::EndAbility"));
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogTemp, Warning, TEXT("===== END ABILITY COMPLETE ====="));
}

FGameplayTag UWoWGameplayAbilityBase::GetCooldownTag() const
{
    // Get ability data
    FAbilityTableRow AbilityData;
    if (GetAbilityData(AbilityData) && AbilityData.CooldownTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Using ability-specific cooldown tag: %s"), *AbilityData.CooldownTag.ToString());
        return AbilityData.CooldownTag;
    }
    
    // Fallback to a generic cooldown tag based on ID
    FGameplayTag FallbackTag = FGameplayTag::RequestGameplayTag(FName(FString::Printf(TEXT("Ability.Cooldown.ID.%d"), AbilityID)));
    UE_LOG(LogTemp, Warning, TEXT("Using fallback cooldown tag: %s"), *FallbackTag.ToString());
    return FallbackTag;
}

bool UWoWGameplayAbilityBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, 
                                               const FGameplayAbilityActorInfo* ActorInfo, 
                                               const FGameplayTagContainer* SourceTags, 
                                               const FGameplayTagContainer* TargetTags, 
                                               OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    UE_LOG(LogTemp, Warning, TEXT("===== CAN ACTIVATE ABILITY %d ====="), AbilityID);
    
    // First check if parent class allows activation
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        UE_LOG(LogTemp, Warning, TEXT("Super::CanActivateAbility returned false"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Super::CanActivateAbility returned true"));
    
    // Check if ability is on cooldown
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        FGameplayTag CooldownTag = GetCooldownTag();
        UE_LOG(LogTemp, Warning, TEXT("Checking for cooldown tag: %s"), *CooldownTag.ToString());
        
        // DEBUG: Log all tags on the ASC
        FGameplayTagContainer AllTags;
        ActorInfo->AbilitySystemComponent->GetOwnedGameplayTags(AllTags);
        UE_LOG(LogTemp, Warning, TEXT("All tags on ASC (%d tags):"), AllTags.Num());
        for (const FGameplayTag& Tag : AllTags)
        {
            UE_LOG(LogTemp, Warning, TEXT(" - %s"), *Tag.ToString());
        }
        
        bool bHasCooldownTag = ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CooldownTag);
        UE_LOG(LogTemp, Warning, TEXT("Has cooldown tag: %s"), bHasCooldownTag ? TEXT("YES") : TEXT("NO"));
        
        if (bHasCooldownTag)
        {
            // Get remaining cooldown time for debugging
            TArray<FActiveGameplayEffectHandle> ActiveEffectHandles = 
                ActorInfo->AbilitySystemComponent->GetActiveEffects(FGameplayEffectQuery());
            
            UE_LOG(LogTemp, Warning, TEXT("Active effects: %d"), ActiveEffectHandles.Num());
            
            float TimeRemaining = 0.0f;
            bool bFoundCooldownEffect = false;
            
            // Check each active effect for ones that grant our cooldown tag
            for (const FActiveGameplayEffectHandle& EffectHandle : ActiveEffectHandles)
            {
                const FActiveGameplayEffect* ActiveEffect = ActorInfo->AbilitySystemComponent->GetActiveGameplayEffect(EffectHandle);
                if (ActiveEffect)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Found active effect: %s"), 
                        ActiveEffect->Spec.Def ? *ActiveEffect->Spec.Def->GetName() : TEXT("NULL Def"));
                    
                    if (ActiveEffect->Spec.Def && ActiveEffect->Spec.DynamicGrantedTags.HasTag(CooldownTag))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("This effect grants our cooldown tag"));
                        
                        // Get the remaining time
                        TimeRemaining = ActiveEffect->GetTimeRemaining(ActorInfo->AbilitySystemComponent->GetWorld()->GetTimeSeconds());
                        
                        UE_LOG(LogTemp, Warning, TEXT("Remaining time: %.2f seconds"), TimeRemaining);
                        UE_LOG(LogTemp, Warning, TEXT("Effect handle: %s"), *EffectHandle.ToString());
                        
                        bFoundCooldownEffect = true;
                        break;
                    }
                }
            }
            
            // If we found the tag but no matching effect, that's suspicious
            if (!bFoundCooldownEffect)
            {
                UE_LOG(LogTemp, Error, TEXT("Found cooldown tag but no matching effect - this is an error!"));
            }
                
            UE_LOG(LogTemp, Warning, TEXT("Ability %d (%s) on cooldown. %.1f seconds remaining."), 
                   AbilityID, *CooldownTag.ToString(), TimeRemaining);
            
            // Display cooldown message
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, 
                    FString::Printf(TEXT("%s on cooldown (%.1f sec)"), 
                    *CooldownTag.ToString(), TimeRemaining));
            }
            
            UE_LOG(LogTemp, Warning, TEXT("===== ABILITY BLOCKED BY COOLDOWN ====="));
            return false;
        }
    }
    
    // Get ability data
    FAbilityTableRow AbilityData;
    bool bGotData = GetAbilityData(AbilityData);
    
    // If no data, fall back to default behavior
    if (!bGotData)
    {
        return true;
    }
    
    // Check if this ability requires a target
    bool bRequiresTarget = (AbilityData.AbilityType == EAbilityType::Target || 
                           AbilityData.AbilityType == EAbilityType::Cast);
    
    if (bRequiresTarget)
    {
        // Validate target existence
        if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
        {
            return false;
        }
        
        // Get player character
        AWoWPlayerCharacter* PlayerCharacter = Cast<AWoWPlayerCharacter>(ActorInfo->AvatarActor.Get());
        if (!PlayerCharacter)
        {
            return false;
        }
        
        // Check targeting component
        UTargetingComponent* TargetingComp = PlayerCharacter->FindComponentByClass<UTargetingComponent>();
        if (!TargetingComp || !TargetingComp->HasValidTarget())
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("This ability requires a target."));
            }
            UE_LOG(LogTemp, Warning, TEXT("%s requires a target, but none is selected"), *GetName());
            return false;
        }
        
        // Check if target is a player (can't target players with harmful spells)
        AActor* TargetActor = TargetingComp->GetCurrentTarget();
        if (TargetActor)
        {
            AWoWPlayerCharacter* PlayerTarget = Cast<AWoWPlayerCharacter>(TargetActor);
            if (PlayerTarget && AbilityData.AbilityType != EAbilityType::Friendly)
            {
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Cannot target other players with this ability."));
                }
                UE_LOG(LogTemp, Warning, TEXT("Cannot target other players with %s"), *GetName());
                return false;
            }
        }
    }
    
    // All checks passed
    UE_LOG(LogTemp, Warning, TEXT("===== ABILITY CAN BE ACTIVATED ====="));
    return true;
}

void UWoWGameplayAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
                                            const FGameplayAbilityActorInfo* ActorInfo, 
                                            const FGameplayAbilityActivationInfo ActivationInfo, 
                                            const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("==== ABILITY ACTIVATION STARTED ===="));
    UE_LOG(LogTemp, Warning, TEXT("Attempting to activate ability ID: %d"), AbilityID);
    
    // Try to get AbilityDataAsset if not already set
    if (!AbilityDataAsset && ActorInfo && ActorInfo->AvatarActor.IsValid())
    {
        AWoWCharacterBase* Character = Cast<AWoWCharacterBase>(ActorInfo->AvatarActor.Get());
        if (Character)
        {
            AbilityDataAsset = Character->GetAbilityDataAsset();
        }
    }
    
    // Get ability data
    FAbilityTableRow AbilityData;
    bool bGotData = GetAbilityData(AbilityData);
    
    if (bGotData)
    {
        UE_LOG(LogTemp, Warning, TEXT("Activating ability: %s (ID: %d)"), *AbilityData.DisplayName, AbilityID);
        
        // Check if this has a cast time
        if (AbilityData.CastTime > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Started casting %s with %.1f second cast time"), 
                *AbilityData.DisplayName, AbilityData.CastTime);
            
            // Store ability info for later use
            CurrentSpecHandle = Handle;
            CurrentActorInfo = ActorInfo;
            CurrentActivationInfo = ActivationInfo;
            
            // Start a timer to delay ability execution
            GetWorld()->GetTimerManager().SetTimer(
                CastTimerHandle,
                this,
                &UWoWGameplayAbilityBase::OnCastTimeComplete,
                AbilityData.CastTime,
                false
            );
            
            // Broadcast an event to notify UI/VFX that casting has started
            OnAbilityCastStarted.Broadcast(AbilityData);
            return;
        }
    }
    
    // For instant cast abilities, just commit and execute right away
    if (CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ability committed successfully"));
        
        // Consume mana if we have data
        if (bGotData)
        {
            ConsumeManaCost(AbilityData.ManaCost, ActorInfo);
        }
        
        // Execute the ability
        ExecuteAbility(Handle, ActorInfo, ActivationInfo);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to commit ability"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void UWoWGameplayAbilityBase::ExecuteAbility(const FGameplayAbilitySpecHandle Handle, 
                                           const FGameplayAbilityActorInfo* ActorInfo, 
                                           const FGameplayAbilityActivationInfo ActivationInfo)
{
    // Apply any effects to target or self
    AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    if (!AvatarActor)
    {
        return;
    }
    
    AWoWPlayerCharacter* PlayerCharacter = Cast<AWoWPlayerCharacter>(AvatarActor);
    if (!PlayerCharacter)
    {
        return;
    }
    
    UTargetingComponent* TargetingComp = PlayerCharacter->FindComponentByClass<UTargetingComponent>();
    if (!TargetingComp)
    {
        return;
    }
    
    // Get ability data
    FAbilityTableRow AbilityData;
    bool bGotData = GetAbilityData(AbilityData);
    
    // Check if we have a target
    if (TargetingComp->HasValidTarget())
    {
        AActor* TargetActor = TargetingComp->GetCurrentTarget();
        if (TargetActor)
        {
            // Apply effects to target
            ApplyEffectsToTarget(TargetActor, EEffectContainerType::Target);
        }
    }
    else if (bGotData && AbilityData.SelfEffects.EffectIDs.Num() > 0)
    {
        // Apply self effects if no target but we have self effects
        ApplyEffectsToTarget(PlayerCharacter, EEffectContainerType::Self);
    }
}

void UWoWGameplayAbilityBase::OnCastTimeComplete()
{
    UE_LOG(LogTemp, Warning, TEXT("==== CAST TIME COMPLETED ===="));
    
    if (!CurrentActorInfo || !CurrentSpecHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid actor info or ability handle!"));
        return;
    }
    
    // Double-check CanActivateAbility again to ensure targeting is still valid
    if (!CanActivateAbility(CurrentSpecHandle, CurrentActorInfo, nullptr, nullptr, nullptr))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ability can no longer be activated (target lost?)"));
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
        return;
    }
    
    FAbilityTableRow AbilityData;
    if (GetAbilityData(AbilityData))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cast completed for ability: %s"), *AbilityData.DisplayName);
        
        // Now that cast is complete, commit the ability and apply effects
        if (CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
        {
            UE_LOG(LogTemp, Warning, TEXT("Ability committed successfully after cast"));
            ConsumeManaCost(AbilityData.ManaCost, CurrentActorInfo);
            ExecuteGameplayEffects(CurrentActorInfo);
            
            // Broadcast cast complete event
            OnAbilityCastComplete.Broadcast(AbilityData);
            
            // Only end the ability on the server, not on clients
            if (CurrentActorInfo->IsLocallyControlled() && CurrentActorInfo->AbilitySystemComponent->GetOwnerRole() == ROLE_Authority)
            {
                UE_LOG(LogTemp, Warning, TEXT("Ending ability on server"));
                EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to commit ability after cast completion"));
            EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get ability data after cast completion"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    }
}

void UWoWGameplayAbilityBase::ConsumeManaCost(float ManaCost, const FGameplayAbilityActorInfo* ActorInfo)
{
    if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
    {
        return;
    }
    
    const UWoWAttributeSet* AttributeSet = Cast<UWoWAttributeSet>(ActorInfo->AbilitySystemComponent->GetAttributeSet(UWoWAttributeSet::StaticClass()));
    if (AttributeSet)
    {
        float CurrentMana = AttributeSet->GetMana();
        float NewMana = FMath::Max(0.0f, CurrentMana - ManaCost);
        
        // Use the ASC to modify the attribute instead of directly modifying it
        ActorInfo->AbilitySystemComponent->SetNumericAttributeBase(UWoWAttributeSet::GetManaAttribute(), NewMana);
        
        UE_LOG(LogTemp, Warning, TEXT("Consumed %.1f mana (%.1f -> %.1f)"), 
            ManaCost, CurrentMana, NewMana);
    }
}

void UWoWGameplayAbilityBase::ExecuteGameplayEffects(const FGameplayAbilityActorInfo* ActorInfo)
{
    UE_LOG(LogTemp, Warning, TEXT("==== EXECUTING GAMEPLAY EFFECTS ===="));
    
    // This is where you'd apply all the effects from your ability
    AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    if (!AvatarActor)
    {
        UE_LOG(LogTemp, Error, TEXT("No avatar actor found!"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }
    
    // Get ability data to check ability type
    FAbilityTableRow AbilityData;
    bool bGotData = GetAbilityData(AbilityData);
    
    // Get target through targeting component
    AWoWPlayerCharacter* PlayerCharacter = Cast<AWoWPlayerCharacter>(AvatarActor);
    if (PlayerCharacter)
    {
        UTargetingComponent* TargetingComp = PlayerCharacter->FindComponentByClass<UTargetingComponent>();
        if (TargetingComp)
        {
            if (TargetingComp->HasValidTarget())
            {
                AActor* TargetActor = TargetingComp->GetCurrentTarget();
                if (TargetActor)
                {
                    // Check if target is a player - don't allow targeting players unless it's a friendly spell
                    AWoWPlayerCharacter* PlayerTarget = Cast<AWoWPlayerCharacter>(TargetActor);
                    if (PlayerTarget && bGotData && AbilityData.AbilityType != EAbilityType::Friendly)
                    {
                        // Can't target other players with non-friendly abilities
                        UE_LOG(LogTemp, Warning, TEXT("Cannot target other players with non-friendly abilities"));
                        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
                        return;
                    }
                    
                    // Apply effects to valid target
                    bool Success = ApplyEffectsToTarget(TargetActor, EEffectContainerType::Target);
                    UE_LOG(LogTemp, Warning, TEXT("Apply effects to target result: %s"), 
                           Success ? TEXT("Success") : TEXT("Failed"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Target actor is null despite HasValidTarget returning true!"));
                    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
                }
            }
            else
            {
                // No valid target - check if ability requires a target
                if (bGotData && 
                    (AbilityData.AbilityType == EAbilityType::Target || 
                     AbilityData.AbilityType == EAbilityType::Cast))
                {
                    UE_LOG(LogTemp, Warning, TEXT("This ability requires a target."));
                    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
                    return;
                }
                
                // For non-targeted abilities, apply self effects if any
                if (bGotData && AbilityData.SelfEffects.EffectIDs.Num() > 0)
                {
                    bool Success = ApplyEffectsToTarget(PlayerCharacter, EEffectContainerType::Self);
                    UE_LOG(LogTemp, Warning, TEXT("Apply self effects result: %s"), 
                           Success ? TEXT("Success") : TEXT("Failed"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("No target found and no self effects to apply."));
                    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No targeting component found on player character!"));
            EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Avatar actor is not a player character!"));
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    }
}

void UWoWGameplayAbilityBase::CancelAbility(const FGameplayAbilitySpecHandle Handle, 
                                         const FGameplayAbilityActorInfo* ActorInfo, 
                                         const FGameplayAbilityActivationInfo ActivationInfo,
                                         bool bReplicateCancelAbility)
{
    // Clear cast timer if we're in the middle of casting
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(CastTimerHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(CastTimerHandle);
        
        // Optional: Broadcast cast interrupted event
        FAbilityTableRow AbilityData;
        if (GetAbilityData(AbilityData))
        {
            OnAbilityCastInterrupted.Broadcast(AbilityData);
        }
    }
    
    Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

bool UWoWGameplayAbilityBase::ApplyEffectsToTarget(AActor* TargetActor, EEffectContainerType ContainerType)
{
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: No target actor!"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Applying effects to target: %s"), *TargetActor->GetName());
    
    // Make sure ability data is loaded
    FAbilityTableRow AbilityData;
    if (!GetAbilityData(AbilityData))
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: Failed to get ability data"));
        return false;
    }
    
    // Get the effect container based on type
    const FEffectContainerSpec* EffectContainer = nullptr;
    switch (ContainerType)
    {
        case EEffectContainerType::Self:
            EffectContainer = &AbilityData.SelfEffects;
            UE_LOG(LogTemp, Warning, TEXT("Using SelfEffects container"));
            break;
            
        case EEffectContainerType::Target:
            EffectContainer = &AbilityData.TargetEffects;
            UE_LOG(LogTemp, Warning, TEXT("Using TargetEffects container with %d effects"), AbilityData.TargetEffects.EffectIDs.Num());
            break;
            
        case EEffectContainerType::Area:
            EffectContainer = &AbilityData.AreaEffects;
            UE_LOG(LogTemp, Warning, TEXT("Using AreaEffects container"));
            break;
    }
    
    if (!EffectContainer || EffectContainer->EffectIDs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: No effect IDs in container!"));
        return false;
    }
    
    // Get the effect component
    UEffectApplicationComponent* EffectComp = GetEffectComponent();
    if (!EffectComp)
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: No EffectApplicationComponent found!"));
        return false;
    }
    
    // Check if effect data asset is set
    if (!EffectComp->GetEffectDataAsset())
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: EffectApplicationComponent has no EffectDataAsset!"));
        return false;
    }
    
    // Apply the effects
    bool Success = EffectComp->ApplyEffectContainerToTarget(*EffectContainer, TargetActor, GetAbilityLevel());
    UE_LOG(LogTemp, Warning, TEXT("Effect application result: %s"), Success ? TEXT("Success") : TEXT("Failed"));
    return Success;
}

int32 UWoWGameplayAbilityBase::ApplyAreaEffects(AActor* CenterActor, float Radius, EEffectContainerType ContainerType)
{
    if (!CenterActor)
    {
        return 0;
    }
    
    // Make sure ability data is loaded
    FAbilityTableRow AbilityData;
    if (!GetAbilityData(AbilityData))
    {
        return 0;
    }
    
    // Get the effect container based on type
    const FEffectContainerSpec* EffectContainer = nullptr;
    switch (ContainerType)
    {
        case EEffectContainerType::Self:
            EffectContainer = &AbilityData.SelfEffects;
            break;
            
        case EEffectContainerType::Target:
            EffectContainer = &AbilityData.TargetEffects;
            break;
            
        case EEffectContainerType::Area:
            EffectContainer = &AbilityData.AreaEffects;
            break;
    }
    
    if (!EffectContainer || EffectContainer->EffectIDs.Num() == 0)
    {
        return 0;
    }
    
    // Get the effect component
    UEffectApplicationComponent* EffectComp = GetEffectComponent();
    if (!EffectComp)
    {
        return 0;
    }
    
    // Use radius from ability data if none provided
    if (Radius <= 0.0f && ContainerType == EEffectContainerType::Area)
    {
        Radius = AbilityData.AreaEffectRadius;
    }
    
    // Apply the area effects
    return EffectComp->ApplyAreaEffects(EffectContainer->EffectIDs, CenterActor, Radius, GetAbilityLevel());
}

const FEffectContainerSpec& UWoWGameplayAbilityBase::GetEffectContainer(EEffectContainerType ContainerType) const
{
    static FEffectContainerSpec EmptyContainer;
    
    // Make sure ability data is loaded
    FAbilityTableRow AbilityData;
    if (!GetAbilityData(AbilityData))
    {
        return EmptyContainer;
    }
    
    // Return the appropriate container
    switch (ContainerType)
    {
        case EEffectContainerType::Self:
            return AbilityData.SelfEffects;
            
        case EEffectContainerType::Target:
            return AbilityData.TargetEffects;
            
        case EEffectContainerType::Area:
            return AbilityData.AreaEffects;
            
        default:
            return EmptyContainer;
    }
}