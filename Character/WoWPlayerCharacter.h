#pragma once

#include "CoreMinimal.h"
#include "WoWCharacterBase.h"
#include "../Components/TargetingComponent.h"
#include "../Components/HotbarComponent.h"
#include "../Components/CastingComponent.h"
#include "WoWPlayerCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;

UCLASS()
class MYPROJECT5_API AWoWPlayerCharacter : public AWoWCharacterBase
{
    GENERATED_BODY()
    
public:

    AWoWPlayerCharacter();
    virtual void Tick(float DeltaTime) override;

    virtual void Jump() override;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ApplyAbilityEffect(int32 AbilityID, AActor* Target);

    UFUNCTION(BlueprintPure, Category = "Abilities")
    UCastingComponent* GetCastingComponent() const { return CastingComponent; }

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void ZoomIn();
    void ZoomOut();

    // Set minimum and maximum camera distance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MinCameraDistance = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float MaxCameraDistance = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float CameraZoomStep = 50.0f;
    virtual float GetForwardInput() const { return ForwardInputValue; }
    virtual float GetRightInput() const { return RightInputValue; }

// Add to WoWPlayerCharacter.h in the public section
public:
    // Method to update skeletal mesh rotation based on movement direction
    void UpdateMeshRotation();
    
    // Add these properties to allow adjustment in the editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    bool bRotateMeshToMatchMovement = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    FRotator MeshBaseRotation = FRotator(0.0f, -90.0f, 0.0f); // Default 90 degrees rotated

protected:

    float ForwardInputValue;
    float RightInputValue;
    // Direction the character is facing
    FRotator TargetRotation;
    
    // Smooth rotation speed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float RotationRate = 10.0f;
    
    // Current input vector
    FVector CurrentInputVector;
    
    // Track input for movement
    void UpdateInputVector(float ForwardInput, float RightInput);
    
    // Is the character controlled by mouse movement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    bool bMouseControlsRotation = true;
    
    // Toggle mouse rotation
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void ToggleMouseRotation() { bMouseControlsRotation = !bMouseControlsRotation; }








    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
UCastingComponent* CastingComponent;

    // Camera boom positioning the camera behind the character
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;

    // Default ability data asset for hotbar
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    UAbilityDataAsset* DefaultAbilityDataAsset;

    // Follow camera
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* FollowCamera;

    // Character mesh (temporary for testing)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
    UStaticMeshComponent* CharacterMesh;

    // Targeting component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
    UTargetingComponent* TargetingComponent;

    // Hotbar component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    UHotbarComponent* HotbarComponent;
    // Movement functions
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    // Targeting input handlers
    void OnTargetEnemy();
    void OnTargetAndAttackEnemy();

        // Hotbar input handlers
    void OnHotbarSlot1();
    void OnHotbarSlot2();
    void OnHotbarSlot3();
    void OnHotbarSlot4();
    void OnHotbarSlot5();
    void OnHotbarSlot6();
    void OnHotbarSlot7();
    void OnHotbarSlot8();
    void OnHotbarSlot9();
    void OnHotbarSlot10();
    void OnHotbarSlot11();
    void OnHotbarSlot12();

    // Generic hotbar slot handler
    void HandleHotbarSlot(int32 SlotIndex);

public:
    // Override from base class to handle ability system related setup for player
    virtual void PossessedBy(AController* NewController) override;
    
    // Called when this Pawn is replicated
    virtual void OnRep_PlayerState() override;

    // Get hotbar component
    UFUNCTION(BlueprintPure, Category = "Abilities")
    UHotbarComponent* GetHotbarComponent() const { return HotbarComponent; }
    
};