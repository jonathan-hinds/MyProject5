#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "WoWPlayerState.generated.h"

class UAbilitySystemComponent;
class UWoWAttributeSet;

UCLASS()
class MYPROJECT5_API AWoWPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()
    
public:
    AWoWPlayerState();

    // Add BeginPlay to initialize attributes on server
    virtual void BeginPlay() override;

    // Implement the IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    // Returns the attribute set
    UWoWAttributeSet* GetAttributeSet() const;

protected:
    // Ability System Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    UAbilitySystemComponent* AbilitySystemComponent;

    // Attribute Set
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    UWoWAttributeSet* AttributeSet;
};