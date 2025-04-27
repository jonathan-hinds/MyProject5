// WoWAutoAttackAbility.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WoWAutoAttackAbility.generated.h"

UCLASS()
class MYPROJECT5_API UWoWAutoAttackAbility : public UGameplayAbility
{
    GENERATED_BODY()
    
public:
    UWoWAutoAttackAbility();
    
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    
    // Static function to check if auto-attack is active
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    static bool IsAutoAttackActive(UAbilitySystemComponent* ASC);
    
protected:
    // The gameplay effect to apply when hitting a target
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    TSubclassOf<class UGameplayEffect> DamageEffect;
    
    // Auto attack range
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float AttackRange;
    
    // Attack speed (aligned with GCD)
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float AttackSpeed;
    
    // Timer handle for continuous auto-attack
    FTimerHandle AutoAttackTimerHandle;
    
    // Apply damage to target
    void ApplyDamageToTarget(AActor* TargetActor, float DamageAmount);
    
    // Execute an auto-attack
    void PerformAutoAttack();
    
    // Check if target is an enemy that can be attacked
    bool CanAttackTarget(AActor* Target) const;
    
    // Get the current target from the targeting component
    AActor* GetCurrentTarget() const;
};