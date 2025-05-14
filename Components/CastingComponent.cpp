#include "CastingComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    bShowingInterrupted = false;
    
    // Configure movement threshold
    MovementInterruptThreshold = 10.0f;
}

void UCastingComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Record initial position
    if (GetOwner())
    {
        PreviousLocation = GetOwner()->GetActorLocation();
    }
}

void UCastingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UCastingComponent, CastingState);
    DOREPLIFETIME(UCastingComponent, SpellName);
    DOREPLIFETIME(UCastingComponent, CastDuration);
    DOREPLIFETIME(UCastingComponent, bCanCastWhileMoving);
    DOREPLIFETIME(UCastingComponent, bShowingInterrupted);
}

void UCastingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (CastingState == ECastingState::Casting)
    {
        // Update cast progress for client-side prediction
        UpdateClientCastProgress(DeltaTime);
        
        // Check if the character is moving while casting
        // Only interrupt if we can't cast while moving
        if (!bCanCastWhileMoving)
        {
            CheckMovementInterrupt();
        }
    }
}

void UCastingComponent::CheckMovementInterrupt()
{
    // Only check movement on locally controlled pawns to avoid network jitter
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        return;
    }
    
    // Check if we're moving too fast to maintain the cast
    float VelocityMagnitude = GetOwnerVelocityMagnitude();
    if (VelocityMagnitude > MovementInterruptThreshold)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cast interrupted due to movement. Velocity: %.2f, Threshold: %.2f"), 
               VelocityMagnitude, MovementInterruptThreshold);
               
        // Interrupt the cast due to movement
        NotifyCastInterrupted();
    }
}

bool UCastingComponent::IsOwnerMoving() const
{
    return GetOwnerVelocityMagnitude() > MovementInterruptThreshold;
}

float UCastingComponent::GetOwnerVelocityMagnitude() const
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (Character)
    {
        // Use velocity from character movement component
        if (Character->GetCharacterMovement())
        {
            return Character->GetCharacterMovement()->Velocity.Size();
        }
        
        // Fallback to calculating velocity using actor location
        FVector CurrentLocation = Character->GetActorLocation();
        float DeltaTime = GetWorld()->GetDeltaSeconds();
        
        if (DeltaTime > 0.0f)
        {
            // Calculate velocity based on position change
            float DistanceMoved = FVector::Dist(CurrentLocation, PreviousLocation);
            return DistanceMoved / DeltaTime;
        }
    }
    
    return 0.0f;
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
            NotifyCastCompleted();
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
    
    // Clear any previous interrupt state
    bShowingInterrupted = false;
    GetWorld()->GetTimerManager().ClearTimer(InterruptTimerHandle);
    
    // Set all the casting data
    SpellName = InSpellName;
    CastDuration = InCastTime;
    CastStartTime = LocalStartTime;
    CastEndTime = CastStartTime + CastDuration;
    ClientCastProgress = 0.0f;
    bCanCastWhileMoving = InCanCastWhileMoving;
    
    // Update the cast state
    CastingState = ECastingState::Casting;
    
    // Store the current position to detect movement
    if (GetOwner())
    {
        PreviousLocation = GetOwner()->GetActorLocation();
    }
    
    UE_LOG(LogTemp, Log, TEXT("Cast Started: %s, Duration: %.2f, StartTime: %.2f, EndTime: %.2f, CanMoveWhileCasting: %d"), 
           *SpellName, CastDuration, CastStartTime, CastEndTime, bCanCastWhileMoving ? 1 : 0);
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
    
    // Set state to Interrupted
    CastingState = ECastingState::Interrupted;
    
    // Freeze the cast progress where it was interrupted
    // This will be used for the original cast bar
    
    // Enable the interrupted flag for showing the overlay
    bShowingInterrupted = true;
    
    // Cancel any active timers for this cast
    if (GetOwner())
    {
        UWorld* World = GetOwner()->GetWorld();
        if (World)
        {
            // Cancel any active timers on the owner - this is important to prevent
            // delayed abilities from activating after interruption
            UWoWGameplayAbilityBase* ActiveAbility = nullptr;
            AActor* OwnerActor = GetOwner();
            
            APawn* Pawn = Cast<APawn>(OwnerActor);
            if (Pawn && Pawn->Controller)
            {
                // Check controller for pending ability activation timers
                World->GetTimerManager().ClearAllTimersForObject(Pawn->Controller);
            }
            
            // Clear timers on the owner too
            World->GetTimerManager().ClearAllTimersForObject(OwnerActor);
        }
    }
    
    // Set up a timer to clear the interrupt state after a delay
    GetWorld()->GetTimerManager().ClearTimer(InterruptTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        InterruptTimerHandle, 
        this, 
        &UCastingComponent::ClearInterruptState, 
        2.0f, // 2 seconds
        false
    );
    
    UE_LOG(LogTemp, Log, TEXT("Cast Interrupted: %s"), *SpellName);
}

void UCastingComponent::ClearInterruptState()
{
    // Clear the interrupted state
    bShowingInterrupted = false;
    
    // If we're still in Interrupted state, set to Idle
    if (CastingState == ECastingState::Interrupted)
    {
        CastingState = ECastingState::Idle;
    }
}

bool UCastingComponent::IsCasting() const
{
    return CastingState == ECastingState::Casting;
}

bool UCastingComponent::IsInterrupted() const
{
    return bShowingInterrupted;
}

FString UCastingComponent::GetSpellName() const
{
    return SpellName;
}

float UCastingComponent::GetCastProgress() const
{
    return ClientCastProgress;
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
    else if (CastingState == ECastingState::Interrupted)
    {
        // The progress bar will be frozen at its current value
        // bShowingInterrupted will be set separately via replication
    }
}