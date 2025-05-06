#include "CastingComponent.h"
#include "Net/UnrealNetwork.h"

UCastingComponent::UCastingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    
    CastingState = ECastingState::Idle;
    SpellName = FString();
    CastDuration = 0.0f;
    CastStartTime = 0.0f;
    CastEndTime = 0.0f;
    ClientCastProgress = 0.0f;
    bCanCastWhileMoving = false;
}

void UCastingComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UCastingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UCastingComponent, CastingState);
    DOREPLIFETIME(UCastingComponent, SpellName);
    DOREPLIFETIME(UCastingComponent, CastDuration);
    DOREPLIFETIME(UCastingComponent, bCanCastWhileMoving);
}

void UCastingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (CastingState == ECastingState::Casting)
    {
        UpdateClientCastProgress(DeltaTime);
    }
}

void UCastingComponent::UpdateClientCastProgress(float DeltaTime)
{
    if (CastDuration <= 0.0f)
    {
        return;
    }
    
    float CurrentTime = GetWorldTime();
    
    if (CurrentTime >= CastEndTime)
    {
        ClientCastProgress = 1.0f;
        
        if (GetOwnerRole() < ROLE_Authority)
        {
            UE_LOG(LogTemp, Log, TEXT("Client-side cast completed"));
        }
    }
    else
    {
        float ElapsedTime = CurrentTime - CastStartTime;
        float NewProgress = FMath::Clamp(ElapsedTime / CastDuration, 0.0f, 1.0f);
        
        ClientCastProgress = NewProgress;
    }
}

void UCastingComponent::NotifyCastStarted(const FString& InSpellName, float InCastTime, bool InCanCastWhileMoving)
{
    float LocalStartTime = GetWorldTime();
    
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_NotifyCastStarted(InSpellName, InCastTime, InCanCastWhileMoving);
    }
    
    SpellName = InSpellName;
    CastDuration = InCastTime;
    CastStartTime = LocalStartTime;
    CastEndTime = CastStartTime + CastDuration;
    ClientCastProgress = 0.0f;
    bCanCastWhileMoving = InCanCastWhileMoving;
    CastingState = ECastingState::Casting;
    
    UE_LOG(LogTemp, Log, TEXT("Cast Started: %s, Duration: %.2f, StartTime: %.2f, EndTime: %.2f, IsServer: %d"), 
           *SpellName, CastDuration, CastStartTime, CastEndTime, (GetOwnerRole() == ROLE_Authority));
}

void UCastingComponent::NotifyCastCompleted()
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_NotifyCastCompleted();
    }
    
    CastingState = ECastingState::Idle;
    ClientCastProgress = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("Cast Completed: %s"), *SpellName);
}

void UCastingComponent::NotifyCastInterrupted()
{
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_NotifyCastInterrupted();
    }
    
    CastingState = ECastingState::Interrupted;
    ClientCastProgress = 0.0f;
    
    GetWorld()->GetTimerManager().ClearTimer(InterruptResetTimer);
    GetWorld()->GetTimerManager().SetTimer(InterruptResetTimer, this, &UCastingComponent::ResetAfterInterrupt, 1.0f, false);
    
    UE_LOG(LogTemp, Log, TEXT("Cast Interrupted: %s"), *SpellName);
}

void UCastingComponent::ResetAfterInterrupt()
{
    if (CastingState == ECastingState::Interrupted)
    {
        CastingState = ECastingState::Idle;
    }
}

bool UCastingComponent::IsCasting() const
{
    return CastingState == ECastingState::Casting;
}

bool UCastingComponent::WasInterrupted() const
{
    return CastingState == ECastingState::Interrupted;
}

FString UCastingComponent::GetSpellName() const
{
    return SpellName;
}

float UCastingComponent::GetCastProgress() const
{
    if (CastingState == ECastingState::Casting)
    {
        return ClientCastProgress;
    }
    return 0.0f;
}

float UCastingComponent::GetWorldTime() const
{
    UWorld* World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0f;
}

void UCastingComponent::Server_NotifyCastStarted_Implementation(const FString& InSpellName, float InCastTime, bool InCanCastWhileMoving)
{
    NotifyCastStarted(InSpellName, InCastTime, InCanCastWhileMoving);
}

void UCastingComponent::Server_NotifyCastCompleted_Implementation()
{
    NotifyCastCompleted();
}

void UCastingComponent::Server_NotifyCastInterrupted_Implementation()
{
    NotifyCastInterrupted();
}

void UCastingComponent::OnRep_CastingState()
{
    UE_LOG(LogTemp, Log, TEXT("Casting state changed to: %s, Spell: %s"), 
           (CastingState == ECastingState::Idle) ? TEXT("Idle") : 
           (CastingState == ECastingState::Casting) ? TEXT("Casting") : TEXT("Interrupted"), 
           *SpellName);
    
    if (CastingState == ECastingState::Casting)
    {
        CastStartTime = GetWorldTime();
        CastEndTime = CastStartTime + CastDuration;
        ClientCastProgress = 0.0f;
    }
}