#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "WoWCharacterBase.generated.h"

class UAbilitySystemComponent;
class UWoWAttributeSet;
class UGameplayEffect;
class UGameplayAbility;

UCLASS()
class MYPROJECT5_API AWoWCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AWoWCharacterBase();

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Implement the IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // Returns the attribute set
    UWoWAttributeSet* GetAttributeSet() const;

    // Initialize the attributes from a data table
    virtual void InitializeAttributes();

    // Grant abilities to this character
    virtual void GiveAbilities();

    // Apply the startup effects (like setting base attributes)
    virtual void ApplyStartupEffects();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Default attributes that will be used to set our starting values
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

    // Default abilities that will be granted to this character
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // Flag to determine if the abilities have been initialized
    bool bAbilitiesInitialized;

public:
    // Helper function to get health percentage
    UFUNCTION(BlueprintPure, Category = "Attributes")
    float GetHealthPercent() const;
    
    // Helper function to get mana percentage
    UFUNCTION(BlueprintPure, Category = "Attributes")
    float GetManaPercent() const;
    
    // Helper functions to get current values
    UFUNCTION(BlueprintPure, Category = "Attributes")
    float GetHealth() const;
    
    UFUNCTION(BlueprintPure, Category = "Attributes")
    float GetMaxHealth() const;
    
    UFUNCTION(BlueprintPure, Category = "Attributes")
    float GetMana() const;
    
    UFUNCTION(BlueprintPure, Category = "Attributes")
    float GetMaxMana() const;
};