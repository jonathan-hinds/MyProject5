#include "WoWGameplayAbilityBase.h"
#include "../Components/EffectApplicationComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "../Data/EffectDataAsset.h"
#include "../Character/WoWCharacterBase.h"
#include "../Character/WoWPlayerCharacter.h"
#include "../Components/TargetingComponent.h"

UWoWGameplayAbilityBase::UWoWGameplayAbilityBase()
{
    AbilityID = 0;
    bAbilityDataLoaded = false;
}

bool UWoWGameplayAbilityBase::GetAbilityData(FAbilityTableRow& OutAbilityData) const
{
    if (bAbilityDataLoaded)
    {
        OutAbilityData = CachedAbilityData;
        return true;
    }
    
    // If not already loaded, try to load it
    if (AbilityDataAsset)
    {
        bool bFound = AbilityDataAsset->GetAbilityDataByID(AbilityID, OutAbilityData);
        if (bFound)
        {
            // Cache it for future use (we need to cast away const here)
            UWoWGameplayAbilityBase* MutableThis = const_cast<UWoWGameplayAbilityBase*>(this);
            MutableThis->CachedAbilityData = OutAbilityData;
            MutableThis->bAbilityDataLoaded = true;
        }
        return bFound;
    }
    
    return false;
}

void UWoWGameplayAbilityBase::InitializeFromAbilityData(int32 InAbilityID)
{
    AbilityID = InAbilityID;
    bAbilityDataLoaded = false;
    
    // Try to load the ability data immediately
    FAbilityTableRow AbilityData;
    GetAbilityData(AbilityData);
}

UEffectApplicationComponent* UWoWGameplayAbilityBase::GetEffectComponent() const
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        return nullptr;
    }
    
    // Try to find existing component
    UEffectApplicationComponent* EffectComp = AvatarActor->FindComponentByClass<UEffectApplicationComponent>();
    
    // If no component found, create one
    if (!EffectComp)
    {
        // We need to cast away const here
        UWoWGameplayAbilityBase* MutableThis = const_cast<UWoWGameplayAbilityBase*>(this);
        AActor* MutableActor = const_cast<AActor*>(AvatarActor);
        
        EffectComp = NewObject<UEffectApplicationComponent>(MutableActor);
        EffectComp->RegisterComponent();
    }
    
    return EffectComp;
}

void UWoWGameplayAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    UE_LOG(LogTemp, Warning, TEXT("WoWAbility Activated - ID: %d"), AbilityID);
    
    // Try to load ability data if not already loaded
    if (!bAbilityDataLoaded)
    {
        // Try to find the data asset
        if (!AbilityDataAsset)
        {
            UE_LOG(LogTemp, Warning, TEXT("No AbilityDataAsset found, trying to get from character"));
            // Try to find from owner
            AWoWCharacterBase* Character = Cast<AWoWCharacterBase>(GetAvatarActorFromActorInfo());
            if (Character)
            {
                AbilityDataAsset = Character->GetAbilityDataAsset();
                if (AbilityDataAsset)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Found AbilityDataAsset from character"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Character has no AbilityDataAsset!"));
                }
            }
        }
        
        // Try to load ability data
        FAbilityTableRow AbilityData;
        bool bFound = GetAbilityData(AbilityData);
        if (bFound)
        {
            UE_LOG(LogTemp, Warning, TEXT("Loaded ability data: %s"), *AbilityData.DisplayName);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load ability data for ID: %d"), AbilityID);
        }
    }
    
    // Get the avatar actor (our character)
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        UE_LOG(LogTemp, Error, TEXT("No avatar actor found for ability"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    // Try to get the targeting component to find current target
    AWoWPlayerCharacter* PlayerCharacter = Cast<AWoWPlayerCharacter>(AvatarActor);
    if (PlayerCharacter)
    {
        UTargetingComponent* TargetingComp = PlayerCharacter->FindComponentByClass<UTargetingComponent>();
        if (TargetingComp)
        {
            AActor* TargetActor = TargetingComp->GetCurrentTarget();
            if (TargetActor)
            {
                UE_LOG(LogTemp, Warning, TEXT("Found target from targeting component: %s"), *TargetActor->GetName());
                
                // Apply effects to the target
                ApplyEffectsToTarget(TargetActor, EEffectContainerType::Target);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Targeting component returned no target"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No targeting component found on player character"));
        }
    }
    
    // Continue with base ability activation
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

bool UWoWGameplayAbilityBase::ApplyEffectsToTarget(AActor* TargetActor, EEffectContainerType ContainerType)
{
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: No target actor!"));
        return false;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Applying effects to target: %s"), *TargetActor->GetName());
    
    // Make sure ability data is loaded
    FAbilityTableRow AbilityData;
    if (!GetAbilityData(AbilityData))
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: Failed to get ability data"));
        return false;
    }
    
    // Get the effect container based on type
    const FEffectContainerSpec* EffectContainer = nullptr;
    switch (ContainerType)
    {
        case EEffectContainerType::Self:
            EffectContainer = &AbilityData.SelfEffects;
            UE_LOG(LogTemp, Warning, TEXT("Using SelfEffects container"));
            break;
            
        case EEffectContainerType::Target:
            EffectContainer = &AbilityData.TargetEffects;
            UE_LOG(LogTemp, Warning, TEXT("Using TargetEffects container with %d effects"), AbilityData.TargetEffects.EffectIDs.Num());
            break;
            
        case EEffectContainerType::Area:
            EffectContainer = &AbilityData.AreaEffects;
            UE_LOG(LogTemp, Warning, TEXT("Using AreaEffects container"));
            break;
    }
    
    if (!EffectContainer || EffectContainer->EffectIDs.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: No effect IDs in container!"));
        return false;
    }
    
    // Get the effect component
    UEffectApplicationComponent* EffectComp = GetEffectComponent();
    if (!EffectComp)
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: No EffectApplicationComponent found!"));
        return false;
    }
    
    // Check if effect data asset is set
    if (!EffectComp->GetEffectDataAsset())
    {
        UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToTarget: EffectApplicationComponent has no EffectDataAsset!"));
        return false;
    }
    
    // Apply the effects
    bool Success = EffectComp->ApplyEffectContainerToTarget(*EffectContainer, TargetActor, GetAbilityLevel());
    UE_LOG(LogTemp, Warning, TEXT("Effect application result: %s"), Success ? TEXT("Success") : TEXT("Failed"));
    return Success;
}

int32 UWoWGameplayAbilityBase::ApplyAreaEffects(AActor* CenterActor, float Radius, EEffectContainerType ContainerType)
{
    if (!CenterActor)
    {
        return 0;
    }
    
    // Make sure ability data is loaded
    FAbilityTableRow AbilityData;
    if (!GetAbilityData(AbilityData))
    {
        return 0;
    }
    
    // Get the effect container based on type
    const FEffectContainerSpec* EffectContainer = nullptr;
    switch (ContainerType)
    {
        case EEffectContainerType::Self:
            EffectContainer = &AbilityData.SelfEffects;
            break;
            
        case EEffectContainerType::Target:
            EffectContainer = &AbilityData.TargetEffects;
            break;
            
        case EEffectContainerType::Area:
            EffectContainer = &AbilityData.AreaEffects;
            break;
    }
    
    if (!EffectContainer || EffectContainer->EffectIDs.Num() == 0)
    {
        return 0;
    }
    
    // Get the effect component
    UEffectApplicationComponent* EffectComp = GetEffectComponent();
    if (!EffectComp)
    {
        return 0;
    }
    
    // Use radius from ability data if none provided
    if (Radius <= 0.0f && ContainerType == EEffectContainerType::Area)
    {
        Radius = AbilityData.AreaEffectRadius;
    }
    
    // Apply the area effects
    return EffectComp->ApplyAreaEffects(EffectContainer->EffectIDs, CenterActor, Radius, GetAbilityLevel());
}

const FEffectContainerSpec& UWoWGameplayAbilityBase::GetEffectContainer(EEffectContainerType ContainerType) const
{
    static FEffectContainerSpec EmptyContainer;
    
    // Make sure ability data is loaded
    FAbilityTableRow AbilityData;
    if (!GetAbilityData(AbilityData))
    {
        return EmptyContainer;
    }
    
    // Return the appropriate container
    switch (ContainerType)
    {
        case EEffectContainerType::Self:
            return AbilityData.SelfEffects;
            
        case EEffectContainerType::Target:
            return AbilityData.TargetEffects;
            
        case EEffectContainerType::Area:
            return AbilityData.AreaEffects;
            
        default:
            return EmptyContainer;
    }
}