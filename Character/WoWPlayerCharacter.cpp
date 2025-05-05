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
    HotbarComponent = CreateDefaultSubobject<UHotbarComponent>(TEXT("HotbarComponent"));
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

void AWoWPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Simple debug message to confirm Tick is running
    static float SimpleTimer = 0.0f;
    SimpleTimer += DeltaTime;
    
    if (SimpleTimer >= 5.0f)
    {
        SimpleTimer = 0.0f;
        
        // Get ability system component
        UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
        if (ASC)
        {
            // Show on-screen debug message
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
                    TEXT("Checking cooldowns in Tick"));
            }
            
            // Get all tags
            FGameplayTagContainer AllTags;
            ASC->GetOwnedGameplayTags(AllTags);
            
            if (AllTags.Num() > 0)
            {
                // Show tag count
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, 
                    FString::Printf(TEXT("ASC has %d tags"), AllTags.Num()));
                
                // Log each tag
                UE_LOG(LogTemp, Warning, TEXT("==== Current ASC Tags (%d) ===="), AllTags.Num());
                
                // Show each tag
                int32 Index = 0;
                for (const FGameplayTag& Tag : AllTags)
                {
                    UE_LOG(LogTemp, Warning, TEXT("  - %s"), *Tag.ToString());
                    
                    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
                        FString::Printf(TEXT("Tag %d: %s"), Index++, *Tag.ToString()));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("ASC has no tags in Tick"));
                
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                    TEXT("ASC has no tags"));
            }
            
            // Check active effects
            TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(FGameplayEffectQuery());
            if (ActiveEffects.Num() > 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("==== Active Effects (%d) ===="), ActiveEffects.Num());
                
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
                    FString::Printf(TEXT("ASC has %d active effects"), ActiveEffects.Num()));
                
                // Show details for each effect
                for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
                {
                    const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(Handle);
                    if (Effect && Effect->Spec.Def)
                    {
                        float TimeLeft = Effect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (%.1f seconds left)"), 
                            *Effect->Spec.Def->GetName(), TimeLeft);
                    }
                }
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
                    TEXT("ASC has no active effects"));
            }
        }
    }
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