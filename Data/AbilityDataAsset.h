// File: AbilityDataAsset.h (modified)
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AbilityEffectTypes.h" // Add this include
#include "AbilityDataAsset.generated.h"

class UGameplayAbility;
class UTexture2D;

UENUM(BlueprintType)
enum class EAbilityType : uint8
{
    Instant UMETA(DisplayName = "Instant"), // No target needed, instant cast
    Target UMETA(DisplayName = "Target"),   // Requires target, instant cast
    Cast UMETA(DisplayName = "Cast"),       // Requires target, has cast time
    Friendly UMETA(DisplayName = "Friendly"), // Can target friendly players
    Area UMETA(DisplayName = "Area"),       // AoE effect
    Passive UMETA(DisplayName = "Passive")  // Passive ability
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
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
    FGameplayTag CooldownTag;

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
    
    // NEW FIELDS FOR EFFECTS SYSTEM
    
    // Effects to apply on self when ability activates
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    FEffectContainerSpec SelfEffects;
    
    // Effects to apply on target
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    FEffectContainerSpec TargetEffects;
    
    // Effects to apply in an area around target
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    FEffectContainerSpec AreaEffects;
    
    // Radius for area effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", 
        meta = (EditCondition = "AreaEffects.EffectIDs.Num() > 0"))
    float AreaEffectRadius;
    
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
        , AreaEffectRadius(300.0f)
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
    
    // Get ability data by ID
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool GetAbilityDataByID(int32 AbilityID, FAbilityTableRow& OutAbilityData) const;
    
    // Get ability data by slot
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool GetAbilityDataBySlot(int32 SlotIndex, FAbilityTableRow& OutAbilityData) const;
};