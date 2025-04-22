#include "WoWPlayerController.h"

AWoWPlayerController::AWoWPlayerController()
{
}

void AWoWPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // Set input mode to game only
    SetInputMode(FInputModeGameOnly());
    
    // Show the mouse cursor for UI interaction
    bShowMouseCursor = true;
}