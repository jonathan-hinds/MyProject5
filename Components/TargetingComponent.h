// TargetingComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

class AWoWCharacterBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECT5_API UTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UTargetingComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    // Set current target
    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void SetTarget(AActor* NewTarget);
    
    // Clear current target
    UFUNCTION(BlueprintCallable, Category = "Targeting")
    void ClearTarget();
    
    // Get current target
    UFUNCTION(BlueprintPure, Category = "Targeting")
    AActor* GetCurrentTarget() const { return CurrentTarget; }
    
    // Is target valid
    UFUNCTION(BlueprintPure, Category = "Targeting")
    bool HasValidTarget() const;
    
    // Get target health percent
    UFUNCTION(BlueprintPure, Category = "Targeting")
    float GetTargetHealthPercent() const;
    
    // Get target mana percent
    UFUNCTION(BlueprintPure, Category = "Targeting")
    float GetTargetManaPercent() const;
    
    // Get target health and max health
    UFUNCTION(BlueprintPure, Category = "Targeting")
    void GetTargetHealth(float& Health, float& MaxHealth) const;
    
    // Get target mana and max mana
    UFUNCTION(BlueprintPure, Category = "Targeting")
    void GetTargetMana(float& Mana, float& MaxMana) const;
    
    // Get target name
    UFUNCTION(BlueprintPure, Category = "Targeting")
    FString GetTargetName() const;
    
    // Start auto-attack
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StartAutoAttack();
    
    // Stop auto-attack
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void StopAutoAttack();
    
    // Is auto-attacking
    UFUNCTION(BlueprintPure, Category = "Combat")
    bool IsAutoAttacking() const { return bIsAutoAttacking; }

    // Server RPCs for network replication
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetTarget(AActor* NewTarget);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ClearTarget();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_StartAutoAttack();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_StopAutoAttack();

protected:
    virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float MeleeAttackRange;

    // Current target
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Targeting")
    AActor* CurrentTarget;
    
    // Is auto-attacking flag
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
    bool bIsAutoAttacking;
    
    // Handle selecting target by mouse click
    void HandleTargetSelection();
    
    // Handle auto-attack
    void HandleAutoAttack();
    
    // For replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Timer handle for auto-attack
    FTimerHandle AutoAttackTimerHandle;
    
    // Auto-attack delay
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float AutoAttackDelay;
    
    // Auto-attack ability tag
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FName AutoAttackAbilityTag;
};