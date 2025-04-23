#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WoWEnemyAttackAbility.generated.h"

UCLASS()
class MYPROJECT5_API UWoWEnemyAttackAbility : public UGameplayAbility
{
    GENERATED_BODY()
    
public:
    UWoWEnemyAttackAbility();
    
    // Override ActivateAbility to implement the ability logic
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    
    // Called when the ability ends
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    // The range of the attack
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float AttackRange;
    
    // Damage coefficient (multiplier for base damage)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float DamageCoefficient;
    
    // Gameplay effect to apply damage
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    TSubclassOf<UGameplayEffect> DamageEffect;
    
    // Cooldown between attacks
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    float CooldownDuration;
    
    // Apply damage to the target
    void ApplyDamageToTarget(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, AActor* TargetActor, float DamageAmount);
};