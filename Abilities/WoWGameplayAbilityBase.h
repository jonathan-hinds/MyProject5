// File: WoWGameplayAbilityBase.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "../Data/AbilityEffectTypes.h"
#include "../Data/AbilityDataAsset.h"
#include "../Components/TargetingComponent.h"
#include "WoWGameplayAbilityBase.generated.h"

class UEffectApplicationComponent;

UCLASS()
class MYPROJECT5_API UWoWGameplayAbilityBase : public UGameplayAbility
{
    GENERATED_BODY()
    
public:
    UWoWGameplayAbilityBase();
    
    // Get the data for this ability
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool GetAbilityData(FAbilityTableRow& OutAbilityData) const;
    
    // Initialize from ability table row ID
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void InitializeFromAbilityData(int32 AbilityID);
    
    // Apply ability effects to target using the effect container in the ability data
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ApplyEffectsToTarget(AActor* TargetActor, EEffectContainerType ContainerType = EEffectContainerType::Target);
    
    // Apply area effects around a target using the effect container in the ability data
    UFUNCTION(BlueprintCallable, Category = "Ability")
    int32 ApplyAreaEffects(AActor* CenterActor, float Radius, EEffectContainerType ContainerType = EEffectContainerType::Area);
    
    // Get the effect container from the ability data
    UFUNCTION(BlueprintCallable, Category = "Ability")
    const FEffectContainerSpec& GetEffectContainer(EEffectContainerType ContainerType = EEffectContainerType::Target) const;
    
protected:
    // The ability ID from the data table
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    int32 AbilityID;
    
    // Cached ability data
    UPROPERTY(Transient)
    FAbilityTableRow CachedAbilityData;
    
    // Flag indicating if ability data has been loaded
    UPROPERTY(Transient)
    bool bAbilityDataLoaded;
    
    // Reference to the ability data asset
    UPROPERTY(Transient)
    UAbilityDataAsset* AbilityDataAsset;
    
    // Find or create the effect application component
    UEffectApplicationComponent* GetEffectComponent() const;
    
    // Override activate ability to load data if needed
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};