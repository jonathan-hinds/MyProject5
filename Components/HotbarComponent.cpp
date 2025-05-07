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