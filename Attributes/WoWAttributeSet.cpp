#include "WoWAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UWoWAttributeSet::UWoWAttributeSet()
{
    // Initialize primary attributes
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
    
    // Initialize vital attributes
    Health.SetBaseValue(100.0f);
    MaxHealth.SetBaseValue(100.0f);
    HealthRegenRate.SetBaseValue(1.0f);
    Mana.SetBaseValue(100.0f);
    MaxMana.SetBaseValue(100.0f);
    ManaRegenRate.SetBaseValue(2.0f);
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
    
    UE_LOG(LogTemp, Warning, TEXT("PreAttributeChange: Attribute=%s, NewValue=%f"), 
        *Attribute.GetName(), NewValue);
    
    // Clamp health and mana between 0 and their max values
    if (Attribute == GetHealthAttribute())
    {
        // Only clamp if MaxHealth is greater than 0
        // This prevents the initialization issue
        if (GetMaxHealth() > 0.0f)
        {
            NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
            UE_LOG(LogTemp, Warning, TEXT("Clamped Health to: %f (Max: %f)"), 
                NewValue, GetMaxHealth());
        }
        else
        {
            // Don't clamp during initialization when MaxHealth might not be set yet
            UE_LOG(LogTemp, Warning, TEXT("Skipped clamping Health: %f (Max not set yet)"), 
                NewValue);
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
    
    // Get the Source actor
    AActor* SourceActor = nullptr;
    AController* SourceController = nullptr;
    APawn* SourcePawn = nullptr;
    
    if (Data.EffectSpec.GetContext().GetSourceObject())
    {
        SourceActor = Cast<AActor>(Data.EffectSpec.GetContext().GetSourceObject());
        
        if (SourceActor)
        {
            SourceController = Cast<AController>(SourceActor);
            if (!SourceController && SourceActor->GetInstigatorController())
            {
                SourceController = SourceActor->GetInstigatorController();
            }
            
            if (!SourceController && SourceActor->GetOwner())
            {
                SourceController = Cast<AController>(SourceActor->GetOwner());
            }
            
            if (SourceController)
            {
                SourcePawn = SourceController->GetPawn();
            }
        }
    }
    
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
    // You can add handlers for other attributes as needed
}

void UWoWAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
    // Get current values
    float CurrentValue = AffectedAttribute.GetCurrentValue();
    float CurrentMax = MaxAttribute.GetCurrentValue();
    
    if (!FMath::IsNearlyEqual(CurrentMax, NewMaxValue) && CurrentMax > 0.0f)
    {
        // Calculate new value based on percentage of max
        // IMPORTANT CHANGE: If current value is 0, set to full health
        float ValuePercentage = (CurrentValue > 0.0f) ? (CurrentValue / CurrentMax) : 1.0f;
        float NewValue = NewMaxValue * ValuePercentage;
        
        // Set the new value
        AffectedAttribute.SetCurrentValue(NewValue);
    }
}