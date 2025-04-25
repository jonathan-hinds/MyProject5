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
    
protected:
    // Damage range for the auto attack (base weapon damage)
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    FVector2D DamageRange;
    
    // The gameplay effect to apply when hitting a target
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    TSubclassOf<class UGameplayEffect> DamageEffect;
    
    // Apply damage to target
    void ApplyDamageToTarget(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, AActor* TargetActor, float DamageAmount);
};