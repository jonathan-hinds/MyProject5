// File: EffectApplicationComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Data/AbilityEffectTypes.h"
#include "GameplayEffectTypes.h"
#include "EffectApplicationComponent.generated.h"

class UEffectDataAsset;
class UAbilitySystemComponent;
class UNiagaraSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEffectApplied, int32, EffectID, AActor*, Target, bool, WasSuccessful);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYPROJECT5_API UEffectApplicationComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UEffectApplicationComponent();

protected:
    virtual void BeginPlay() override;

public:    
    // Main function to apply a single effect to a target
    UFUNCTION(BlueprintCallable, Category = "Effects")
    FActiveGameplayEffectHandle ApplyEffectToTarget(int32 EffectID, AActor* TargetActor, float Level = 1.0f);
    
    // Apply multiple effects to a target
    UFUNCTION(BlueprintCallable, Category = "Effects")
    bool ApplyEffectsToTarget(const TArray<int32>& EffectIDs, AActor* TargetActor, float Level = 1.0f);
    
    // Apply a container of effects to a target
    UFUNCTION(BlueprintCallable, Category = "Effects")
    bool ApplyEffectContainerToTarget(const FEffectContainerSpec& EffectContainer, AActor* TargetActor, float Level = 1.0f);
    
    // Apply area effects around a target
    UFUNCTION(BlueprintCallable, Category = "Effects")
    int32 ApplyAreaEffects(const TArray<int32>& EffectIDs, AActor* CenterActor, float Radius, float Level = 1.0f);
    
    // Calculate the final magnitude for an effect
    UFUNCTION(BlueprintCallable, Category = "Effects")
    float CalculateEffectMagnitude(const FEffectTableRow& EffectData, AActor* SourceActor, AActor* TargetActor, float Level = 1.0f);
    
    // Set the effect data asset
    UFUNCTION(BlueprintCallable, Category = "Effects")
    void SetEffectDataAsset(UEffectDataAsset* NewDataAsset) { CachedEffectDataAsset = NewDataAsset; }
    
    // Get the effect data asset
    UFUNCTION(BlueprintPure, Category = "Effects")
    UEffectDataAsset* GetEffectDataAsset() const { return CachedEffectDataAsset; }
    
    // Event when an effect is applied
    UPROPERTY(BlueprintAssignable, Category = "Effects")
    FOnEffectApplied OnEffectApplied;

protected:
    // Reference to the effect data asset
    UPROPERTY()
    UEffectDataAsset* CachedEffectDataAsset;
    
    // Helper to get the ASC from an actor
    UAbilitySystemComponent* GetAbilitySystemComponent(AActor* Actor) const;
    
    // Helper to play effect VFX and audio
    void PlayEffectFeedback(const FEffectTableRow& EffectData, AActor* TargetActor, bool IsPersistent);
    
    // Helper to get stats from a character
    float GetStatValue(AActor* Actor, const FGameplayTag& StatTag) const;
    
    // Helper to make the gameplay effect spec
    FGameplayEffectSpecHandle CreateEffectSpec(const FEffectTableRow& EffectData, AActor* SourceActor, AActor* TargetActor, float Level, float CalculatedMagnitude);
};