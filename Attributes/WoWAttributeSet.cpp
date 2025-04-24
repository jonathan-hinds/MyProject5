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

// WoWAttributeSet.cpp - Enhanced PostGameplayEffectExecute function
void UWoWAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    
    // Log at the beginning of PostGameplayEffectExecute
    UE_LOG(LogTemp, Warning, TEXT("==== ATTRIBUTE SET - POST GE EXECUTE ===="));
    UE_LOG(LogTemp, Warning, TEXT("Attribute modified: %s"), *Data.EvaluatedData.Attribute.GetName());
    UE_LOG(LogTemp, Warning, TEXT("Magnitude: %.2f"), Data.EvaluatedData.Magnitude);
    
    // Get the Target actor, which should be our owner
    AActor* TargetActor = nullptr;
    AController* TargetController = nullptr;
    APawn* TargetPawn = nullptr;
    
    if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
    {
        TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
        UE_LOG(LogTemp, Warning, TEXT("Target Actor: %s"), *TargetActor->GetName());
        
        TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
        if (TargetController)
        {
            UE_LOG(LogTemp, Warning, TEXT("Target Controller: %s"), *TargetController->GetName());
        }
        
        TargetPawn = Cast<APawn>(TargetActor);
        if (TargetPawn)
        {
            UE_LOG(LogTemp, Warning, TEXT("Target is a Pawn: %s"), *TargetPawn->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid target actor info"));
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
        UE_LOG(LogTemp, Warning, TEXT("Health changed: Old=%.2f, New=%.2f"), GetHealth() - Data.EvaluatedData.Magnitude, GetHealth());
        
        // Handle health change
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        UE_LOG(LogTemp, Warning, TEXT("Health after clamping: %.2f / %.2f"), GetHealth(), GetMaxHealth());
        
        // Here you could handle death if health <= 0
        if (GetHealth() <= 0.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("Character health is zero or below - Death should be triggered"));
        }
    }
    else if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
        UE_LOG(LogTemp, Warning, TEXT("Mana changed: Old=%.2f, New=%.2f"), GetMana() - Data.EvaluatedData.Magnitude, GetMana());
        
        // Handle mana change
        SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
        UE_LOG(LogTemp, Warning, TEXT("Mana after clamping: %.2f / %.2f"), GetMana(), GetMaxMana());
    }
    else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
    {
        UE_LOG(LogTemp, Warning, TEXT("MaxHealth changed: Old=%.2f, New=%.2f"), 
            GetMaxHealth() - Data.EvaluatedData.Magnitude, GetMaxHealth());
        
        // Handle max health change
        AdjustAttributeForMaxChange(Health, MaxHealth, GetMaxHealth(), GetHealthAttribute());
        UE_LOG(LogTemp, Warning, TEXT("After adjustment: Health=%.2f, MaxHealth=%.2f"), GetHealth(), GetMaxHealth());
    }
    else if (Data.EvaluatedData.Attribute == GetMaxManaAttribute())
    {
        UE_LOG(LogTemp, Warning, TEXT("MaxMana changed: Old=%.2f, New=%.2f"), 
            GetMaxMana() - Data.EvaluatedData.Magnitude, GetMaxMana());
        
        // Handle max mana change
        AdjustAttributeForMaxChange(Mana, MaxMana, GetMaxMana(), GetManaAttribute());
        UE_LOG(LogTemp, Warning, TEXT("After adjustment: Mana=%.2f, MaxMana=%.2f"), GetMana(), GetMaxMana());
    }
    
    // If a primary attribute changed, update derived attributes
    if (bPrimaryAttributeChanged)
    {
        UE_LOG(LogTemp, Warning, TEXT("Primary attribute changed, updating derived attributes"));
        
        // CHANGE THIS LINE - use the correct way to get the ASC
        UAbilitySystemComponent* ASC = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
        if (!ASC)
        {
            UE_LOG(LogTemp, Warning, TEXT("Trying alternate method to get ASC"));
            // Try alternate method if the first one failed
            ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
        }
        
        if (ASC)
        {
            UE_LOG(LogTemp, Warning, TEXT("Found ASC, updating derived attributes"));
            // Update the derived attributes
            UpdateDerivedAttributes(ASC);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get ASC for updating derived attributes"));
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("==== END ATTRIBUTE SET - POST GE EXECUTE ===="));
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