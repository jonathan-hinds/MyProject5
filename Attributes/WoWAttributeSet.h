#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WoWAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class MYPROJECT5_API UWoWAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UWoWAttributeSet();

    // Primary Attributes (Base WoW stats)
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Primary")
    FGameplayAttributeData Strength;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Strength)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Primary")
    FGameplayAttributeData Agility;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Agility)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Primary")
    FGameplayAttributeData Intellect;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Intellect)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Primary")
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Stamina)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Primary")
    FGameplayAttributeData Spirit;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Spirit)

    // Secondary Attributes
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Secondary")
    FGameplayAttributeData Armor;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Armor)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Secondary")
    FGameplayAttributeData CriticalStrikeChance;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, CriticalStrikeChance)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Secondary")
    FGameplayAttributeData HasteRating;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, HasteRating)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Secondary")
    FGameplayAttributeData MasteryRating;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, MasteryRating)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Secondary")
    FGameplayAttributeData VersatilityRating;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, VersatilityRating)

    // Vital Attributes
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Vital")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Health)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Vital")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, MaxHealth)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Vital")
    FGameplayAttributeData HealthRegenRate;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, HealthRegenRate)
    
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Attributes|Vital")
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, Mana)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Vital")
    FGameplayAttributeData MaxMana;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, MaxMana)
    
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Attributes|Vital")
    FGameplayAttributeData ManaRegenRate;
    ATTRIBUTE_ACCESSORS(UWoWAttributeSet, ManaRegenRate)

    // Stat conversion config (could be read from data table in production)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Config")
    float HealthPerStamina;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Config")
    float ManaPerIntellect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Config")
    float ArmorPerAgility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Config") 
    float CritPerAgility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Config")
    float ManaRegenPerSpirit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Config")
    float BaseHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Config")
    float BaseMana;

    // OnRep functions for replicated attributes
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
    
    UFUNCTION()
    virtual void OnRep_Mana(const FGameplayAttributeData& OldMana);

    // Override PostGameplayEffectExecute to handle attribute changes
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    
    // PreAttributeChange is called before any modification happens
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

    // Initialize attributes to their default values
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // Recalculate derived attributes when base stats change
// Recalculate derived attributes when base stats change
void UpdateDerivedAttributes(UAbilitySystemComponent* AbilityComp);
protected:
    // Helper function to clamp attributes
    void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
};