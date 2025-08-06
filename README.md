# GameEventSystem

A high-performance, type-safe UE5 event system plugin supporting both Blueprint and C++. It provides a flexible event listening and dispatch mechanism, suitable for inter-module communication, UI updates, and decoupling game logic in game development.

![UE Version](https://img.shields.io/badge/UE-5.0+-blue) ![Thread Safe](https://img.shields.io/badge/Thread%20Safe-✓-green)

## Features

- 🚀 **High Performance**: Optimized event dispatching, supports large-scale event processing
- 🧵 **Thread Safe**: Safe operations in multi-threaded environments
- 📘 **Blueprint Friendly**: Full Blueprint node support
- 📌 **Event Pinning**: Supports event state persistence, allowing late listeners to immediately receive pinned events
- 🏷️ **No Type Limitations**: Supports any data type
- 🔧 **Parameter Type Deduction**: Automatically deduces event parameter types, supports any number of parameters (C++ only)

## Installation Guide

### 1. Plugin Installation

1. Place the `GameEventSystem` plugin folder in your project's `Plugins` directory
2. Regenerate project files (right-click .uproject -> Generate Visual Studio project files)
3. Build your project

### 2. Module Dependency Configuration

Add dependencies in your project's `Build.cs` file:

```csharp
PublicDependencyModuleNames.AddRange(new string[] 
{
    "GameEventSystem"  // Runtime module
});

// For Blueprint editor node usage
if (Target.bBuildEditor)
{
    PrivateDependencyModuleNames.AddRange(new string[] 
    {
        "GameEventNode"  // Blueprint editor module
    });
}
```

### 3. Project Settings

Enable the plugin in your project settings:

- Open `Edit -> Plugins`
- Search "GameEventSystem"
- Check to enable the plugin

## Core Concepts

### Event Identifier (FEventId)

Supports two types of event identifiers:

```cpp
// String identifier (recommended)
FEventId StringEvent(TEXT("Player.LevelUp"));
FEventId NestedEvent(TEXT("UI.MainMenu.ButtonClicked"));

// GameplayTag identifier
FGameplayTag PlayerTag = FGameplayTag::RequestGameplayTag(FName("Player.Health.Changed"));
FEventId TagEvent(PlayerTag);
```

### Event Manager (FGameEventManager)

Global singleton event manager, responsible for event registration, dispatch, and management:

```cpp
// Get event manager instance
TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
```

## Usage

### C++ Usage Examples

#### 1. Basic Event Listener

```cpp
// Declare event handler functions in header file
UCLASS()
class YOURGAME_API APlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // Event handler function
    UFUNCTION()
    void OnPlayerLevelUp(int32 NewLevel, FString PlayerName);
    
    UFUNCTION()
    void OnGamePaused();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TSharedPtr<FGameEventManager> EventManager;
};
```

```cpp
// Implementation in source file
void APlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // Get event manager
    EventManager = FGameEventManager::Get();
    
    // Add function listener
    EventManager->AddListenerFunction(
        FEventId(TEXT("Player.LevelUp")), 
        this, 
        TEXT("OnPlayerLevelUp")
    );
    
    EventManager->AddListenerFunction(
        FEventId(TEXT("Game.Paused")), 
        this, 
        TEXT("OnGamePaused")
    );
}

void APlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Remove all listeners
    if (EventManager.IsValid())
    {
        EventManager->RemoveAllListenersForReceiver(this);
    }
    
    Super::EndPlay(EndPlayReason);
}

void APlayerController::OnPlayerLevelUp(int32 NewLevel, FString PlayerName)
{
    UE_LOG(LogTemp, Log, TEXT("Player %s reached level %d!"), *PlayerName, NewLevel);
}

void APlayerController::OnGamePaused()
{
    UE_LOG(LogTemp, Log, TEXT("Game has been paused"));
}
```

#### 2. Lambda Listener

```cpp
void AGameMode::SetupEventListeners()
{
    auto EventManager = FGameEventManager::Get();
    
    // Simple lambda listener
    FString ListenerId1 = EventManager->AddLambdaListener(
        FEventId(TEXT("Player.Death")), 
        this,
        []() {
            UE_LOG(LogTemp, Warning, TEXT("A player has died!"));
        }
    );
    
    // Lambda listener with parameters
    FString ListenerId2 = EventManager->AddLambdaListener(
        FEventId(TEXT("Player.Death")), 
        this,
        [this](FVector SpawnLocation, int32 EnemyType) {
            UE_LOG(LogTemp, Log, TEXT("Enemy type %d spawned at %s"), 
                   EnemyType, *SpawnLocation.ToString());
            // Can access 'this' pointer for more complex operations
            this->OnEnemySpawned(SpawnLocation, EnemyType);
        }
    );
    
    // Save listener IDs for later removal
    LambdaListenerIds.Add(ListenerId1);
    LambdaListenerIds.Add(ListenerId2);
}

void AGameMode::CleanupEventListeners()
{
    auto EventManager = FGameEventManager::Get();
    
    // Remove lambda listeners
    fEventManager->RemoveLambdaListener(FEventId(TEXT("Player.Death")), ListenerId1);
    fEventManager->RemoveLambdaListener(FEventId(TEXT("Player.Death")), ListenerId2);
}
```

#### 3. Sending Events

```cpp
void APlayerCharacter::LevelUp()
{
    // Update level
    CurrentLevel++;
    
    auto EventManager = FGameEventManager::Get();
    
    // Send event with parameters
    EventManager->SendEvent(
        FEventId(TEXT("Player.LevelUp")), 
        this,           // World context
        false,          // Is pinned event
        CurrentLevel,   // New level
        GetName()       // Player name
    );
}

void AGameManager::PauseGame()
{
    auto EventManager = FGameEventManager::Get();
    
    // Send simple event (no parameters)
    EventManager->SendEvent(FEventId(TEXT("Game.Paused")), this, true);  // Pinned event
}

void AWeaponSystem::FireWeapon(FVector FireLocation, float Damage, int32 AmmoRemaining)
{
    auto EventManager = FGameEventManager::Get();
    
    // Send event with complex parameters
    EventManager->SendEvent(
        FEventId(TEXT("Weapon.Fired")), 
        this, 
        false,
        FireLocation,    // FVector parameter
        Damage,          // float parameter
        AmmoRemaining    // int32 parameter
    );
}
```

#### 4. Event Pinning

```cpp
void AGameState::InitializeGameState()
{
    auto EventManager = FGameEventManager::Get();
    
    // Send pinned event - listeners registered later will immediately receive this event
    EventManager->SendEvent(
        FEventId(TEXT("Game.StateInitialized")), 
        this, 
        true,  // Pinned event
        FString(TEXT("GameStarted")),
        GetWorld()->GetTimeSeconds()
    );
}

// Listeners registered later will also instantly receive pinned events
void AUI_MainHUD::BeginPlay()
{
    Super::BeginPlay();
    
    auto EventManager = FGameEventManager::Get();
    
    // Even if game state is already initialized, this listener will instantly receive the event
    EventManager->AddLambdaListener(
        FEventId(TEXT("Game.StateInitialized")), 
        this,
        [this](FString GameState, float InitTime) {
            UpdateGameStateUI(GameState, InitTime);
        }
    );
}
```

### Blueprint Usage Example

#### 1. Blueprint Node Overview

The plugin provides the following Blueprint nodes:

- **Add Listener**: Add event listener
- **Send Event**: Send event
- **Remove Listener**: Remove listeners for all event keys
- **Remove All Listeners For Receiver**: Remove all listeners for a receiver
- **Has Event**: Check if event exists
- **Get Event Listener Count**: Get the number of listeners for an event
- **Unpin Event**: Unpin a pinned event

#### 2. Blueprint Event Listening

1. Add **Add Listener** node in Blueprint
2. Set **Event ID Type** (String or Tag)
3. Set **Event Name** or **Event Tag** based on type
4. Connect **Event Function** to a custom event
5. Specify **Receiver** (usually Self)
6. For **Delegate**, right-click the Delegate Pin to add parameters (Currently only supports one parameter in Blueprint!)

#### 3. Blueprint Event Sending

1. Add **Send Event** node
2. Configure event identifier
3. Set **Pinned** property (whether the event is pinned)
4. Connect **Param Data** input if parameters are needed
5. Connect **Self** pin as world context

#### 4. Blueprint Parameter Types

Blueprint nodes support the following parameter types:

- All basic types, structs, objects, and container types

## Best Practices

### 1. Naming Convention

Use hierarchical event naming:

```cpp
// ✅ Recommended: hierarchical naming
FEventId(TEXT("Player.Combat.Damage.Taken"))
FEventId(TEXT("UI.MainMenu.Button.Clicked"))
FEventId(TEXT("System.Save.Progress.Updated"))

// ❌ Not recommended: flat naming
FEventId(TEXT("PlayerDamage"))
FEventId(TEXT("ButtonClick"))
FEventId(TEXT("SaveUpdate"))
```

### 2. Memory Management

```cpp
class AMyActor : public AActor
{
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
    {
        // Important: remove all listeners when object is destroyed
        if (auto EventManager = FGameEventManager::Get())
        {
            EventManager->RemoveAllListenersForReceiver(this);
        }
        
        Super::EndPlay(EndPlayReason);
    }
};
```

### 3. Thread Safety Notes

```cpp
// ✅ Recommended: operate on game thread
void AMyActor::SafeEventOperation()
{
    if (IsInGameThread())
    {
        auto EventManager = FGameEventManager::Get();
        EventManager->SendEvent(FEventId(TEXT("Safe.Event")), this, false);
    }
    else
    {
        // Cross-thread calls should use AsyncTask
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            auto EventManager = FGameEventManager::Get();
            EventManager->SendEvent(FEventId(TEXT("Safe.Event")), this, false);
        });
    }
}
```

### 4. Event Parameter Optimization

```cpp
// ✅ Recommended: pass by reference to avoid copies
void SendLargeDataEvent(const FLargeDataStruct& Data)
{
    auto EventManager = FGameEventManager::Get();
    EventManager->SendEvent(FEventId(TEXT("Data.Large")), this, false, Data);
}

// ✅ Recommended: use smart pointer for passing UObject
void SendObjectEvent(TWeakObjectPtr<UMyObject> Object)
{
    auto EventManager = FGameEventManager::Get();
    EventManager->SendEvent(FEventId(TEXT("Object.Updated")), this, false, Object);
}
```

## Common Use Cases

### 1. Player State System

```cpp
// Health system
void AHealthComponent::TakeDamage(float Damage)
{
    CurrentHealth -= Damage;
    
    auto EventManager = FGameEventManager::Get();
    
    // Send health change event
    EventManager->SendEvent(
        FEventId(TEXT("Player.Health.Changed")), 
        GetOwner(), 
        false,
        CurrentHealth, MaxHealth, Damage
    );
    
    // If dead, send death event
    if (CurrentHealth <= 0.0f)
    {
        EventManager->SendEvent(
            FEventId(TEXT("Player.Death")), 
            GetOwner(), 
            true
        );
    }
}
```

### 2. UI Response System

```cpp
// UI controller listens to game events
void AUIController::BeginPlay()
{
    Super::BeginPlay();
    
    auto EventManager = FGameEventManager::Get();
    
    // Listen to health change to update UI
    EventManager->AddLambdaListener(
        FEventId(TEXT("Player.Health.Changed")), 
        this,
        [this](float Current, float Max, float Damage) {
            UpdateHealthBar(Current / Max);
            ShowDamageNumber(Damage);
        }
    );
    
    // Listen to currency change
    EventManager->AddLambdaListener(
        FEventId(TEXT("Player.Currency.Changed")), 
        this,
        [this](int32 NewAmount, int32 Change) {
            UpdateCurrencyDisplay(NewAmount);
            if (Change > 0)
            {
                ShowCurrencyGainEffect(Change);
            }
        }
    );
}
```

## Troubleshooting

### Common Issues

1. **Listener not triggered**
    - Check if event name matches exactly
    - Ensure receiver object is still valid
    - Verify function signatures match

2. **Compile errors**
    - Ensure module dependencies are added
    - Check header file includes

3. **Blueprint nodes not showing**
    - Ensure GameEventNode module is enabled
    - Check if plugin is installed correctly

## Technical Specs

- **Minimum UE Version**: 5.0+
- **Thread Safe**: Yes
- **Blueprint Support**: Full
- **Memory Usage**: Lightweight design, minimal memory footprint

## Technical Support

If you encounter issues during use:

1. Check the Troubleshooting section of this document
2. Review code samples and best practices
3. Confirm plugin version and UE version compatibility

---

**GameEventSystem** - Make event communication in your UE5 project simpler and more efficient!
