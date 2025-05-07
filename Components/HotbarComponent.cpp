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
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    
    HotbarSlots.SetNum(12);
    GlobalCooldownDuration = 1.5f;
    GlobalCooldownStartTime = -GlobalCooldownDuration;
}

void UHotbarComponent::Client_StartCooldownAfterCast_Implementation(int32 SlotIndex)
{
    StartCooldownAfterCast(SlotIndex);
}

void UHotbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UHotbarComponent, AbilityDataAsset);
    DOREPLIFETIME(UHotbarComponent, CooldownSlotIndices);
    DOREPLIFETIME(UHotbarComponent, CooldownEndTimes);
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
    
    if (GetOwnerRole() < ROLE_Authority)
    {
        return;
    }
    
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
    
    TArray<FAbilityTableRow*> AllAbilities;
    DataAsset->AbilityDataTable->GetAllRows(TEXT("InitializeHotbar"), AllAbilities);
    
    for (FAbilityTableRow* AbilityRow : AllAbilities)
    {
        if (AbilityRow && AbilityRow->DefaultHotbarSlot > 0 && AbilityRow->DefaultHotbarSlot <= HotbarSlots.Num())
        {
            SetAbilityInSlot(AbilityRow->DefaultHotbarSlot - 1, AbilityRow->AbilityID);
        }
    }
    
    NotifyHotbarUIUpdate();
}

void UHotbarComponent::SetAbilityInSlot(int32 SlotIndex, int32 AbilityID)
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num() || !AbilityDataAsset || !AbilityDataAsset->AbilityDataTable)
    {
        return;
    }
    
    static const FString ContextString(TEXT("SetAbilityInSlot"));
    FAbilityTableRow* AbilityData = nullptr;
    
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
    
    HotbarSlots[SlotIndex].AbilityID = AbilityID;
    HotbarSlots[SlotIndex].AbilityData = AbilityData;
    
    GrantAbilityToOwner(SlotIndex, AbilityData);
    
    NotifyHotbarUIUpdate();
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
    if (GetOwnerRole() < ROLE_Authority)
    {
        if (SlotIndex >= 0 && SlotIndex < HotbarSlots.Num())
        {
            FAbilityTableRow AbilityData;
            if (GetAbilityDataForSlot(SlotIndex, AbilityData))
            {
                if (AbilityData.Cooldown > 0.0f && AbilityData.CastTime <= 0.0f)
                {
                    float CurrentTime = GetWorld()->GetTimeSeconds();
                    float EndTime = CurrentTime + AbilityData.Cooldown;
                    
                    int32 ExistingIndex = CooldownSlotIndices.Find(SlotIndex);
                    if (ExistingIndex != INDEX_NONE)
                    {
                        CooldownEndTimes[ExistingIndex] = EndTime;
                    }
                    else
                    {
                        CooldownSlotIndices.Add(SlotIndex);
                        CooldownEndTimes.Add(EndTime);
                    }
                }
            }
        }
        
        ServerActivateAbilityInSlot(SlotIndex);
        return;
    }
    
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return;
    }
    
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
    
    if (Slot.AbilityData->bUsesGlobalCooldown && IsOnGlobalCooldown())
    {
        return;
    }
    
    bool bSuccess = OwnerAbilitySystem->TryActivateAbility(Slot.AbilityHandle);
    
    if (bSuccess && Slot.AbilityData->bUsesGlobalCooldown)
    {
        GlobalCooldownStartTime = GetWorld()->GetTimeSeconds();
    }
    
    if (bSuccess && Slot.AbilityData && Slot.AbilityData->Cooldown > 0.0f && Slot.AbilityData->CastTime <= 0.0f)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float EndTime = CurrentTime + Slot.AbilityData->Cooldown;
        
        int32 ExistingIndex = CooldownSlotIndices.Find(SlotIndex);
        if (ExistingIndex != INDEX_NONE)
        {
            CooldownEndTimes[ExistingIndex] = EndTime;
        }
        else
        {
            CooldownSlotIndices.Add(SlotIndex);
            CooldownEndTimes.Add(EndTime);
        }
        
        Client_NotifyCooldownStarted(SlotIndex, Slot.AbilityData->Cooldown);
    }
}

void UHotbarComponent::StartCooldownAfterCast(int32 SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return;
    }
    
    FAbilityTableRow AbilityData;
    if (GetAbilityDataForSlot(SlotIndex, AbilityData) && AbilityData.Cooldown > 0.0f)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float EndTime = CurrentTime + AbilityData.Cooldown;
        
        int32 ExistingIndex = CooldownSlotIndices.Find(SlotIndex);
        if (ExistingIndex != INDEX_NONE)
        {
            CooldownEndTimes[ExistingIndex] = EndTime;
        }
        else
        {
            CooldownSlotIndices.Add(SlotIndex);
            CooldownEndTimes.Add(EndTime);
        }
        
        if (GetOwnerRole() == ROLE_Authority)
        {
            UE_LOG(LogTemp, Warning, TEXT("SERVER: Sending cooldown notification for slot %d"), SlotIndex);
            Client_NotifyCooldownStarted(SlotIndex, AbilityData.Cooldown);
        }
    }
}

void UHotbarComponent::Client_NotifyCooldownStarted_Implementation(int32 SlotIndex, float Duration)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float EndTime = CurrentTime + Duration;
    
    int32 ExistingIndex = CooldownSlotIndices.Find(SlotIndex);
    if (ExistingIndex != INDEX_NONE)
    {
        CooldownEndTimes[ExistingIndex] = EndTime;
    }
    else
    {
        CooldownSlotIndices.Add(SlotIndex);
        CooldownEndTimes.Add(EndTime);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("CLIENT: Received cooldown notification for slot %d, duration %.1f"), 
           SlotIndex, Duration);
}

float UHotbarComponent::GetCooldownPercent(int32 SlotIndex) const
{
    // Check client-side tracking arrays
    for (int32 i = 0; i < CooldownSlotIndices.Num(); i++)
    {
        if (CooldownSlotIndices[i] == SlotIndex)
        {
            float CurrentTime = GetWorld()->GetTimeSeconds();
            float EndTime = CooldownEndTimes[i];
            
            float CooldownDuration = 0.0f;
            if (SlotIndex >= 0 && SlotIndex < HotbarSlots.Num())
            {
                FAbilityTableRow AbilityData;
                if (GetAbilityDataForSlot(SlotIndex, AbilityData))
                {
                    CooldownDuration = AbilityData.Cooldown;
                }
            }
            
            if (CooldownDuration > 0.0f)
            {
                float TimeRemaining = FMath::Max(0.0f, EndTime - CurrentTime);
                return TimeRemaining / CooldownDuration;
            }
        }
    }
    
    return 0.0f;
}

float UHotbarComponent::GetCooldownStartTime(int32 SlotIndex) const
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
    
    // Get ability spec to check cooldown
    FGameplayAbilitySpec* AbilitySpec = OwnerAbilitySystem->FindAbilitySpecFromHandle(Slot.AbilityHandle);
    if (AbilitySpec)
    {
        float TimeRemaining, CooldownDuration;
        AbilitySpec->Ability->GetCooldownTimeRemainingAndDuration(
            Slot.AbilityHandle, OwnerAbilitySystem->AbilityActorInfo.Get(), 
            TimeRemaining, CooldownDuration);
        
        if (TimeRemaining > 0.0f && CooldownDuration > 0.0f)
        {
            // Calculate when the cooldown started
            return GetWorld()->GetTimeSeconds() - (CooldownDuration - TimeRemaining);
        }
    }
    
    return 0.0f;
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
    for (int32 i = 0; i < CooldownSlotIndices.Num(); i++)
    {
        if (CooldownSlotIndices[i] == SlotIndex)
        {
            float CurrentTime = GetWorld()->GetTimeSeconds();
            float EndTime = CooldownEndTimes[i];
            
            if (CurrentTime < EndTime)
            {
                return true;
            }
        }
    }
    
    if (GetOwnerRole() == ROLE_Authority && OwnerAbilitySystem && SlotIndex >= 0 && SlotIndex < HotbarSlots.Num())
    {
        const FHotbarSlot& Slot = HotbarSlots[SlotIndex];
        if (!Slot.AbilityHandle.IsValid())
        {
            return false;
        }
        
        if (Slot.AbilityData && Slot.AbilityData->CooldownTag.IsValid())
        {
            bool bHasTag = OwnerAbilitySystem->HasMatchingGameplayTag(Slot.AbilityData->CooldownTag);
            if (bHasTag)
            {
                return true;
            }
        }
        
        if (Slot.AbilityID > 0)
        {
            FName TagName = FName(*FString::Printf(TEXT("Ability.Cooldown.ID.%d"), Slot.AbilityID));
            FGameplayTag GenericCooldownTag = FGameplayTag::RequestGameplayTag(TagName);
            if (GenericCooldownTag.IsValid() && OwnerAbilitySystem->HasMatchingGameplayTag(GenericCooldownTag))
            {
                return true;
            }
        }
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

UTexture2D* UHotbarComponent::GetAbilityIconForSlot(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return nullptr;
    }
    
    FAbilityTableRow AbilityData;
    if (GetAbilityDataForSlot(SlotIndex, AbilityData))
    {
        return AbilityData.AbilityIcon;
    }
    
    return nullptr;
}

FString UHotbarComponent::GetKeybindTextForSlot(int32 SlotIndex) const
{
    switch (SlotIndex)
    {
        case 0: return TEXT("1");
        case 1: return TEXT("2");
        case 2: return TEXT("3");
        case 3: return TEXT("4");
        case 4: return TEXT("5");
        case 5: return TEXT("6");
        case 6: return TEXT("7");
        case 7: return TEXT("8");
        case 8: return TEXT("9");
        case 9: return TEXT("0");
        case 10: return TEXT("-");
        case 11: return TEXT("=");
        default: return TEXT("");
    }
}

bool UHotbarComponent::HasAbilityInSlot(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= HotbarSlots.Num())
    {
        return false;
    }
    
    return HotbarSlots[SlotIndex].AbilityID > 0 && HotbarSlots[SlotIndex].AbilityHandle.IsValid();
}

float UHotbarComponent::GetCooldownProgressForSlot(int32 SlotIndex) const
{
    if (!HasAbilityInSlot(SlotIndex) || !OwnerAbilitySystem)
    {
        return 0.0f;
    }
    
    if (IsOnGlobalCooldown())
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float TimeElapsed = CurrentTime - GlobalCooldownStartTime;
        float NormalizedProgress = FMath::Clamp(TimeElapsed / GlobalCooldownDuration, 0.0f, 1.0f);
        
        return 1.0f - NormalizedProgress;
    }
    
    float RemainingCooldown = GetRemainingCooldown(SlotIndex);
    if (RemainingCooldown <= 0.0f)
    {
        return 0.0f;
    }
    
    FAbilityTableRow AbilityData;
    float TotalCooldown = 0.0f;
    if (GetAbilityDataForSlot(SlotIndex, AbilityData))
    {
        TotalCooldown = AbilityData.Cooldown;
    }
    
    if (TotalCooldown <= 0.0f)
    {
        return 0.0f;
    }
    
    return FMath::Clamp(RemainingCooldown / TotalCooldown, 0.0f, 1.0f);
}

FString UHotbarComponent::GetCooldownTextForSlot(int32 SlotIndex) const
{
    float RemainingCooldown = GetRemainingCooldown(SlotIndex);
    
    if (IsOnGlobalCooldown())
    {
        float GCDRemaining = GlobalCooldownDuration - (GetWorld()->GetTimeSeconds() - GlobalCooldownStartTime);
        if (GCDRemaining > RemainingCooldown)
        {
            RemainingCooldown = GCDRemaining;
        }
    }
    
    if (RemainingCooldown <= 0.0f)
    {
        return TEXT("");
    }
    
    if (RemainingCooldown >= 1.0f)
    {
        return FString::Printf(TEXT("%.1f"), RemainingCooldown);
    }
    else
    {
        return FString::Printf(TEXT("%.0f"), RemainingCooldown * 10.0f);
    }
}

void UHotbarComponent::NotifyHotbarUIUpdate()
{
    OnHotbarUpdated.Broadcast();
}

void UHotbarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    for (int32 i = CooldownSlotIndices.Num() - 1; i >= 0; i--)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float EndTime = CooldownEndTimes[i];
        
        if (CurrentTime >= EndTime)
        {
            int32 SlotIndex = CooldownSlotIndices[i];
            CooldownSlotIndices.RemoveAt(i);
            CooldownEndTimes.RemoveAt(i);
            
            UE_LOG(LogTemp, Warning, TEXT("%s: Cooldown for slot %d has expired"), 
                   GetOwnerRole() == ROLE_Authority ? TEXT("SERVER") : TEXT("CLIENT"),
                   SlotIndex);
        }
    }
}