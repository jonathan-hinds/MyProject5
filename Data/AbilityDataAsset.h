#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AbilityDataAsset.generated.h"

class UGameplayAbility;
class UTexture2D;

UENUM(BlueprintType)
enum class EAbilityType : uint8
{
    Instant UMETA(DisplayName = "Instant"),
    Channeled UMETA(DisplayName = "Channeled"),
    Passive UMETA(DisplayName = "Passive"),
    Toggle UMETA(DisplayName = "Toggle")
};

USTRUCT(BlueprintType)
struct FAbilityTableRow : public FTableRowBase
{
    GENERATED_BODY()
    
    // Unique ID for each ability
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    int32 AbilityID;
    
    // Display name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    FString DisplayName;
    
    // Description shown in UI
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    FString Description;
    
    // Icon for the hotbar
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    UTexture2D* AbilityIcon;
    
    // The actual ability class to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    TSubclassOf<UGameplayAbility> AbilityClass;
    
    // Type of ability
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
    EAbilityType AbilityType;
    
    // Resource cost
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float ManaCost;
    
    // Cooldown in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
    float Cooldown;
    
    // Cast time (0 for instant)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cast")
    float CastTime;
    
    // Maximum range (0 for melee/self)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Range")
    float MaxRange;
    
    // Is this ability on GCD?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
    bool bUsesGlobalCooldown;
    
    // Tags for the ability
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTagContainer AbilityTags;
    
    // Default hotbar slot (1-12)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    int32 DefaultHotbarSlot;
    
    FAbilityTableRow()
        : AbilityID(0)
        , DisplayName("")
        , Description("")
        , AbilityIcon(nullptr)
        , AbilityClass(nullptr)
        , AbilityType(EAbilityType::Instant)
        , ManaCost(0.0f)
        , Cooldown(0.0f)
        , CastTime(0.0f)
        , MaxRange(0.0f)
        , bUsesGlobalCooldown(true)
        , DefaultHotbarSlot(0)
    {
    }
};

UCLASS()
class MYPROJECT5_API UAbilityDataAsset : public UDataAsset
{
    GENERATED_BODY()
    
public:
    // The data table containing ability definitions
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    UDataTable* AbilityDataTable;
    
    // Get ability data by ID - Returns by value
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool GetAbilityDataByID(int32 AbilityID, FAbilityTableRow& OutAbilityData) const;
    
    // Get ability data by slot - Returns by value
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool GetAbilityDataBySlot(int32 SlotIndex, FAbilityTableRow& OutAbilityData) const;
};