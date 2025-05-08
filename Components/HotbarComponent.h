#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "HotbarComponent.generated.h"

class UAbilityDataAsset;
class UAbilitySystemComponent;
struct FAbilityTableRow;

USTRUCT(BlueprintType)
struct FHotbarSlot
{
    GENERATED_BODY()
    
    // The ability ID associated with this slot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar")
    int32 AbilityID;
    
    // The gameplay ability spec handle (for runtime use)
    UPROPERTY()
    FGameplayAbilitySpecHandle AbilityHandle;
    
    // Cached ability data - Not exposed to Blueprint
    const FAbilityTableRow* AbilityData;
    
    FHotbarSlot()
        : AbilityID(-1)
        , AbilityData(nullptr)
    {
    }
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECT5_API UHotbarComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UHotbarComponent();

    // For replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

public:    

UFUNCTION(BlueprintCallable, Category = "Cooldown")
float GetAbilityCooldownRemainingTime(int32 SlotIndex) const;

UFUNCTION(BlueprintCallable, Category = "Cooldown")
float GetAbilityCooldownProgress(int32 SlotIndex) const;

UFUNCTION(BlueprintCallable, Category = "Debug")
void LogAllGameplayEffects() const;

// These functions should already exist in your header file, but just to be sure:
UFUNCTION(BlueprintCallable, Category = "Cooldown")
bool IsAbilityOnCooldownByTag(int32 SlotIndex) const;

UFUNCTION(BlueprintCallable, Category = "Cooldown")
bool IsAbilityOnCooldownByGameplayEffect(int32 SlotIndex) const;

    // Initialize the hotbar with ability data
    UFUNCTION(BlueprintCallable, Category = "Hotbar")
    void InitializeHotbar(UAbilityDataAsset* DataAsset);
    
    // Activate ability in specific slot
    UFUNCTION(BlueprintCallable, Category = "Hotbar")
    void ActivateAbilityInSlot(int32 SlotIndex);
    
    // Server RPC for ability activation
    UFUNCTION(Server, Reliable)
    void ServerActivateAbilityInSlot(int32 SlotIndex);
    
    // Set ability for a specific slot
    UFUNCTION(BlueprintCallable, Category = "Hotbar")
    void SetAbilityInSlot(int32 SlotIndex, int32 AbilityID);
    
    // Get ability data for a slot - Returns by value
    UFUNCTION(BlueprintPure, Category = "Hotbar")
    bool GetAbilityDataForSlot(int32 SlotIndex, FAbilityTableRow& OutAbilityData) const;
    
    // Check if ability is on cooldown
    UFUNCTION(BlueprintPure, Category = "Hotbar")
    bool IsAbilityOnCooldown(int32 SlotIndex) const;
    
    // Get remaining cooldown for ability
    UFUNCTION(BlueprintPure, Category = "Hotbar")
    float GetRemainingCooldown(int32 SlotIndex) const;

    // Setter for ability system component
    UFUNCTION(BlueprintCallable, Category = "Hotbar")
    void SetAbilitySystemComponent(UAbilitySystemComponent* NewASC) 
    { 
        OwnerAbilitySystem = NewASC;
    }

    UFUNCTION(BlueprintPure, Category = "Hotbar")
    float GetGlobalCooldownDuration() const { return GlobalCooldownDuration; }
    
    // Get global cooldown start time
    UFUNCTION(BlueprintPure, Category = "Hotbar")
    float GetGlobalCooldownStartTime() const { return GlobalCooldownStartTime; }

protected:
    // The hotbar slots (typically 12)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hotbar")
    TArray<FHotbarSlot> HotbarSlots;
    
    // Reference to ability data asset
    UPROPERTY(Replicated)
    UAbilityDataAsset* AbilityDataAsset;
    
    // Cached reference to owner's ability system component
    UPROPERTY()
    UAbilitySystemComponent* OwnerAbilitySystem;
    
    // Global cooldown duration
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hotbar")
    float GlobalCooldownDuration;
    
    // Time when global cooldown started
    UPROPERTY()
    float GlobalCooldownStartTime;
    
    // Timer handle for checking player state
    FTimerHandle CheckPlayerStateTimerHandle;
    
    // Function to check for player state
    void CheckForPlayerState();
    
    // Check if on global cooldown
    bool IsOnGlobalCooldown() const;
    
    // Grant ability to owner's ability system
    void GrantAbilityToOwner(int32 SlotIndex, const FAbilityTableRow* AbilityData);
};