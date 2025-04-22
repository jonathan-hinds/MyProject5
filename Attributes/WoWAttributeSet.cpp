#include "WoWAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UWoWAttributeSet::UWoWAttributeSet()
{
    // Initialize conversion ratios
    HealthPerStamina = 10.0f;
    ManaPerIntellect = 15.0f;
    ArmorPerAgility = 2.0f;
    CritPerAgility = 0.05f;
    ManaRegenPerSpirit = 0.5f;
    BaseHealth = 50.0f;
    BaseMana = 50.0f;
    
    // Initialize primary attributes with default values
    // These will be overridden by GameplayEffects
    Strength.SetBaseValue(10.0f);
    Agility.SetBaseValue(10.0f);
    Intellect.SetBaseValue(10.0f);
    Stamina.SetBaseValue(10.0f);
    Spirit.SetBaseValue(10.0f);
    
    // Initialize secondary attributes
    Armor.SetBaseValue(0.0f);
    CriticalStrikeChance.SetBaseValue(5.0f); // Base 5% crit
    HasteRating.SetBaseValue(0.0f);
    MasteryRating.SetBaseValue(0.0f);
    VersatilityRating.SetBaseValue(0.0f);
    
    // Initialize vital attributes - will be calculated in UpdateDerivedAttributes
    // We set temporary values to avoid issues in the UI
    Health.SetBaseValue(1.0f);
    MaxHealth.SetBaseValue(1.0f);
    HealthRegenRate.SetBaseValue(0.0f);
    Mana.SetBaseValue(1.0f);
    MaxMana.SetBaseValue(1.0f);
    ManaRegenRate.SetBaseValue(0.0f);
}

void UWoWAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // Primary attributes - replicate to everyone
    DOREPLIFETIME(UWoWAttributeSet, Strength);
    DOREPLIFETIME(UWoWAttributeSet, Agility);
    DOREPLIFETIME(UWoWAttributeSet, Intellect);
    DOREPLIFETIME(UWoWAttributeSet, Stamina);
    DOREPLIFETIME(UWoWAttributeSet, Spirit);
    
    // Secondary attributes
    DOREPLIFETIME(UWoWAttributeSet, Armor);
    DOREPLIFETIME(UWoWAttributeSet, CriticalStrikeChance);
    DOREPLIFETIME(UWoWAttributeSet, HasteRating);
    DOREPLIFETIME(UWoWAttributeSet, MasteryRating);
    DOREPLIFETIME(UWoWAttributeSet, VersatilityRating);
    
    // Vital attributes
    DOREPLIFETIME(UWoWAttributeSet, Health);
    DOREPLIFETIME(UWoWAttributeSet, MaxHealth);
    DOREPLIFETIME(UWoWAttributeSet, HealthRegenRate);
    DOREPLIFETIME(UWoWAttributeSet, Mana);
    DOREPLIFETIME(UWoWAttributeSet, MaxMana);
    DOREPLIFETIME(UWoWAttributeSet, ManaRegenRate);
}

void UWoWAttributeSet::UpdateDerivedAttributes(UAbilitySystemComponent* AbilityComp)
{
    if (!AbilityComp || !AbilityComp->IsOwnerActorAuthoritative())
    {
        // Only the server should modify attributes directly
        return;
    }

    // Store current values and percentages
    float CurrentHealthPct = (GetMaxHealth() > 0.0f) ? (GetHealth() / GetMaxHealth()) : 1.0f;
    float CurrentManaPct = (GetMaxMana() > 0.0f) ? (GetMana() / GetMaxMana()) : 1.0f;
    
    // Calculate new max values
    float NewMaxHealth = BaseHealth + (GetStamina() * HealthPerStamina);
    float NewMaxMana = BaseMana + (GetIntellect() * ManaPerIntellect);
    
    // Calculate regeneration rates
    float NewHealthRegenRate = 0.5f + (GetSpirit() * 0.1f);
    float NewManaRegenRate = 1.0f + (GetSpirit() * ManaRegenPerSpirit);
    
    // Calculate armor and crit from agility
    float NewArmor = GetAgility() * ArmorPerAgility;
    float NewCritChance = 5.0f + (GetAgility() * CritPerAgility);
    
    // Use the AbilitySystemComponent to apply these changes
    // This ensures proper replication
    AbilityComp->SetNumericAttributeBase(GetMaxHealthAttribute(), NewMaxHealth);
    AbilityComp->SetNumericAttributeBase(GetHealthRegenRateAttribute(), NewHealthRegenRate);
    AbilityComp->SetNumericAttributeBase(GetMaxManaAttribute(), NewMaxMana);
    AbilityComp->SetNumericAttributeBase(GetManaRegenRateAttribute(), NewManaRegenRate);
    AbilityComp->SetNumericAttributeBase(GetArmorAttribute(), NewArmor);
    AbilityComp->SetNumericAttributeBase(GetCriticalStrikeChanceAttribute(), NewCritChance);
    
    // Calculate new current health/mana while maintaining percentage
    float NewHealth = CurrentHealthPct * NewMaxHealth;
    float NewMana = CurrentManaPct * NewMaxMana;
    
    // Only set health/mana if significantly different (prevents minor floating point issues)
    if (!FMath::IsNearlyEqual(GetHealth(), NewHealth, 0.1f))
    {
        AbilityComp->SetNumericAttributeBase(GetHealthAttribute(), NewHealth);
    }
    
    if (!FMath::IsNearlyEqual(GetMana(), NewMana, 0.1f))
    {
        AbilityComp->SetNumericAttributeBase(GetManaAttribute(), NewMana);
    }
}

void UWoWAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWoWAttributeSet, Health, OldHealth);
}

void UWoWAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWoWAttributeSet, Mana, OldMana);
}

void UWoWAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    
    // Clamp health and mana between 0 and their max values
    if (Attribute == GetHealthAttribute())
    {
        // Only clamp if MaxHealth is greater than 0
        if (GetMaxHealth() > 0.0f)
        {
            NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
        }
    }
    else if (Attribute == GetManaAttribute())
    {
        // Only clamp if MaxMana is greater than 0
        if (GetMaxMana() > 0.0f)
        {
            NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
        }
    }
}

void UWoWAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    
    // Get the Target actor, which should be our owner
    AActor* TargetActor = nullptr;
    AController* TargetController = nullptr;
    APawn* TargetPawn = nullptr;
    
    if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
    {
        TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
        TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
        TargetPawn = Cast<APawn>(TargetActor);
    }
    
    // Check if a primary attribute was modified
    bool bPrimaryAttributeChanged = 
        Data.EvaluatedData.Attribute == GetStrengthAttribute() ||
        Data.EvaluatedData.Attribute == GetAgilityAttribute() ||
        Data.EvaluatedData.Attribute == GetIntellectAttribute() ||
        Data.EvaluatedData.Attribute == GetStaminaAttribute() ||
        Data.EvaluatedData.Attribute == GetSpiritAttribute();
    
    // Handle changes to different attributes
    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        // Handle health change
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        
        // Here you could handle death if health <= 0
    }
    else if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
        // Handle mana change
        SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
    }
    else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
    {
        // Handle max health change
        AdjustAttributeForMaxChange(Health, MaxHealth, GetMaxHealth(), GetHealthAttribute());
    }
    else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
    {
        // Handle max mana change
        AdjustAttributeForMaxChange(Mana, MaxMana, GetMaxMana(), GetManaAttribute());
    }
    
    // If a primary attribute changed, update derived attributes
    if (bPrimaryAttributeChanged)
    {
        // CHANGE THIS LINE - use the correct way to get the ASC
        UAbilitySystemComponent* ASC = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
        if (!ASC)
        {
            // Try alternate method if the first one failed
            ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
        }
        
        if (ASC)
        {
            // Update the derived attributes
            UpdateDerivedAttributes(ASC);
        }
    }
}

void UWoWAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
    // Get current values
    float CurrentValue = AffectedAttribute.GetCurrentValue();
    float CurrentMax = MaxAttribute.GetCurrentValue();
    
    if (!FMath::IsNearlyEqual(CurrentMax, NewMaxValue) && CurrentMax > 0.0f)
    {
        // Calculate new value based on percentage of max
        float ValuePercentage = (CurrentValue > 0.0f) ? (CurrentValue / CurrentMax) : 1.0f;
        float NewValue = NewMaxValue * ValuePercentage;
        
        // Set the new value
        AffectedAttribute.SetCurrentValue(NewValue);
    }
}