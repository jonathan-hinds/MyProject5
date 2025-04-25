#include "WoWPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "../States/WoWPlayerState.h"
#include "../Character/WoWEnemyCharacter.h"
#include "Engine/Engine.h"

AWoWPlayerCharacter::AWoWPlayerCharacter()
{
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    
    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;  // Character moves in the direction of input...
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);  // ...at this rotation rate
    GetCharacterMovement()->JumpZVelocity = 600.f;
    GetCharacterMovement()->AirControl = 0.2f;
    
    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.0f;  // The camera follows at this distance behind the character
    CameraBoom->bUsePawnControlRotation = true;  // Rotate the arm based on the controller
    
    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;  // Camera does not rotate relative to arm
    
    // Create a temporary mesh for testing
    CharacterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterMesh"));
    CharacterMesh->SetupAttachment(RootComponent);
    CharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));  // Position the mesh correctly
    
    // Create targeting component
    TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
    
    // Note: The skeletal mesh and animation blueprint references will be set in the editor
    
    // Don't rotate character to camera direction
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
}

void AWoWPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
}

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
}

void AWoWPlayerCharacter::MoveForward(float Value)
{
    if ((Controller != nullptr) && (Value != 0.0f))
    {
        // Find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        
        // Get forward vector
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        AddMovementInput(Direction, Value);
    }
}

void AWoWPlayerCharacter::MoveRight(float Value)
{
    if ((Controller != nullptr) && (Value != 0.0f))
    {
        // Find out which way is right
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        
        // Get right vector 
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(Direction, Value);
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

void AWoWPlayerCharacter::OnTargetEnemy()
{
    // Add at the start of the function
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("OnTargetEnemy Called"));
    
    // Rest of your original code...
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        // Add this debug message in the error path
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Controller is null"));
        return;
    }
    
    FHitResult HitResult;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
    {
        // Add this debug message after successful hit test
        if (GEngine && HitResult.GetActor()) 
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                FString::Printf(TEXT("Hit: %s"), *HitResult.GetActor()->GetName()));
        
        AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(HitResult.GetActor());
        if (EnemyCharacter)
        {
            // Add this after successful enemy cast
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Hit an enemy"));
            
            // Set as target
            TargetingComponent->SetTarget(EnemyCharacter);
            
            // Add this after setting target
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Target set"));
        }
        else
        {
            // Add this in the failure path
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Not an enemy"));
        }
    }
    else
    {
        // Add this in the hit test failure path
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No hit under cursor"));
    }
}
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
        // Check if we hit an enemy character
        AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(HitResult.GetActor());
        if (EnemyCharacter)
        {
            // Set as target and start auto-attack
            TargetingComponent->SetTarget(EnemyCharacter);
            TargetingComponent->StartAutoAttack();
        }
        else
        {
            // Clear target if we clicked on something else
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
            ASC->InitAbilityActorInfo(PS, this);
            
            // Initialize attributes and abilities
            InitializeAttributes();
            GiveAbilities();
            ApplyStartupEffects();
        }
    }
}

void AWoWPlayerCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    // Get the player state on clients
    AWoWPlayerState* PS = GetPlayerState<AWoWPlayerState>();
    if (PS)
    {
        // Initialize ability actor info on clients
        UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
        if (ASC)
        {
            ASC->InitAbilityActorInfo(PS, this);
            
            // Force a refresh of replicated data
            ASC->ForceReplication();
            
            // Simple debug print without requiring Cast to UWoWAttributeSet
            if (GEngine)
            {
                // Just check if we're getting the attribute set
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
                    FString::Printf(TEXT("OnRep_PlayerState: HasAttributeSet=%s"), 
                    PS->GetAttributeSet() ? TEXT("Yes") : TEXT("No")));
            }
        }
    }
}