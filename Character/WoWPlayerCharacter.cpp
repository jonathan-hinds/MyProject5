#include "WoWPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "../States/WoWPlayerState.h"
#include "../Character/WoWEnemyCharacter.h"
#include "../Components/TargetingComponent.h"
#include "../Components/HotbarComponent.h"
#include "../Components/EffectApplicationComponent.h" // Add this include
#include "../Data/AbilityDataAsset.h" // Add this include
#include "../Data/AbilityEffectTypes.h" // Add this include
#include "Engine/Engine.h"


// Replace your existing AWoWPlayerCharacter constructor in WoWPlayerCharacter.cpp:

AWoWPlayerCharacter::AWoWPlayerCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 600.f;
    GetCharacterMovement()->AirControl = 0.2f;
    
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeSize = 50.0f;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 15.0f;
    CameraBoom->CameraRotationLagSpeed = 10.0f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->ProbeChannel = ECC_Camera;
    
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
    
    CharacterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterMesh"));
    CharacterMesh->SetupAttachment(RootComponent);
    CharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
    
    TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
    HotbarComponent = CreateDefaultSubobject<UHotbarComponent>(TEXT("HotbarComponent"));
    CastingComponent = CreateDefaultSubobject<UCastingComponent>(TEXT("CastingComponent"));
    
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;
    
    MinCameraDistance = 150.0f;
    MaxCameraDistance = 800.0f;
    CameraZoomStep = 50.0f;
        ForwardInputValue = 0.0f;
    RightInputValue = 0.0f;
}

void AWoWPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
}

// Add these new functions to WoWPlayerCharacter.cpp:

void AWoWPlayerCharacter::ZoomIn()
{
    if (CameraBoom)
    {
        float NewTargetArmLength = FMath::Max(CameraBoom->TargetArmLength - CameraZoomStep, MinCameraDistance);
        CameraBoom->TargetArmLength = NewTargetArmLength;
    }
}

void AWoWPlayerCharacter::ZoomOut()
{
    if (CameraBoom)
    {
        float NewTargetArmLength = FMath::Min(CameraBoom->TargetArmLength + CameraZoomStep, MaxCameraDistance);
        CameraBoom->TargetArmLength = NewTargetArmLength;
    }
}

// Replace your existing SetupPlayerInputComponent function in WoWPlayerCharacter.cpp:

void AWoWPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    
    // Set up gameplay key bindings
    PlayerInputComponent->BindAxis("MoveForward", this, &AWoWPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AWoWPlayerCharacter::MoveRight);
    
    // Mouse look bindings
    PlayerInputComponent->BindAxis("Turn", this, &AWoWPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AWoWPlayerCharacter::LookUp);
    
    // Jump binding
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
    
    // Targeting bindings
    PlayerInputComponent->BindAction("TargetEnemy", IE_Pressed, this, &AWoWPlayerCharacter::OnTargetEnemy);
    PlayerInputComponent->BindAction("TargetAndAttackEnemy", IE_Pressed, this, &AWoWPlayerCharacter::OnTargetAndAttackEnemy);

    // Hotbar bindings
    PlayerInputComponent->BindAction("HotbarSlot1", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot1);
    PlayerInputComponent->BindAction("HotbarSlot2", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot2);
    PlayerInputComponent->BindAction("HotbarSlot3", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot3);
    PlayerInputComponent->BindAction("HotbarSlot4", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot4);
    PlayerInputComponent->BindAction("HotbarSlot5", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot5);
    PlayerInputComponent->BindAction("HotbarSlot6", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot6);
    PlayerInputComponent->BindAction("HotbarSlot7", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot7);
    PlayerInputComponent->BindAction("HotbarSlot8", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot8);
    PlayerInputComponent->BindAction("HotbarSlot9", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot9);
    PlayerInputComponent->BindAction("HotbarSlot10", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot10);
    PlayerInputComponent->BindAction("HotbarSlot11", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot11);
    PlayerInputComponent->BindAction("HotbarSlot12", IE_Pressed, this, &AWoWPlayerCharacter::OnHotbarSlot12);
    
    // Camera zoom bindings
    PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &AWoWPlayerCharacter::ZoomIn);
    PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &AWoWPlayerCharacter::ZoomOut);
}

void AWoWPlayerCharacter::MoveForward(float Value)
{
    // Keep your existing code
    UCastingComponent* CastComp = GetCastingComponent();
    if (CastComp && CastComp->IsCasting() && !CastComp->CanCastWhileMoving() && Value != 0.0f)
    {
        CastComp->NotifyCastInterrupted();
    }

    // Simply store the input value
    ForwardInputValue = Value;

    // Rest of your existing code remains unchanged
    if (Controller != nullptr && Value != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
    UpdateMeshRotation();
}


void AWoWPlayerCharacter::OnHotbarSlot1() { HandleHotbarSlot(0); }
void AWoWPlayerCharacter::OnHotbarSlot2() { HandleHotbarSlot(1); }
void AWoWPlayerCharacter::OnHotbarSlot3() { HandleHotbarSlot(2); }
void AWoWPlayerCharacter::OnHotbarSlot4() { HandleHotbarSlot(3); }
void AWoWPlayerCharacter::OnHotbarSlot5() { HandleHotbarSlot(4); }
void AWoWPlayerCharacter::OnHotbarSlot6() { HandleHotbarSlot(5); }
void AWoWPlayerCharacter::OnHotbarSlot7() { HandleHotbarSlot(6); }
void AWoWPlayerCharacter::OnHotbarSlot8() { HandleHotbarSlot(7); }
void AWoWPlayerCharacter::OnHotbarSlot9() { HandleHotbarSlot(8); }
void AWoWPlayerCharacter::OnHotbarSlot10() { HandleHotbarSlot(9); }
void AWoWPlayerCharacter::OnHotbarSlot11() { HandleHotbarSlot(10); }
void AWoWPlayerCharacter::OnHotbarSlot12() { HandleHotbarSlot(11); }

void AWoWPlayerCharacter::HandleHotbarSlot(int32 SlotIndex)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
            FString::Printf(TEXT("Hotbar slot %d pressed"), SlotIndex + 1));
    }
    
    if (HotbarComponent)
    {
        HotbarComponent->ActivateAbilityInSlot(SlotIndex);
    }
}

void AWoWPlayerCharacter::Jump()
{
    UCastingComponent* CastComp = GetCastingComponent();
    if (CastComp && CastComp->IsCasting() && !CastComp->CanCastWhileMoving())
    {
        CastComp->NotifyCastInterrupted();
        return;
    }

    Super::Jump();
}

void AWoWPlayerCharacter::MoveRight(float Value)
{
    // Keep your existing code
    UCastingComponent* CastComp = GetCastingComponent();
    if (CastComp && CastComp->IsCasting() && !CastComp->CanCastWhileMoving() && Value != 0.0f)
    {
        CastComp->NotifyCastInterrupted();
    }

    // Simply store the input value
    RightInputValue = Value;

    // Rest of your existing code remains unchanged
    if (Controller != nullptr && Value != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
    }
    UpdateMeshRotation();
}

void AWoWPlayerCharacter::UpdateMeshRotation()
{
    // Skip if feature is disabled
    if (!bRotateMeshToMatchMovement)
    {
        return;
    }
    
    // Get the skeletal mesh component
    USkeletalMeshComponent* SkelMesh = GetMesh();
    if (!SkelMesh || !Controller)
    {
        return;
    }
    
    // Check if we're moving diagonally
    bool bMovingDiagonally = FMath::Abs(ForwardInputValue) > 0.1f && FMath::Abs(RightInputValue) > 0.1f;
    
    // If not moving diagonally, reset to base rotation
    if (!bMovingDiagonally || (FMath::IsNearlyZero(ForwardInputValue) && FMath::IsNearlyZero(RightInputValue)))
    {
        // Reset to the base rotation for cardinal directions
        SkelMesh->SetRelativeRotation(MeshBaseRotation);
        return;
    }
    
    // Determine if we're moving backward diagonally
    bool bMovingBackwardDiagonally = ForwardInputValue < 0.0f && FMath::Abs(RightInputValue) > 0.1f;
    
    // For diagonal backward movement, we need special handling
    if (bMovingBackwardDiagonally)
    {
        // For backward diagonal movement, we want to face forward but move backward
        // So we need to rotate in the opposite direction to the movement
        
        // Calculate the angle based on right input (determines left/right direction)
        float YawOffset = (RightInputValue > 0.0f) ? -45.0f : 45.0f;
        
        // Apply the offset to the base rotation
        FRotator BackwardDiagonalRot = MeshBaseRotation;
        BackwardDiagonalRot.Yaw += YawOffset;
        
        // Apply the new rotation directly
        SkelMesh->SetRelativeRotation(BackwardDiagonalRot);
    }
    else if (bMovingDiagonally)
    {
        // For forward diagonal movement, proceed as before
        // Get the camera's forward and right vectors
        FRotator CameraRotation = Controller->GetControlRotation();
        CameraRotation.Pitch = 0.0f;
        CameraRotation.Roll = 0.0f;
        
        FVector ForwardVector = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::X);
        FVector RightVector = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);
        
        // Calculate movement direction based on input values
        FVector MovementDirection = ForwardVector * ForwardInputValue + RightVector * RightInputValue;
        
        // Only update if we have a valid direction
        if (!MovementDirection.IsNearlyZero())
        {
            // Calculate world-space rotation for the movement direction
            FRotator MovementRotation = MovementDirection.ToOrientationRotator();
            
            // Calculate the difference between actor forward and movement direction
            FRotator ActorRot = GetActorRotation();
            FRotator DeltaRot = MovementRotation - ActorRot;
            DeltaRot.Normalize();
            
            // Apply the base rotation offset plus the delta rotation to align with movement
            FRotator NewMeshRotation = MeshBaseRotation + DeltaRot;
            
            // Apply the new rotation to the mesh
            SkelMesh->SetRelativeRotation(NewMeshRotation);
        }
    }
}

void AWoWPlayerCharacter::Turn(float Value)
{
    AddControllerYawInput(Value * 0.5f);
}

void AWoWPlayerCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value * 0.5f);
}

// In Character/WoWPlayerCharacter.cpp

void AWoWPlayerCharacter::OnTargetEnemy()
{
    // Add at the start of the function
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("OnTargetEnemy Called"));
    
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Controller is null"));
        return;
    }
    
    FHitResult HitResult;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
    {
        if (GEngine && HitResult.GetActor()) 
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("Hit: %s"), *HitResult.GetActor()->GetName()));
        
        // Check if we hit any WoWCharacterBase (enemy or player)
        AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(HitResult.GetActor());
        if (TargetCharacter && TargetCharacter != this) // Don't target self
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Hit a targetable character"));
            
            // Set as target
            TargetingComponent->SetTarget(TargetCharacter);
            
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Target set"));
        }
        else if (HitResult.GetActor() == nullptr || !TargetCharacter)
        {
            // Clicked on empty space or non-character - clear target
            TargetingComponent->ClearTarget();
            
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("Target cleared"));
        }
    }
    else
    {
        // No hit result - clicked on empty space, clear target
        TargetingComponent->ClearTarget();
        
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No hit under cursor"));
    }
}
// In Character/WoWPlayerCharacter.cpp

void AWoWPlayerCharacter::OnTargetAndAttackEnemy()
{
    // Get player controller
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }
    
    // Perform trace from mouse position
    FHitResult HitResult;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
    {
        // Check if we hit any WoWCharacterBase (enemy or player)
        AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(HitResult.GetActor());
        if (TargetCharacter && TargetCharacter != this) // Don't target self
        {
            // Set as target
            TargetingComponent->SetTarget(TargetCharacter);
            
            // Only start auto-attack if it's an enemy
            AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(TargetCharacter);
            if (EnemyCharacter)
            {
                TargetingComponent->StartAutoAttack();
            }
            else
            {
                // It's a player, so don't auto-attack
                TargetingComponent->StopAutoAttack();
            }
        }
        else
        {
            // Clear target if we clicked on something else or empty space
            TargetingComponent->ClearTarget();
            TargetingComponent->StopAutoAttack();
        }
    }
    else
    {
        // Clear target if we didn't hit anything
        TargetingComponent->ClearTarget();
        TargetingComponent->StopAutoAttack();
    }
}

// Update WoWPlayerCharacter.cpp PossessedBy function:
void AWoWPlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    // Get the player state
    AWoWPlayerState* PS = GetPlayerState<AWoWPlayerState>();
    if (PS)
    {
        // Initialize ability actor info
        UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
        if (ASC)
        {
            // This is crucial - set up the ability actor info
            ASC->InitAbilityActorInfo(PS, this);
            UE_LOG(LogTemp, Warning, TEXT("**** ASC Actor Info Initialized ****"));
            
            // Initialize attributes and abilities
            InitializeAttributes();
            GiveAbilities();
            ApplyStartupEffects();
            
            // Now that the ASC is fully set up, initialize the hotbar
            if (HotbarComponent)
            {
                if (DefaultAbilityDataAsset)
                {
                    HotbarComponent->InitializeHotbar(DefaultAbilityDataAsset);
                }
            }
        }
    }
}

void AWoWPlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    UE_LOG(LogTemp, Warning, TEXT("***** OnRep_PlayerState Called *****"));
    
    // Get the player state on clients
    AWoWPlayerState* PS = GetPlayerState<AWoWPlayerState>();
    if (PS)
    {
        // Initialize ability actor info on clients
        UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
        if (ASC)
        {
            ASC->InitAbilityActorInfo(PS, this);
            UE_LOG(LogTemp, Warning, TEXT("***** Client ASC Actor Info Initialized *****"));
            
            // Force a refresh of replicated data
            ASC->ForceReplication();
            
            // Debug any tags that might already be on the ASC
            FGameplayTagContainer AllGameplayTags; // Changed variable name from Tags to AllGameplayTags
            ASC->GetOwnedGameplayTags(AllGameplayTags);
            if (AllGameplayTags.Num() > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("ASC has %d tags when PlayerState is replicated:"), AllGameplayTags.Num());
                for (const FGameplayTag& Tag : AllGameplayTags)
                {
                    UE_LOG(LogTemp, Warning, TEXT("  - %s"), *Tag.ToString());
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("ASC has no tags when PlayerState is replicated"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("OnRep_PlayerState: No AbilitySystemComponent found!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("OnRep_PlayerState: No PlayerState found!"));
    }
}

void AWoWPlayerCharacter::UpdateInputVector(float ForwardInput, float RightInput)
{
    // Store the input vector for use in UpdateMovementAndRotation
    CurrentInputVector.X = ForwardInput;
    CurrentInputVector.Y = RightInput;
    CurrentInputVector.Z = 0.0f;
}

void AWoWPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateMeshRotation();
    // Update movement and rotation each frame
}


bool AWoWPlayerCharacter::Server_ApplyAbilityEffect_Validate(int32 AbilityID, AActor* Target)
{
    // Basic validation
    return IsValid(Target) && AbilityID > 0;
}

void AWoWPlayerCharacter::Server_ApplyAbilityEffect_Implementation(int32 AbilityID, AActor* Target)
{
    if (!Target)
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Server received request to apply ability %d to target %s"), 
        AbilityID, *Target->GetName());
    
    // Get the effect application component
    UEffectApplicationComponent* EffectComp = GetEffectApplicationComponent();
    if (!EffectComp)
    {
        UE_LOG(LogTemp, Error, TEXT("No EffectApplicationComponent found on player"));
        return;
    }
    
    // Get ability data to find effect IDs
    UAbilityDataAsset* AbilityAsset = GetAbilityDataAsset();
    if (!AbilityAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("No AbilityDataAsset found"));
        return;
    }
    
    FAbilityTableRow AbilityData;
    if (AbilityAsset->GetAbilityDataByID(AbilityID, AbilityData))
    {
        // Apply the target effects from this ability
        if (AbilityData.TargetEffects.EffectIDs.Num() > 0)
        {
            // Apply effects to the target
            bool Success = EffectComp->ApplyEffectContainerToTarget(AbilityData.TargetEffects, Target);
            
            UE_LOG(LogTemp, Warning, TEXT("Server applied effects to target: %s"), 
                Success ? TEXT("Success") : TEXT("Failed"));
                
            if (Success && GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
                    FString::Printf(TEXT("Server: Applied ability %d effects to %s"), 
                    AbilityID, *Target->GetName()));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Server couldn't find ability data for ID %d"), AbilityID);
    }
}