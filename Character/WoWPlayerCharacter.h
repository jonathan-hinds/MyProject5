#pragma once

#include "CoreMinimal.h"
#include "WoWCharacterBase.h"
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

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Camera boom positioning the camera behind the character
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;

    // Follow camera
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* FollowCamera;

    // Character mesh (temporary for testing)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
    UStaticMeshComponent* CharacterMesh;

    // Movement functions
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

public:
    // Override from base class to handle ability system related setup for player
    virtual void PossessedBy(AController* NewController) override;
    
    // Called when this Pawn is replicated
    virtual void OnRep_PlayerState() override;
};