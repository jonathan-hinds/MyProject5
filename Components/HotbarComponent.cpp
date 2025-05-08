#include "HotbarComponent.h"
#include "Net/UnrealNetwork.h"
#include "../Data/AbilityDataAsset.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "../Character/WoWPlayerCharacter.h"
#include "../States/WoWPlayerState.h"
#include "GameplayAbilitySpec.h"

UHotbarComponent::UHotbarComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    
    // Initialize with 12 slots (standard WoW hotbar size)
    HotbarSlots.SetNum(12);
    GlobalCooldownDuration = 1.5f; // Standard WoW GCD
    GlobalCooldownStartTime = -GlobalCooldownDuration;
}

void UHotbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UHotbarComponent, AbilityDataAsset);
}

void UHotbarComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get the ability system from the owner
    AActor* Owner = GetOwner();
    if (Owner)
    {
        // The WoWPlayerCharacter implements IAbilitySystemInterface
        IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(Owner);
        if (ASInterface)
        {
            OwnerAbilitySystem = ASInterface->GetAbilitySystemComponent();
            
            if (GEngine && OwnerAbilitySystem)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
                    TEXT("Successfully found ability system component through interface"));
            }
        }
    }
}

void UHotbarComponent::CheckForPlayerState()
{
    AActor* Owner = GetOwner();
    if (Owner)
    {
        APawn* OwnerPawn = Cast<APawn>(Owner);
        if (OwnerPawn && OwnerPawn->GetPlayerState())
        {
            AWoWPlayerState* PS = Cast<AWoWPlayerState>(OwnerPawn->GetPlayerState());
            if (PS)
            {
                OwnerAbilitySystem = PS->GetAbilitySystemComponent();
                
                if (OwnerAbilitySystem)
                {
                    // Clear the timer since we found the player state
                    GetWorld()->GetTimerManager().ClearTimer(CheckPlayerStateTimerHandle);
                    
                    // Now that we have the ASC, initialize the hotbar if we have the data asset
                    if (AbilityDataAsset)
                    {
                        InitializeHotbar(AbilityDataAsset);
                    }
                }
            }
        }
    }
}

void UHotbarComponent::InitializeHotbar(UAbilityDataAsset* DataAsset)
{
    if (!DataAsset || !DataAsset->AbilityDataTable)
    {
        return;
    }
    
    AbilityDataAsset = DataAsset;
    
    // On clients, we just store the data asset
    // The server will handle ability granting
    if (GetOwnerRole() < ROLE_Authority)
    {
        return;
    }
    
    // Server continues with ability granting
    // Try to get the ASC if we don't have it
    if (!OwnerAbilitySystem)
    {
        AActor* Owner = GetOwner();
        if (Owner)
        {
            IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(Owner);
            if (ASInterface)
            {
                OwnerAbilitySystem = ASInterface->GetAbilitySystemComponent();
            }
        }
    }
    
    // Go through all abilities and set them in their default slots
    TArray<FAbilityTableRow*> AllAbilities;
    DataAsset->AbilityDataTable->GetAllRows(TEXT("InitializeHotbar"), AllAbilities);
    
    for (FAbilityTableRow* AbilityRow : AllAbilities)
    {
        if (AbilityRow && AbilityRow->DefaultHotbarSlot > 0 && AbilityRow->DefaultHotbarSlot <= HotbarSlots.Num())
        {
            SetAbilityInSlot(AbilityRow->DefaultHotbarSlot - 1, AbilityRow->AbilityID);
        }
    }
}

void UHotbarComponent::SetAbilityInSlot(int32 SlotIndex, int32 AbilityID)
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num() || !AbilityDataAsset || !AbilityDataAsset->AbilityDataTable)
    {
        return;
    }
    
    // Get ability data from data table
    static const FString ContextString(TEXT("SetAbilityInSlot"));
    FAbilityTableRow* AbilityData = nullptr;
    
    // Find ability data in table
    TArray<FAbilityTableRow*> AllRows;
    AbilityDataAsset->AbilityDataTable->GetAllRows(ContextString, AllRows);
    
    for (FAbilityTableRow* Row : AllRows)
    {
        if (Row && Row->AbilityID == AbilityID)
        {
            AbilityData = Row;
            break;
        }
    }
    
    if (!AbilityData)
    {
        return;
    }
    
    // Set up the slot
    HotbarSlots[SlotIndex].AbilityID = AbilityID;
    HotbarSlots[SlotIndex].AbilityData = AbilityData;
    
    // Grant the ability to the owner
    GrantAbilityToOwner(SlotIndex, AbilityData);
}

void UHotbarComponent::GrantAbilityToOwner(int32 SlotIndex, const FAbilityTableRow* AbilityData)
{
    // Only grant abilities on the server
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    
    if (!AbilityData || !AbilityData->AbilityClass)
    {
        return;
    }
    
    // Try to get the ASC if we don't have it
    if (!OwnerAbilitySystem)
    {
        AActor* Owner = GetOwner();
        if (Owner)
        {
            IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(Owner);
            if (ASInterface)
            {
                OwnerAbilitySystem = ASInterface->GetAbilitySystemComponent();
            }
        }
    }
    
    if (!OwnerAbilitySystem)
    {
        return;
    }
    
    // Create ability spec
    FGameplayAbilitySpec AbilitySpec(AbilityData->AbilityClass, 1, SlotIndex, GetOwner());
    
    // Give ability to owner and store handle
    FGameplayAbilitySpecHandle Handle = OwnerAbilitySystem->GiveAbility(AbilitySpec);
    
    if (Handle.IsValid())
    {
        HotbarSlots[SlotIndex].AbilityHandle = Handle;
    }
}

void UHotbarComponent::ActivateAbilityInSlot(int32 SlotIndex)
{
    // If we're not the server, call the server RPC
    if (GetOwnerRole() < ROLE_Authority)
    {
        ServerActivateAbilityInSlot(SlotIndex);
        return;
    }
    
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return;
    }
    
    // Always try to get the ASC fresh
    if (!OwnerAbilitySystem)
    {
        AActor* Owner = GetOwner();
        if (Owner)
        {
            IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(Owner);
            if (ASInterface)
            {
                OwnerAbilitySystem = ASInterface->GetAbilitySystemComponent();
            }
        }
    }
    
    if (!OwnerAbilitySystem)
    {
        return;
    }
    
    const FHotbarSlot& Slot = HotbarSlots[SlotIndex];
    if (!Slot.AbilityData)
    {
        return;
    }
    
    if (!Slot.AbilityHandle.IsValid())
    {
        return;
    }
    
    // Check global cooldown
    if (Slot.AbilityData->bUsesGlobalCooldown && IsOnGlobalCooldown())
    {
        return;
    }
    
    // Try to activate ability
    bool bSuccess = OwnerAbilitySystem->TryActivateAbility(Slot.AbilityHandle);
    
    // If successful and ability uses GCD, trigger it
    if (bSuccess && Slot.AbilityData->bUsesGlobalCooldown)
    {
        GlobalCooldownStartTime = GetWorld()->GetTimeSeconds();
    }
}

void UHotbarComponent::ServerActivateAbilityInSlot_Implementation(int32 SlotIndex)
{
    ActivateAbilityInSlot(SlotIndex);
}

bool UHotbarComponent::IsOnGlobalCooldown() const
{
    return (GetWorld()->GetTimeSeconds() - GlobalCooldownStartTime) < GlobalCooldownDuration;
}

bool UHotbarComponent::GetAbilityDataForSlot(int32 SlotIndex, FAbilityTableRow& OutAbilityData) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return false;
    }
    
    if (HotbarSlots[SlotIndex].AbilityData)
    {
        OutAbilityData = *HotbarSlots[SlotIndex].AbilityData;
        return true;
    }
    
    return false;
}

bool UHotbarComponent::IsAbilityOnCooldown(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num() || !OwnerAbilitySystem)
    {
        return false;
    }
    
    const FHotbarSlot& Slot = HotbarSlots[SlotIndex];
    if (!Slot.AbilityHandle.IsValid())
    {
        return false;
    }
    
    // Check if ability is on cooldown
    FGameplayAbilitySpec* AbilitySpec = OwnerAbilitySystem->FindAbilitySpecFromHandle(Slot.AbilityHandle);
    if (AbilitySpec)
    {
        float RemainingTime, CooldownDuration;
        AbilitySpec->Ability->GetCooldownTimeRemainingAndDuration(
            Slot.AbilityHandle, OwnerAbilitySystem->AbilityActorInfo.Get(), 
            RemainingTime, CooldownDuration);
        
        return RemainingTime > 0.0f;
    }
    
    return false;
}

float UHotbarComponent::GetRemainingCooldown(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num() || !OwnerAbilitySystem)
    {
        return 0.0f;
    }
    
    const FHotbarSlot& Slot = HotbarSlots[SlotIndex];
    if (!Slot.AbilityHandle.IsValid())
    {
        return 0.0f;
    }
    
    FGameplayAbilitySpec* AbilitySpec = OwnerAbilitySystem->FindAbilitySpecFromHandle(Slot.AbilityHandle);
    if (AbilitySpec)
    {
        float RemainingTime, CooldownDuration;
        AbilitySpec->Ability->GetCooldownTimeRemainingAndDuration(
            Slot.AbilityHandle, OwnerAbilitySystem->AbilityActorInfo.Get(), 
            RemainingTime, CooldownDuration);
        
        return RemainingTime;
    }
    
    return 0.0f;
}

float UHotbarComponent::GetAbilityCooldownProgress(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return 1.0f; // Return 1.0 (fully cooled down) if invalid slot
    }

    // Get ASC from PlayerState directly
    UAbilitySystemComponent* ASC = nullptr;
    AActor* OwningActor = GetOwner();
    AWoWPlayerCharacter* PlayerChar = Cast<AWoWPlayerCharacter>(OwningActor);
    if (PlayerChar)
    {
        AWoWPlayerState* PS = PlayerChar->GetPlayerState<AWoWPlayerState>();
        if (PS)
        {
            ASC = PS->GetAbilitySystemComponent();
        }
    }
    
    if (!ASC)
    {
        return 1.0f; // Return 1.0 (fully cooled down) if no ASC
    }

    // Get all active effects
    TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(FGameplayEffectQuery());
    
    // Check if any of these effects is our cooldown effect
    for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
    {
        const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
        if (Effect && Effect->Spec.Def)
        {
            // Check if this is a cooldown effect by name
            FString EffectName = Effect->Spec.Def->GetName();
            if (EffectName.Contains("GE_AbilityCooldown"))
            {
                // Found a cooldown effect - calculate progress
                
                // Get the current game world time
                float CurrentTime = GetWorld()->GetTimeSeconds();
                
                // Get the time remaining
                float TimeRemaining = Effect->GetTimeRemaining(CurrentTime);
                
                // Get the total duration
                float Duration = Effect->GetDuration();
                
                // Handle edge cases
                if (Duration <= 0.0f)
                {
                    return 1.0f; // Avoid division by zero
                }
                
                // Calculate progress (0 = just started, 1 = finished)
                float Progress = 1.0f - (TimeRemaining / Duration);
                
                // Clamp between 0 and 1 for safety
                return FMath::Clamp(Progress, 0.0f, 1.0f);
            }
        }
    }
    
    return 1.0f; // Return 1.0 (fully cooled down) if no cooldown effect found
}

float UHotbarComponent::GetAbilityCooldownRemainingTime(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return 0.0f; // Return 0 (no time remaining) if invalid slot
    }

    // Get ASC from PlayerState directly
    UAbilitySystemComponent* ASC = nullptr;
    AActor* OwningActor = GetOwner();
    AWoWPlayerCharacter* PlayerChar = Cast<AWoWPlayerCharacter>(OwningActor);
    if (PlayerChar)
    {
        AWoWPlayerState* PS = PlayerChar->GetPlayerState<AWoWPlayerState>();
        if (PS)
        {
            ASC = PS->GetAbilitySystemComponent();
        }
    }
    
    if (!ASC)
    {
        return 0.0f; // Return 0 (no time remaining) if no ASC
    }

    // Get all active effects
    TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(FGameplayEffectQuery());
    
    // Check if any of these effects is our cooldown effect
    for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
    {
        const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
        if (Effect && Effect->Spec.Def)
        {
            // Check if this is a cooldown effect by name
            FString EffectName = Effect->Spec.Def->GetName();
            if (EffectName.Contains("GE_AbilityCooldown"))
            {
                // Found a cooldown effect
                float CurrentTime = GetWorld()->GetTimeSeconds();
                return Effect->GetTimeRemaining(CurrentTime);
            }
        }
    }
    
    return 0.0f; // Return 0 (no time remaining) if no cooldown effect found
}

// Replace LogAllGameplayEffects with this complete implementation
void UHotbarComponent::LogAllGameplayEffects() const
{
    // Print who owns this component
    AActor* OwningActor = GetOwner();
    FString OwnerName = OwningActor ? OwningActor->GetName() : TEXT("None");
    
    // Check if we're on server
    bool bIsServer = OwningActor ? OwningActor->HasAuthority() : false;
    FString MachineType = bIsServer ? TEXT("SERVER") : TEXT("CLIENT");
    
    // Get ASC from PlayerState instead of using the cached OwnerAbilitySystem
    UAbilitySystemComponent* ASC = nullptr;
    AWoWPlayerCharacter* PlayerChar = Cast<AWoWPlayerCharacter>(OwningActor);
    if (PlayerChar)
    {
        AWoWPlayerState* PS = PlayerChar->GetPlayerState<AWoWPlayerState>();
        if (PS)
        {
            ASC = PS->GetAbilitySystemComponent();
        }
    }
    
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
            FString::Printf(TEXT("[%s] LogAllGameplayEffects for %s"), *MachineType, *OwnerName));
    
    // Check if ASC is available
    if (!ASC)
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                FString::Printf(TEXT("[%s] No ASC Available for %s"), *MachineType, *OwnerName));
        
        // Check PlayerState
        if (PlayerChar)
        {
            AWoWPlayerState* PS = PlayerChar->GetPlayerState<AWoWPlayerState>();
            if (!PS)
            {
                if (GEngine)
                    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                        FString::Printf(TEXT("[%s] No PlayerState Available"), *MachineType));
            }
            else
            {
                if (GEngine)
                    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
                        FString::Printf(TEXT("[%s] Found PlayerState: %s"), *MachineType, *PS->GetName()));
                        
                if (!PS->GetAbilitySystemComponent())
                {
                    if (GEngine)
                        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                            FString::Printf(TEXT("[%s] PlayerState has no ASC"), *MachineType));
                }
            }
        }
        
        return;
    }
    
    // Log that we have a valid ASC
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
            FString::Printf(TEXT("[%s] Found valid ASC for %s"), *MachineType, *OwnerName));
    
    // Get all active effects
    TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(FGameplayEffectQuery());
    
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, 
            FString::Printf(TEXT("[%s] Found %d active effects"), *MachineType, ActiveEffects.Num()));
    
    // Show details for each active effect
    for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
    {
        const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
        if (Effect && Effect->Spec.Def)
        {
            float TimeRemaining = Effect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
                    FString::Printf(TEXT("[%s] Effect: %s (%.1fs remaining)"), 
                        *MachineType, *Effect->Spec.Def->GetName(), TimeRemaining));
                
                // Show all tags on this effect
                FString TagList = TEXT("Tags: ");
                for (const FGameplayTag& Tag : Effect->Spec.DynamicGrantedTags)
                {
                    TagList += Tag.ToString() + TEXT(" ");
                }
                
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TagList);
            }
        }
    }
    
    // Also check for any gameplay tags on the ASC
    FGameplayTagContainer OwnedTags;
    ASC->GetOwnedGameplayTags(OwnedTags);
    
    if (GEngine)
    {
        FString TagList = TEXT("ASC Tags: ");
        for (const FGameplayTag& Tag : OwnedTags)
        {
            TagList += Tag.ToString() + TEXT(" ");
        }
        
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, TagList);
    }
}

// Replace IsAbilityOnCooldownByGameplayEffect with this implementation
bool UHotbarComponent::IsAbilityOnCooldownByGameplayEffect(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return false;
    }

    // Get ASC from PlayerState directly
    UAbilitySystemComponent* ASC = nullptr;
    AActor* OwningActor = GetOwner();
    AWoWPlayerCharacter* PlayerChar = Cast<AWoWPlayerCharacter>(OwningActor);
    if (PlayerChar)
    {
        AWoWPlayerState* PS = PlayerChar->GetPlayerState<AWoWPlayerState>();
        if (PS)
        {
            ASC = PS->GetAbilitySystemComponent();
        }
    }
    
    if (!ASC)
    {
        return false;
    }

    // Get all active effects
    TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(FGameplayEffectQuery());
    
    // Check if any of these effects is our cooldown effect
    for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
    {
        const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
        if (Effect && Effect->Spec.Def)
        {
            // Check if this is a cooldown effect by name
            FString EffectName = Effect->Spec.Def->GetName();
            if (EffectName.Contains("GE_AbilityCooldown"))
            {
                // Found a cooldown effect
                return true;
            }
        }
    }
    
    return false;
}

// Replace IsAbilityOnCooldownByTag with this implementation
bool UHotbarComponent::IsAbilityOnCooldownByTag(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return false;
    }

    // Get ASC from PlayerState instead of using the cached OwnerAbilitySystem
    UAbilitySystemComponent* ASC = nullptr;
    AActor* OwningActor = GetOwner();
    AWoWPlayerCharacter* PlayerChar = Cast<AWoWPlayerCharacter>(OwningActor);
    if (PlayerChar)
    {
        AWoWPlayerState* PS = PlayerChar->GetPlayerState<AWoWPlayerState>();
        if (PS)
        {
            ASC = PS->GetAbilitySystemComponent();
        }
    }
    
    if (!ASC)
    {
        return false;
    }

    // Get the ability data to find its cooldown tag
    FAbilityTableRow AbilityData;
    if (!GetAbilityDataForSlot(SlotIndex, AbilityData))
    {
        return false;
    }

    // Check for ability-specific cooldown tag
    if (AbilityData.CooldownTag.IsValid())
    {
        if (ASC->HasMatchingGameplayTag(AbilityData.CooldownTag))
        {
            return true;
        }
    }

    // Check for generic cooldown tag format
    FName TagName = FName(*FString::Printf(TEXT("Ability.Cooldown.ID.%d"), AbilityData.AbilityID));
    FGameplayTag GenericCooldownTag = FGameplayTag::RequestGameplayTag(TagName, false);
    if (GenericCooldownTag.IsValid() && ASC->HasMatchingGameplayTag(GenericCooldownTag))
    {
        return true;
    }

    // Fallback to base cooldown tag
    FGameplayTag BaseCooldownTag = FGameplayTag::RequestGameplayTag(FName("Ability.Cooldown"), false);
    return ASC->HasMatchingGameplayTag(BaseCooldownTag);
}