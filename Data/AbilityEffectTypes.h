// File: AbilityEffectTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "NiagaraSystem.h" // Add this for Niagara
#include "Sound/SoundBase.h" // Add this for sound
#include "AbilityEffectTypes.generated.h"

// Enumeration for basic effect types
UENUM(BlueprintType)
enum class EEffectType : uint8
{
    Damage          UMETA(DisplayName = "Instant Damage"),
    DamageOverTime  UMETA(DisplayName = "Damage Over Time"),
    Healing         UMETA(DisplayName = "Instant Healing"),
    HealingOverTime UMETA(DisplayName = "Healing Over Time"),
    Buff            UMETA(DisplayName = "Buff"),
    Debuff          UMETA(DisplayName = "Debuff"),
    Slow            UMETA(DisplayName = "Slow"),
    Stun            UMETA(DisplayName = "Stun"),
    Snare           UMETA(DisplayName = "Snare"),
    Custom          UMETA(DisplayName = "Custom")
};

// Duration type (instant, duration, or permanent)
UENUM(BlueprintType)
enum class EEffectDurationType : uint8
{
    Instant         UMETA(DisplayName = "Instant"),
    Duration        UMETA(DisplayName = "Duration"),
    Permanent       UMETA(DisplayName = "Permanent Until Removed")
};

// How the effect's magnitude is calculated
UENUM(BlueprintType)
enum class EEffectMagnitudeType : uint8
{
    Flat            UMETA(DisplayName = "Flat Value"),
    ScaledByStat    UMETA(DisplayName = "Scaled By Stat"),
    ScaledByLevel   UMETA(DisplayName = "Scaled By Level"),
    Custom          UMETA(DisplayName = "Custom Formula")
};

// Data structure for effect scaling
USTRUCT(BlueprintType)
struct FEffectScalingInfo
{
    GENERATED_BODY()
    
    // Base flat value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Scaling")
    float BaseValue = 0.0f;
    
    // Type of scaling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Scaling")
    EEffectMagnitudeType MagnitudeType = EEffectMagnitudeType::Flat;
    
    // Scaling coefficient
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Scaling", 
        meta = (EditCondition = "MagnitudeType != EEffectMagnitudeType::Flat"))
    float ScalingCoefficient = 1.0f;
    
    // Which stat to scale from, as a gameplay tag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Scaling", 
        meta = (EditCondition = "MagnitudeType == EEffectMagnitudeType::ScaledByStat"))
    FGameplayTag StatTag;
    
    // For custom formulas - name of the formula to use
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Scaling", 
        meta = (EditCondition = "MagnitudeType == EEffectMagnitudeType::Custom"))
    FName CustomFormula;
};

// Table row defining a WoW-like effect
USTRUCT(BlueprintType)
struct FEffectTableRow : public FTableRowBase
{
    GENERATED_BODY()
    
    // Unique ID for the effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 EffectID = 0;
    
    // Name of the effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString EffectName;
    
    // Description of the effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString Description;
    
    // Effect type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    EEffectType EffectType = EEffectType::Damage;
    
    // Duration type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    EEffectDurationType DurationType = EEffectDurationType::Instant;
    
    // Duration in seconds (for timed effects)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect",
        meta = (EditCondition = "DurationType == EEffectDurationType::Duration"))
    float Duration = 0.0f;
    
    // Period between ticks for DoT/HoT (0 = no ticks)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect", 
        meta = (EditCondition = "EffectType == EEffectType::DamageOverTime || EffectType == EEffectType::HealingOverTime"))
    float TickPeriod = 0.0f;
    
    // Magnitude information
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FEffectScalingInfo Magnitude;
    
    // Secondary magnitudes (for complex effects)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TMap<FName, FEffectScalingInfo> AdditionalMagnitudes;
    
    // Affected attribute (what attribute this modifies)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FGameplayTag AffectedAttributeTag;
    
    // Gameplay effect class to use
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TSoftClassPtr<class UGameplayEffect> GameplayEffectClass;
    
    // Required application tags (target must have these)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FGameplayTagContainer RequiredTargetTags;
    
    // Forbidden application tags (target must NOT have these)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FGameplayTagContainer ForbiddenTargetTags;
    
    // Tags granted to target when effect is applied
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FGameplayTagContainer GrantedTags;
    
    // Effect application VFX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Visuals")
    TSoftObjectPtr<class UNiagaraSystem> ApplicationVFX;
    
    // Effect persistent VFX (for duration effects)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Visuals")
    TSoftObjectPtr<class UNiagaraSystem> PersistentVFX;
    
    // Sound to play on application
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Audio")
    TSoftObjectPtr<class USoundBase> ApplicationSound;
    
    // Persistent sound for duration effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect|Audio")
    TSoftObjectPtr<class USoundBase> PersistentSound;
};

// Container for a collection of effects to apply
USTRUCT(BlueprintType)
struct FEffectContainerSpec
{
    GENERATED_BODY()
    
    // Array of effect IDs to apply
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<int32> EffectIDs;
};

// Enum for selecting which effect container to use
UENUM(BlueprintType)
enum class EEffectContainerType : uint8
{
    Self UMETA(DisplayName = "Self Effects"),
    Target UMETA(DisplayName = "Target Effects"),
    Area UMETA(DisplayName = "Area Effects")
};