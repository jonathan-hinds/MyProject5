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
    
    // Capture mouse input permanently, including the initial mouse down
    if (GetWorld() && GetWorld()->GetGameViewport())
    {
        GetWorld()->GetGameViewport()->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
    }
}