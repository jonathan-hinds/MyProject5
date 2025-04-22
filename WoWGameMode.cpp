#include "WoWGameMode.h"
#include "Character/WoWPlayerCharacter.h"
#include "Controllers/WoWPlayerController.h"
#include "States/WoWPlayerState.h"

AWoWGameMode::AWoWGameMode()
{
    // Set default classes
    DefaultPawnClass = AWoWPlayerCharacter::StaticClass();
    PlayerControllerClass = AWoWPlayerController::StaticClass();
    PlayerStateClass = AWoWPlayerState::StaticClass();
}