// In WoWCharacterBase.h
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
class UEffectApplicationComponent;
class UEffectDataAsset;
class UAbilityDataAsset;

    UENUM(BlueprintType)
    enum class EMovementStance : uint8
    {
        Idle        UMETA(DisplayName = "Idle"),
        Forward     UMETA(DisplayName = "Forward"),
        Backward    UMETA(DisplayName = "Backward"),
        StrafeLeft  UMETA(DisplayName = "Strafe Left"),
        StrafeRight UMETA(DisplayName = "Strafe Right"),
    };

UCLASS()
class MYPROJECT5_API AWoWCharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AWoWCharacterBase();

UFUNCTION(BlueprintCallable, Category = "Animation")
float GetMovementDirection() const;

UFUNCTION(BlueprintCallable, Category = "Animation")
float GetMovementSpeed() const;

UFUNCTION(BlueprintCallable, Category = "Animation")
bool IsMoving() const;

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Implement the IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // Returns the attribute set
    virtual UWoWAttributeSet* GetAttributeSet() const;

    // Initialize the attributes from a data table
    virtual void InitializeAttributes();

    // Grant abilities to this character
    virtual void GiveAbilities();

    // Apply the startup effects (like setting base attributes)
    virtual void ApplyStartupEffects();
    
    // Get the Effect Application Component
    UFUNCTION(BlueprintPure, Category = "Effects")
    UEffectApplicationComponent* GetEffectApplicationComponent() const { return EffectApplicationComponent; }
    
    // Get the Ability Data Asset
    UFUNCTION(BlueprintPure, Category = "Abilities")
    UAbilityDataAsset* GetAbilityDataAsset() const { return AbilityDataAsset; }
    
    // Get the Effect Data Asset
    UFUNCTION(BlueprintPure, Category = "Effects")
    UEffectDataAsset* GetEffectDataAsset() const { return EffectDataAsset; }

        // Gets the appropriate animation stance based on movement direction
    UFUNCTION(BlueprintCallable, Category = "Animation")
    EMovementStance GetMovementStance() const;
    
    // Enum for animation stances - add to header or make a separate header
    
    // Animation references
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimSequence* IdleAnimation;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimSequence* ForwardAnimation;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimSequence* BackwardAnimation;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimSequence* StrafeLeftAnimation;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimSequence* StrafeRightAnimation;
    
    // Get the current animation to play
    UFUNCTION(BlueprintCallable, Category = "Animation")
    UAnimSequence* GetCurrentAnimation() const;
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    
   EMovementStance DirectionToStance(float Direction) const;

    // Effect Application component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
    UEffectApplicationComponent* EffectApplicationComponent;

    // Default attributes that will be used to set our starting values
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TSubclassOf<UGameplayEffect> DefaultAttributeEffect;

    // Default abilities that will be granted to this character
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;
    
    // Ability data asset
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    UAbilityDataAsset* AbilityDataAsset;
    
    // Effect data asset
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    UEffectDataAsset* EffectDataAsset;

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