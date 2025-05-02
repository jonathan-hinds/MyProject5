// File: EffectApplicationComponent.cpp
#include "EffectApplicationComponent.h"
#include "../Data/EffectDataAsset.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NiagaraFunctionLibrary.h" // Add this for Niagara
#include "NiagaraComponent.h" // Add this too
#include "Kismet/GameplayStatics.h"
#include "../Character/WoWCharacterBase.h"
#include "../Attributes/WoWAttributeSet.h"
#include "GameplayEffect.h"
#include "AbilitySystemGlobals.h"

UEffectApplicationComponent::UEffectApplicationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CachedEffectDataAsset = nullptr;
}

void UEffectApplicationComponent::BeginPlay()
{
    Super::BeginPlay();
}

FActiveGameplayEffectHandle UEffectApplicationComponent::ApplyEffectToTarget(int32 EffectID, AActor* TargetActor, float Level)
{
    if (!CachedEffectDataAsset || !TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: Missing data asset or target"));
        OnEffectApplied.Broadcast(EffectID, TargetActor, false);
        return FActiveGameplayEffectHandle();
    }
    
    // Get the effect data
    FEffectTableRow EffectData;
    if (!CachedEffectDataAsset->GetEffectDataByID(EffectID, EffectData))
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: Effect ID %d not found"), EffectID);
        OnEffectApplied.Broadcast(EffectID, TargetActor, false);
        return FActiveGameplayEffectHandle();
    }
    
    // Get the ASC from the target
    UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(TargetActor);
    if (!TargetASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: Target has no AbilitySystemComponent"));
        OnEffectApplied.Broadcast(EffectID, TargetActor, false);
        return FActiveGameplayEffectHandle();
    }
    
    // Get the source actor (owner of this component)
    AActor* SourceActor = GetOwner();
    if (!SourceActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: No source actor"));
        OnEffectApplied.Broadcast(EffectID, TargetActor, false);
        return FActiveGameplayEffectHandle();
    }
    
    // Check tags on the target
    if (EffectData.RequiredTargetTags.Num() > 0)
    {
        FGameplayTagContainer TargetTags;
        TargetASC->GetOwnedGameplayTags(TargetTags);
        
        if (!TargetTags.HasAll(EffectData.RequiredTargetTags))
        {
            // Missing required tags
            UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: Target missing required tags"));
            OnEffectApplied.Broadcast(EffectID, TargetActor, false);
            return FActiveGameplayEffectHandle();
        }
    }
    
    if (EffectData.ForbiddenTargetTags.Num() > 0)
    {
        FGameplayTagContainer TargetTags;
        TargetASC->GetOwnedGameplayTags(TargetTags);
        
        if (TargetTags.HasAny(EffectData.ForbiddenTargetTags))
        {
            // Has forbidden tags
            UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: Target has forbidden tags"));
            OnEffectApplied.Broadcast(EffectID, TargetActor, false);
            return FActiveGameplayEffectHandle();
        }
    }
    
    // Get the GE class
    UClass* GEClass = EffectData.GameplayEffectClass.LoadSynchronous();
    if (!GEClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: GameplayEffect class not found for effect ID %d"), EffectID);
        OnEffectApplied.Broadcast(EffectID, TargetActor, false);
        return FActiveGameplayEffectHandle();
    }
    
    // Calculate magnitude
    float Magnitude = CalculateEffectMagnitude(EffectData, SourceActor, TargetActor, Level);
    
    // Create the effect spec
    FGameplayEffectSpecHandle SpecHandle = CreateEffectSpec(EffectData, SourceActor, TargetActor, Level, Magnitude);
    
    if (!SpecHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ApplyEffectToTarget: Failed to create valid effect spec"));
        OnEffectApplied.Broadcast(EffectID, TargetActor, false);
        return FActiveGameplayEffectHandle();
    }
    
    // Apply the effect
    FActiveGameplayEffectHandle ActiveHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    
    // Play application feedback
    PlayEffectFeedback(EffectData, TargetActor, false);
    
    // If it's a duration effect, also play persistent feedback
    if (EffectData.DurationType == EEffectDurationType::Duration)
    {
        PlayEffectFeedback(EffectData, TargetActor, true);
    }
    
    // Broadcast the event
    OnEffectApplied.Broadcast(EffectID, TargetActor, ActiveHandle.IsValid());
    
    return ActiveHandle;
}

bool UEffectApplicationComponent::ApplyEffectsToTarget(const TArray<int32>& EffectIDs, AActor* TargetActor, float Level)
{
    if (!TargetActor || EffectIDs.Num() == 0)
    {
        return false;
    }
    
    bool Success = true;
    for (int32 EffectID : EffectIDs)
    {
        FActiveGameplayEffectHandle Handle = ApplyEffectToTarget(EffectID, TargetActor, Level);
        if (!Handle.IsValid())
        {
            Success = false;
        }
    }
    
    return Success;
}

bool UEffectApplicationComponent::ApplyEffectContainerToTarget(const FEffectContainerSpec& EffectContainer, AActor* TargetActor, float Level)
{
    return ApplyEffectsToTarget(EffectContainer.EffectIDs, TargetActor, Level);
}

int32 UEffectApplicationComponent::ApplyAreaEffects(const TArray<int32>& EffectIDs, AActor* CenterActor, float Radius, float Level)
{
    if (!CenterActor || EffectIDs.Num() == 0 || Radius <= 0.0f)
    {
        return 0;
    }
    
    // Get all actors in range
    TArray<AActor*> OverlappingActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), OverlappingActors);
    
    const FVector CenterLocation = CenterActor->GetActorLocation();
    int32 SuccessCount = 0;
    
    // Filter by distance and apply effects
    for (AActor* Actor : OverlappingActors)
    {
        if (Actor != CenterActor && Actor)
        {
            float Distance = FVector::Dist(CenterLocation, Actor->GetActorLocation());
            if (Distance <= Radius)
            {
                if (ApplyEffectsToTarget(EffectIDs, Actor, Level))
                {
                    SuccessCount++;
                }
            }
        }
    }
    
    return SuccessCount;
}

float UEffectApplicationComponent::CalculateEffectMagnitude(const FEffectTableRow& EffectData, AActor* SourceActor, AActor* TargetActor, float Level)
{
    const FEffectScalingInfo& ScalingInfo = EffectData.Magnitude;
    float Result = ScalingInfo.BaseValue;
    
    switch (ScalingInfo.MagnitudeType)
    {
        case EEffectMagnitudeType::Flat:
            // Already set to base value
            break;
            
        case EEffectMagnitudeType::ScaledByStat:
            if (SourceActor && ScalingInfo.StatTag.IsValid())
            {
                float StatValue = GetStatValue(SourceActor, ScalingInfo.StatTag);
                Result += (StatValue * ScalingInfo.ScalingCoefficient);
            }
            break;
            
        case EEffectMagnitudeType::ScaledByLevel:
            Result += (Level * ScalingInfo.ScalingCoefficient);
            break;
            
        case EEffectMagnitudeType::Custom:
            // Custom formulas would need to be implemented in a subclass
            // For now we'll just use the base value
            break;
    }
    
    return Result;
}

UAbilitySystemComponent* UEffectApplicationComponent::GetAbilitySystemComponent(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }
    
    // Try the interface first
    IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Actor);
    if (ASCInterface)
    {
        return ASCInterface->GetAbilitySystemComponent();
    }
    
    // Try finding the component directly
    return Actor->FindComponentByClass<UAbilitySystemComponent>();
}

void UEffectApplicationComponent::PlayEffectFeedback(const FEffectTableRow& EffectData, AActor* TargetActor, bool IsPersistent)
{
    if (!TargetActor)
    {
        return;
    }
    
    // Play VFX
    UNiagaraSystem* VFX = nullptr;
    if (IsPersistent)
    {
        VFX = EffectData.PersistentVFX.LoadSynchronous();
    }
    else
    {
        VFX = EffectData.ApplicationVFX.LoadSynchronous();
    }
    
    if (VFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            VFX,
            TargetActor->GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true
        );
    }
    
    // Play sound
    USoundBase* Sound = nullptr;
    if (IsPersistent)
    {
        Sound = EffectData.PersistentSound.LoadSynchronous();
    }
    else
    {
        Sound = EffectData.ApplicationSound.LoadSynchronous();
    }
    
    if (Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            Sound,
            TargetActor->GetActorLocation()
        );
    }
}

float UEffectApplicationComponent::GetStatValue(AActor* Actor, const FGameplayTag& StatTag) const
{
    if (!Actor || !StatTag.IsValid())
    {
        return 0.0f;
    }
    
    // Try to get the character
    AWoWCharacterBase* Character = Cast<AWoWCharacterBase>(Actor);
    if (!Character)
    {
        return 0.0f;
    }
    
    // Get the attribute set
    UWoWAttributeSet* AttributeSet = Character->GetAttributeSet();
    if (!AttributeSet)
    {
        return 0.0f;
    }
    
    // Map the tag to an attribute
    FString TagString = StatTag.ToString();
    
    // Check for primary attributes
    if (TagString.Contains("Attribute.Primary.Strength"))
    {
        return AttributeSet->GetStrength();
    }
    else if (TagString.Contains("Attribute.Primary.Agility"))
    {
        return AttributeSet->GetAgility();
    }
    else if (TagString.Contains("Attribute.Primary.Intellect"))
    {
        return AttributeSet->GetIntellect();
    }
    else if (TagString.Contains("Attribute.Primary.Stamina"))
    {
        return AttributeSet->GetStamina();
    }
    else if (TagString.Contains("Attribute.Primary.Spirit"))
    {
        return AttributeSet->GetSpirit();
    }
    
    // Check for secondary attributes
    else if (TagString.Contains("Attribute.Secondary.Armor"))
    {
        return AttributeSet->GetArmor();
    }
    else if (TagString.Contains("Attribute.Secondary.CriticalStrikeChance"))
    {
        return AttributeSet->GetCriticalStrikeChance();
    }
    
    // Check for vital attributes
    else if (TagString.Contains("Attribute.Vital.Health"))
    {
        return AttributeSet->GetHealth();
    }
    else if (TagString.Contains("Attribute.Vital.MaxHealth"))
    {
        return AttributeSet->GetMaxHealth();
    }
    else if (TagString.Contains("Attribute.Vital.Mana"))
    {
        return AttributeSet->GetMana();
    }
    else if (TagString.Contains("Attribute.Vital.MaxMana"))
    {
        return AttributeSet->GetMaxMana();
    }
    
    return 0.0f;
}

FGameplayEffectSpecHandle UEffectApplicationComponent::CreateEffectSpec(const FEffectTableRow& EffectData, AActor* SourceActor, AActor* TargetActor, float Level, float CalculatedMagnitude)
{
    UClass* GEClass = EffectData.GameplayEffectClass.LoadSynchronous();
    if (!GEClass || !SourceActor)
    {
        return FGameplayEffectSpecHandle();
    }
    
    // Get source ASC
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent(SourceActor);
    if (!SourceASC)
    {
        // Fallback to creating a context without a source ASC
        FGameplayEffectContextHandle EffectContext = FGameplayEffectContextHandle(UAbilitySystemGlobals::Get().AllocGameplayEffectContext());
        EffectContext.AddSourceObject(SourceActor);
        
        // Create spec
        FGameplayEffectSpecHandle SpecHandle = FGameplayEffectSpecHandle(new FGameplayEffectSpec(GEClass->GetDefaultObject<UGameplayEffect>(), EffectContext, Level));
        
        // Handle damage effects
        if (EffectData.EffectType == EEffectType::Damage || EffectData.EffectType == EEffectType::DamageOverTime)
        {
            FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
            SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -CalculatedMagnitude);
            UE_LOG(LogTemp, Warning, TEXT("Setting damage magnitude to %.2f"), -CalculatedMagnitude);
        }
        // Handle healing effects
        else if (EffectData.EffectType == EEffectType::Healing || EffectData.EffectType == EEffectType::HealingOverTime)
        {
            FGameplayTag HealingTag = FGameplayTag::RequestGameplayTag(FName("Data.Healing"));
            SpecHandle.Data->SetSetByCallerMagnitude(HealingTag, CalculatedMagnitude);
        }
        // Other effects use generic magnitude tag
        else 
        {
            FGameplayTag MagnitudeTag = FGameplayTag::RequestGameplayTag(FName("Data.Magnitude"));
            SpecHandle.Data->SetSetByCallerMagnitude(MagnitudeTag, CalculatedMagnitude);
        }
        
        return SpecHandle;
    }
    
    // Create effect context with source ASC
    FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
    EffectContext.AddSourceObject(SourceActor);
    
    // Create spec
    FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(GEClass, Level, EffectContext);
    if (SpecHandle.IsValid())
    {
        // Handle damage effects
        if (EffectData.EffectType == EEffectType::Damage || EffectData.EffectType == EEffectType::DamageOverTime)
        {
            FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
            SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -CalculatedMagnitude);
            UE_LOG(LogTemp, Warning, TEXT("Setting damage magnitude to %.2f"), -CalculatedMagnitude);
        }
        // Handle healing effects
        else if (EffectData.EffectType == EEffectType::Healing || EffectData.EffectType == EEffectType::HealingOverTime)
        {
            FGameplayTag HealingTag = FGameplayTag::RequestGameplayTag(FName("Data.Healing"));
            SpecHandle.Data->SetSetByCallerMagnitude(HealingTag, CalculatedMagnitude);
        }
        // Other effects use generic magnitude tag
        else 
        {
            FGameplayTag MagnitudeTag = FGameplayTag::RequestGameplayTag(FName("Data.Magnitude"));
            SpecHandle.Data->SetSetByCallerMagnitude(MagnitudeTag, CalculatedMagnitude);
        }
    }
    
    return SpecHandle;
}