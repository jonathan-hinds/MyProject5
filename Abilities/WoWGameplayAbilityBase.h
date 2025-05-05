// File: WoWGameplayAbilityBase.h

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "../Data/AbilityEffectTypes.h"
#include "../Data/AbilityDataAsset.h"
#include "WoWGameplayAbilityBase.generated.h"

// Forward declarations
class UEffectApplicationComponent;
class UWoWAttributeSet;
class UTargetingComponent;

// Delegate for casting events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityCastEvent, const FAbilityTableRow&, AbilityData);

UCLASS()
class MYPROJECT5_API UWoWGameplayAbilityBase : public UGameplayAbility
{
    GENERATED_BODY()
    
public:
    UWoWGameplayAbilityBase();
    
    // Override to implement ability logic
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
                                const FGameplayAbilityActorInfo* ActorInfo, 
                                const FGameplayAbilityActivationInfo ActivationInfo, 
                                const FGameplayEventData* TriggerEventData) override;
    
    // Override to handle cooldowns
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, 
                          const FGameplayAbilityActorInfo* ActorInfo, 
                          const FGameplayAbilityActivationInfo ActivationInfo, 
                          bool bReplicateEndAbility, bool bWasCancelled) override;
                          
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, 
                                const FGameplayAbilityActorInfo* ActorInfo, 
                                const FGameplayTagContainer* SourceTags = nullptr, 
                                const FGameplayTagContainer* TargetTags = nullptr, 
                                OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    void ExecuteAbility(const FGameplayAbilitySpecHandle Handle, 
                    const FGameplayAbilityActorInfo* ActorInfo, 
                    const FGameplayAbilityActivationInfo ActivationInfo);

    // Get the data for this ability
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool GetAbilityData(FAbilityTableRow& OutAbilityData) const;
    
    // Initialize from ability table row ID
    UFUNCTION(BlueprintCallable, Category = "Ability")
    void InitializeFromAbilityData(int32 AbilityID);
    
    // Apply ability effects to target
    UFUNCTION(BlueprintCallable, Category = "Ability")
    bool ApplyEffectsToTarget(AActor* TargetActor, EEffectContainerType ContainerType = EEffectContainerType::Target);
    
    // Apply area effects around a target
    UFUNCTION(BlueprintCallable, Category = "Ability")
    int32 ApplyAreaEffects(AActor* CenterActor, float Radius, EEffectContainerType ContainerType = EEffectContainerType::Area);
    
    // Get the effect container from the ability data
    UFUNCTION(BlueprintCallable, Category = "Ability")
    const FEffectContainerSpec& GetEffectContainer(EEffectContainerType ContainerType = EEffectContainerType::Target) const;
    
    // Get the cast timer handle for UI to check remaining time
    UFUNCTION(BlueprintCallable, Category = "Casting")
    FTimerHandle GetCastTimerHandle() const { return CastTimerHandle; }
    
    // Override to handle cast interruption
    virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, 
                              const FGameplayAbilityActorInfo* ActorInfo, 
                              const FGameplayAbilityActivationInfo ActivationInfo,
                              bool bReplicateCancelAbility) override;
    
    // Get cooldown tag for this ability
    UFUNCTION(BlueprintCallable, Category = "Cooldown")
    FGameplayTag GetCooldownTag() const;
    
protected:
    // The ability ID from the data table
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    int32 AbilityID;
    
    // Cooldown GameplayEffect to use
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
    TSubclassOf<UGameplayEffect> CooldownGameplayEffect;
    
    // Cached ability data
    UPROPERTY(Transient)
    FAbilityTableRow CachedAbilityData;
    
    // Flag indicating if ability data has been loaded
    UPROPERTY(Transient)
    bool bAbilityDataLoaded;
    
    // Reference to the ability data asset
    UPROPERTY(Transient)
    UAbilityDataAsset* AbilityDataAsset;
    
    // For cast time handling
    FTimerHandle CastTimerHandle;
    FGameplayAbilitySpecHandle CurrentSpecHandle;
    const FGameplayAbilityActorInfo* CurrentActorInfo;
    FGameplayAbilityActivationInfo CurrentActivationInfo;
    
    // Helper methods
    void OnCastTimeComplete();
    void ConsumeManaCost(float ManaCost, const FGameplayAbilityActorInfo* ActorInfo);
    void ExecuteGameplayEffects(const FGameplayAbilityActorInfo* ActorInfo);
    
    // Find or create the effect application component
    UEffectApplicationComponent* GetEffectComponent() const;
    
    // Event delegates for UI notifications
    UPROPERTY(BlueprintAssignable, Category = "Ability")
    FOnAbilityCastEvent OnAbilityCastStarted;
    
    UPROPERTY(BlueprintAssignable, Category = "Ability")
    FOnAbilityCastEvent OnAbilityCastComplete;
    
    UPROPERTY(BlueprintAssignable, Category = "Ability")
    FOnAbilityCastEvent OnAbilityCastInterrupted;
};