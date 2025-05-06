// WoWGameplayAbilityBase.cpp
#include "WoWGameplayAbilityBase.h"
#include "../Components/EffectApplicationComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h" // Add this for UGameplayTagsManager
#include "GameplayTagsModule.h" // Add this for accessing tag manager
#include "../Data/EffectDataAsset.h"
#include "../Character/WoWCharacterBase.h"
#include "../Character/WoWPlayerCharacter.h"
#include "../Components/TargetingComponent.h"
#include "../Attributes/WoWAttributeSet.h" // Critical include!

UWoWGameplayAbilityBase::UWoWGameplayAbilityBase()
{
    AbilityID = 0;
    bAbilityDataLoaded = false;
    
    // Find the cooldown effect class
    static ConstructorHelpers::FClassFinder<UGameplayEffect> CooldownEffectFinder(TEXT("/Game/Abilities/Effects/GE_AbilityCooldown"));
    if (CooldownEffectFinder.Succeeded())
    {
        CooldownGameplayEffect = CooldownEffectFinder.Class;
        UE_LOG(LogTemp, Warning, TEXT("Found cooldown effect: %s"), *CooldownGameplayEffect->GetName());
    }
    else
    {
        // Try to find a different path if the first one fails
        static ConstructorHelpers::FClassFinder<UGameplayEffect> AlternateFinder(TEXT("/Script/MyProject5.GE_AbilityCooldown"));
        if (AlternateFinder.Succeeded())
        {
            CooldownGameplayEffect = AlternateFinder.Class;
            UE_LOG(LogTemp, Warning, TEXT("Found cooldown effect via alternate path: %s"), *CooldownGameplayEffect->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find cooldown effect class!"));
        }
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
    
    // Skip cooldown if the ability was cancelled
    if (!bWasCancelled && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        // Get ability data for cooldown info
        FAbilityTableRow AbilityData;
        if (GetAbilityData(AbilityData) && AbilityData.Cooldown > 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Applying cooldown for ability %d (%s)"), 
                AbilityID, *AbilityData.DisplayName);
            
            // Get the cooldown tag
            FGameplayTag CooldownTag = AbilityData.CooldownTag.IsValid() ?
                AbilityData.CooldownTag :
                FGameplayTag::RequestGameplayTag(FName("Ability.Cooldown.Spell.Fireball"), true);
            
            if (CooldownTag.IsValid())
            {
                // Apply the tag directly
                FGameplayTagContainer Tags;
                Tags.AddTag(CooldownTag);
                ActorInfo->AbilitySystemComponent->AddLooseGameplayTags(Tags);
                
                UE_LOG(LogTemp, Warning, TEXT("**** DIRECT METHOD: Added cooldown tag %s ****"), 
                    *CooldownTag.ToString());
                
                // Display on-screen message
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
                        FString::Printf(TEXT("COOLDOWN ADDED: %s (%.1fs)"), 
                        *CooldownTag.ToString(), AbilityData.Cooldown));
                }
                
                // Set up a timer to remove the tag
                UWorld* World = ActorInfo->AbilitySystemComponent->GetWorld();
                if (World)
                {
                    // Use a static TMap to store timers for each ability/ASC combo
                    static TMap<void*, TMap<int32, FTimerHandle>> CooldownTimers;
                    
                    // Get or create a map for this ASC
                    TMap<int32, FTimerHandle>& AbilityTimers = 
                        CooldownTimers.FindOrAdd(ActorInfo->AbilitySystemComponent.Get());
                    
                    // Clear any existing timer for this ability
                    FTimerHandle& TimerHandle = AbilityTimers.FindOrAdd(AbilityID);
                    if (TimerHandle.IsValid())
                    {
                        World->GetTimerManager().ClearTimer(TimerHandle);
                    }
                    
                    // Create timer delegate with lambda
                    FTimerDelegate TimerDelegate;
                    
                    // Capture weak pointers to avoid memory issues
                    TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ActorInfo->AbilitySystemComponent;
                    FGameplayTag WeakTag = CooldownTag;
                    
                    TimerDelegate.BindLambda([WeakASC, WeakTag]()
                    {
                        if (WeakASC.IsValid())
                        {
                            // Remove the tag when cooldown expires
                            FGameplayTagContainer TagsToRemove;
                            TagsToRemove.AddTag(WeakTag);
                            WeakASC->RemoveLooseGameplayTags(TagsToRemove);
                            
                            UE_LOG(LogTemp, Warning, TEXT("**** DIRECT METHOD: Removed cooldown tag %s ****"), 
                                *WeakTag.ToString());
                            
                            // Show on-screen message
                            if (GEngine)
                            {
                                GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
                                    FString::Printf(TEXT("COOLDOWN EXPIRED: %s"), 
                                    *WeakTag.ToString()));
                            }
                        }
                    });
                    
                    // Start the timer
                    World->GetTimerManager().SetTimer(
                        TimerHandle, 
                        TimerDelegate, 
                        AbilityData.Cooldown, 
                        false
                    );
                }
                
                // Try to apply using the GE approach as a backup
                if (CooldownGameplayEffect)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Using cooldown effect: %s"), *CooldownGameplayEffect->GetName());
                    
                    // Create the effect context
                    FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
                    EffectContext.AddSourceObject(this);
                    
                    // Create the effect spec
                    FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(
                        CooldownGameplayEffect, 1, EffectContext);
                    
                    if (SpecHandle.IsValid())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Created valid cooldown effect spec"));
                        
                        // Set the cooldown duration
                        FGameplayTag DurationTag = FGameplayTag::RequestGameplayTag(FName("Data.Cooldown"));
                        SpecHandle.Data->SetSetByCallerMagnitude(DurationTag, AbilityData.Cooldown);
                        
                        UE_LOG(LogTemp, Warning, TEXT("Set cooldown duration to %.1f using tag: %s"), 
                            AbilityData.Cooldown, *DurationTag.ToString());
                        
                        // Add the cooldown tag to the granted tags
                        SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
                        
                        UE_LOG(LogTemp, Warning, TEXT("Added cooldown tag to spec: %s"), *CooldownTag.ToString());
                        
                        // Apply the cooldown effect to self
                        FActiveGameplayEffectHandle ActiveGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
                            *SpecHandle.Data.Get());
                        
                        if (ActiveGEHandle.IsValid())
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Successfully applied cooldown effect with handle: %s"), 
                                *ActiveGEHandle.ToString());
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("Failed to apply cooldown effect for ability %d"), AbilityID);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to create valid cooldown effect spec"));
                    }
                }
            }
        }
    }
    
    // Call the parent class implementation
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogTemp, Warning, TEXT("===== END ABILITY COMPLETE ====="));
}

FGameplayTag UWoWGameplayAbilityBase::GetCooldownTag() const
{
    // Get ability data
    FAbilityTableRow AbilityData;
    if (GetAbilityData(AbilityData) && AbilityData.CooldownTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Using ability-specific cooldown tag: %s"), 
            *AbilityData.CooldownTag.ToString());
        return AbilityData.CooldownTag;
    }
    
    // Create a fallback tag if none was provided
    FName TagName = FName(*FString::Printf(TEXT("Ability.Cooldown.ID.%d"), AbilityID));
    
    // Use the standard way to request a gameplay tag
    FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TagName, false);
    
    if (!CooldownTag.IsValid())
    {
        // If the tag doesn't exist yet, try to create it
        CooldownTag = FGameplayTag::RequestGameplayTag(TagName, true);
        
        if (CooldownTag.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Created new cooldown tag: %s"), *CooldownTag.ToString());
        }
        else
        {
            // Fall back to a more generic tag
            CooldownTag = FGameplayTag::RequestGameplayTag(FName("Ability.Cooldown"), false);
            UE_LOG(LogTemp, Warning, TEXT("Using generic cooldown tag: %s"), *CooldownTag.ToString());
        }
    }
    
    return CooldownTag;
}

bool UWoWGameplayAbilityBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, 
                                              const FGameplayAbilityActorInfo* ActorInfo, 
                                              const FGameplayTagContainer* SourceTags, 
                                              const FGameplayTagContainer* TargetTags, 
                                              OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    UE_LOG(LogTemp, Warning, TEXT("===== CAN ACTIVATE ABILITY %d ====="), AbilityID);
    
    // Show on-screen debug info
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow,
            FString::Printf(TEXT("Checking ability %d activation"), AbilityID));
    }
    
    // Check if parent class allows activation
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        UE_LOG(LogTemp, Warning, TEXT("Super::CanActivateAbility returned false"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Super::CanActivateAbility returned true"));
    
    // Check if ability is on cooldown
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        // Get all the tags currently on the ASC
        FGameplayTagContainer AllTags;
        ActorInfo->AbilitySystemComponent->GetOwnedGameplayTags(AllTags);
        UE_LOG(LogTemp, Warning, TEXT("All tags on ASC (%d tags):"), AllTags.Num());
        for (const FGameplayTag& Tag : AllTags)
        {
            UE_LOG(LogTemp, Warning, TEXT(" - %s"), *Tag.ToString());
        }
        
        // Get the correct ability data
        FAbilityTableRow AbilityData;
        if (GetAbilityData(AbilityData) && AbilityData.CooldownTag.IsValid())
        {
            // Use the specific tag from data
            FGameplayTag CooldownTag = AbilityData.CooldownTag;
            UE_LOG(LogTemp, Warning, TEXT("Checking for ability-specific cooldown tag: %s"), 
                *CooldownTag.ToString());
            
            // Check if we have this tag
            bool bHasTag = ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CooldownTag);
            UE_LOG(LogTemp, Warning, TEXT("Has specific cooldown tag: %s"), 
                bHasTag ? TEXT("YES") : TEXT("NO"));
            
            if (bHasTag)
            {
                UE_LOG(LogTemp, Warning, TEXT("Ability %d is on cooldown"), AbilityID);
                
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
                        FString::Printf(TEXT("Ability %d on cooldown (tag: %s)"), 
                        AbilityID, *CooldownTag.ToString()));
                }
                
                return false;
            }
        }
        
        // As a backup, also check the generic cooldown tag
        FGameplayTag GenericCooldownTag = FGameplayTag::RequestGameplayTag(
            FName(*FString::Printf(TEXT("Ability.Cooldown.ID.%d"), AbilityID)));
            
        if (GenericCooldownTag.IsValid())
        {
            bool bHasGenericTag = ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(GenericCooldownTag);
            
            if (bHasGenericTag)
            {
                UE_LOG(LogTemp, Warning, TEXT("Ability %d is on cooldown (generic tag)"), AbilityID);
                
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
                        FString::Printf(TEXT("Ability %d on cooldown (generic tag)"), AbilityID));
                }
                
                return false;
            }
        }
        
        // As an ultimate fallback, check the base cooldown tag
        FGameplayTag BaseCooldownTag = FGameplayTag::RequestGameplayTag(FName("Ability.Cooldown"));
        bool bHasBaseTag = ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(BaseCooldownTag);
        
        if (bHasBaseTag)
        {
            UE_LOG(LogTemp, Warning, TEXT("Ability %d is on cooldown (base tag)"), AbilityID);
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
                    FString::Printf(TEXT("Ability %d on cooldown (base tag)"), AbilityID));
            }
            
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
    
    // Store the ability handles for later use
    CurrentSpecHandle = Handle;
    CurrentActorInfo = ActorInfo;
    CurrentActivationInfo = ActivationInfo;
    
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
            
            // Set up a timer to delay ability execution
            GetWorld()->GetTimerManager().SetTimer(
                CastTimerHandle,
                this,
                &UWoWGameplayAbilityBase::OnCastTimeComplete,
                AbilityData.CastTime,
                false
            );
            
            // Notify the casting component
            AActor* OwnerActor = ActorInfo->AvatarActor.Get();
            if (OwnerActor)
            {
                UCastingComponent* CastingComp = OwnerActor->FindComponentByClass<UCastingComponent>();
                if (CastingComp)
                {
                    bool bCanCastWhileMoving = AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Ability.CanCastWhileMoving")));
                    CastingComp->NotifyCastStarted(AbilityData.DisplayName, AbilityData.CastTime, bCanCastWhileMoving);
                }
            }
            
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
            
            // Notify the casting component about completion
            AActor* OwnerActor = CurrentActorInfo->AvatarActor.Get();
            if (OwnerActor)
            {
                UCastingComponent* CastingComp = OwnerActor->FindComponentByClass<UCastingComponent>();
                if (CastingComp)
                {
                    CastingComp->NotifyCastCompleted();
                }
            }
            
            ConsumeManaCost(AbilityData.ManaCost, CurrentActorInfo);
            ExecuteGameplayEffects(CurrentActorInfo);
            
            // Broadcast cast complete event
            OnAbilityCastComplete.Broadcast(AbilityData);
            
            // IMPORTANT CHANGE: Always end the ability regardless of authority
            // This ensures cooldowns are applied on both server and client
            bool bIsServer = CurrentActorInfo->AbilitySystemComponent->GetOwnerRole() == ROLE_Authority;
            UE_LOG(LogTemp, Warning, TEXT("Ending ability on %s"), bIsServer ? TEXT("server") : TEXT("client"));
            EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
                    
                    // Check if we're the client without authority
                    bool bIsClient = ActorInfo->AbilitySystemComponent->GetOwnerRole() != ROLE_Authority;
                    
                    if (bIsClient)
                    {
                        // Client-side: forward the request to the server
                        UE_LOG(LogTemp, Warning, TEXT("Client-side effect application, forwarding to server"));
                        
                        // Set up parameters for server RPC
                        int32 EffectID = AbilityID; // You might want to send specific effect ID instead
                        
                        // Cast to your character to access the Server RPC
                        AWoWPlayerCharacter* PlayerChar = Cast<AWoWPlayerCharacter>(AvatarActor);
                        if (PlayerChar)
                        {
                            // Call Server RPC to apply the effect
                            PlayerChar->Server_ApplyAbilityEffect(EffectID, TargetActor);
                            
                            UE_LOG(LogTemp, Warning, TEXT("Requested server to apply effect"));
                            
                            // Show visual effects locally for responsiveness
                            if (GEngine)
                            {
                                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
                                    FString::Printf(TEXT("Ability effect sent to server!")));
                            }
                        }
                    }
                    else
                    {
                        // Server-side: direct effect application
                        UE_LOG(LogTemp, Warning, TEXT("Server-side effect application"));
                        
                        // Apply effects to valid target
                        bool Success = ApplyEffectsToTarget(TargetActor, EEffectContainerType::Target);
                        UE_LOG(LogTemp, Warning, TEXT("Apply effects to target result: %s"), 
                               Success ? TEXT("Success") : TEXT("Failed"));
                    }
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
        
        // Notify the casting component about interruption
        AActor* OwnerActor = ActorInfo->AvatarActor.Get();
        if (OwnerActor)
        {
            UCastingComponent* CastingComp = OwnerActor->FindComponentByClass<UCastingComponent>();
            if (CastingComp)
            {
                CastingComp->NotifyCastInterrupted();
            }
        }
        
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