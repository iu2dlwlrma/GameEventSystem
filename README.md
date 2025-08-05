# GameEventSystem

一个高性能、类型安全的UE5事件系统插件，支持蓝图和C++，提供灵活的事件监听和分发机制。适用于游戏开发中的模块间通信、UI更新、游戏逻辑解耦等场景。

![UE版本](https://img.shields.io/badge/UE-5.0+-blue) ![平台](https://img.shields.io/badge/平台-Windows%20%7C%20Mac%20%7C%20Linux-green) ![线程安全](https://img.shields.io/badge/线程安全-✓-green)

## 功能特性

- 🚀 **高性能**: 优化的事件分发机制，支持大规模事件处理
- 🔒 **类型安全**: 编译时类型检查，避免运行时错误
- 🧵 **线程安全**: 支持多线程环境下的安全操作
- 📘 **蓝图友好**: 完整的蓝图节点支持，所见即所得
- 🎯 **Lambda支持**: 支持C++ Lambda表达式作为事件监听器
- 🏷️ **双重标识**: 支持字符串和GameplayTag两种事件标识符
- 📌 **事件固定**: 支持事件状态持久化，后注册的监听器可立即收到固定事件
- 🔧 **参数类型推导**: 自动推导事件参数类型，支持任意数量参数

## 安装指南

### 1. 插件安装

1. 将 `GameEventSystem` 插件文件夹放置到项目的 `Plugins` 目录下
2. 重新生成项目文件（右键.uproject文件 -> Generate Visual Studio project files）
3. 编译项目

### 2. 模块依赖配置

在项目的 `Build.cs` 文件中添加依赖：

```csharp
PublicDependencyModuleNames.AddRange(new string[] 
{
    "GameEventSystem"  // 运行时模块
});

// 如果需要在编辑器中使用蓝图节点
if (Target.bBuildEditor)
{
    PrivateDependencyModuleNames.AddRange(new string[] 
    {
        "GameEventNode"  // 蓝图编辑器模块
    });
}
```

### 3. 项目设置

在项目设置中启用插件：

- 打开 `Edit -> Plugins`
- 搜索 "GameEventSystem"
- 勾选启用插件

## 核心概念

### 事件标识符 (FEventId)

支持两种类型的事件标识符：

```cpp
// 字符串标识符（推荐）
FEventId StringEvent(TEXT("Player.LevelUp"));
FEventId NestedEvent(TEXT("UI.MainMenu.ButtonClicked"));

// GameplayTag标识符
FGameplayTag PlayerTag = FGameplayTag::RequestGameplayTag(FName("Player.Health.Changed"));
FEventId TagEvent(PlayerTag);
```

### 事件管理器 (FGameEventManager)

全局单例事件管理器，负责事件的注册、分发和管理：

```cpp
// 获取事件管理器实例
TSharedPtr<FGameEventManager> EventManager = FGameEventManager::Get();
```

## 使用方法

### C++ 使用示例

#### 1. 基础事件监听

```cpp
// 在头文件中声明事件处理函数
UCLASS()
class YOURGAME_API APlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // 事件处理函数
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
// 在源文件中实现
void APlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // 获取事件管理器
    EventManager = FGameEventManager::Get();
    
    // 添加函数监听器
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
    // 移除所有监听器
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

#### 2. Lambda 监听器

```cpp
void AGameMode::SetupEventListeners()
{
    auto EventManager = FGameEventManager::Get();
    
    // 简单Lambda监听器
    FString ListenerId1 = EventManager->AddLambdaListener(
        FEventId(TEXT("Player.Death")), 
        this,
        []() {
            UE_LOG(LogTemp, Warning, TEXT("A player has died!"));
        }
    );
    
    // 带参数的Lambda监听器
    FString ListenerId2 = EventManager->AddLambdaListener(
        FEventId(TEXT("Enemy.Spawned")), 
        this,
        [this](FVector SpawnLocation, int32 EnemyType) {
            UE_LOG(LogTemp, Log, TEXT("Enemy type %d spawned at %s"), 
                   EnemyType, *SpawnLocation.ToString());
            // 可以访问this指针进行更复杂的操作
            this->OnEnemySpawned(SpawnLocation, EnemyType);
        }
    );
    
    // 保存监听器ID以便后续移除
    LambdaListenerIds.Add(ListenerId1);
    LambdaListenerIds.Add(ListenerId2);
}

void AGameMode::CleanupEventListeners()
{
    auto EventManager = FGameEventManager::Get();
    
    // 移除Lambda监听器
    for (const FString& ListenerId : LambdaListenerIds)
    {
        EventManager->RemoveLambdaListener(FEventId(TEXT("Player.Death")), ListenerId);
    }
}
```

#### 3. 发送事件

```cpp
void APlayerCharacter::LevelUp()
{
    // 更新等级
    CurrentLevel++;
    
    auto EventManager = FGameEventManager::Get();
    
    // 发送带参数的事件
    EventManager->SendEvent(
        FEventId(TEXT("Player.LevelUp")), 
        this,           // 世界上下文
        false,          // 是否固定事件
        CurrentLevel,   // 新等级
        GetName()       // 玩家名称
    );
}

void AGameManager::PauseGame()
{
    auto EventManager = FGameEventManager::Get();
    
    // 发送简单事件（无参数）
    EventManager->SendEvent(FEventId(TEXT("Game.Paused")), this, true);  // 固定事件
}

void AWeaponSystem::FireWeapon(FVector FireLocation, float Damage, int32 AmmoRemaining)
{
    auto EventManager = FGameEventManager::Get();
    
    // 发送复杂参数事件
    EventManager->SendEvent(
        FEventId(TEXT("Weapon.Fired")), 
        this, 
        false,
        FireLocation,    // FVector参数
        Damage,          // float参数
        AmmoRemaining    // int32参数
    );
}
```

#### 4. 事件固定功能

```cpp
void AGameState::InitializeGameState()
{
    auto EventManager = FGameEventManager::Get();
    
    // 发送固定事件 - 后续注册的监听器也会立即收到此事件
    EventManager->SendEvent(
        FEventId(TEXT("Game.StateInitialized")), 
        this, 
        true,  // 固定事件
        FString(TEXT("GameStarted")),
        GetWorld()->GetTimeSeconds()
    );
}

// 稍后注册的监听器也会立即收到固定事件
void AUI_MainHUD::BeginPlay()
{
    Super::BeginPlay();
    
    auto EventManager = FGameEventManager::Get();
    
    // 即使游戏状态已经初始化，这个监听器也会立即收到事件
    EventManager->AddLambdaListener(
        FEventId(TEXT("Game.StateInitialized")), 
        this,
        [this](FString GameState, float InitTime) {
            UpdateGameStateUI(GameState, InitTime);
        }
    );
}
```

### 蓝图使用示例

#### 1. 蓝图节点总览

插件提供了以下蓝图节点：

- **Add Listener**: 添加事件监听器
- **Send Event**: 发送事件
- **Remove Listener**: 移除所有事件Key的监听器
- **Remove All Listeners For Receiver**: 移除接收者的所有监听器
- **Has Event**: 检查事件是否存在
- **Get Event Listener Count**: 获取事件监听器数量
- **Unpin Event**: 取消固定事件

#### 2. 蓝图事件监听

1. 在蓝图中添加 **Add Listener** 节点
2. 设置 **Event ID Type** (String 或 Tag)
3. 根据类型设置 **Event Name** 或 **Event Tag**
4. 连接 **Event Function** 到自定义事件
5. 指定 **Receiver** (通常是Self)
6. 如果是 **Delegate** 右键Delegate Pin脚可以创建参数（当前蓝图内仅支持一个参数！）

#### 3. 蓝图事件发送

1. 添加 **Send Event** 节点
2. 配置事件标识符
3. 设置 **Pinned** 属性（是否固定事件）
4. 连接 **Param Data** 输入（如果事件需要参数）
5. 连接 **Self** 引脚作为世界上下文

#### 4. 蓝图参数类型

蓝图节点支持以下参数类型：

- 基础类型：int32, float, bool, FString, FName
- UE类型：FVector, FRotator, FTransform, FColor
- 对象引用：UObject派生类
- 结构体：自定义USTRUCT
- 容器类型：TArray, TSet, TMap

## 最佳实践

### 1. 命名规范

建议使用层次化的事件命名：

```cpp
// ✅ 推荐：层次化命名
FEventId(TEXT("Player.Combat.Damage.Taken"))
FEventId(TEXT("UI.MainMenu.Button.Clicked"))
FEventId(TEXT("System.Save.Progress.Updated"))

// ❌ 不推荐：扁平化命名
FEventId(TEXT("PlayerDamage"))
FEventId(TEXT("ButtonClick"))
FEventId(TEXT("SaveUpdate"))
```

### 2. 内存管理

```cpp
class AMyActor : public AActor
{
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
    {
        // 重要：在对象销毁时移除所有监听器
        if (auto EventManager = FGameEventManager::Get())
        {
            EventManager->RemoveAllListenersForReceiver(this);
        }
        
        Super::EndPlay(EndPlayReason);
    }
};
```

### 3. 线程安全注意事项

```cpp
// ✅ 推荐：在游戏线程中操作
void AMyActor::SafeEventOperation()
{
    if (IsInGameThread())
    {
        auto EventManager = FGameEventManager::Get();
        EventManager->SendEvent(FEventId(TEXT("Safe.Event")), this, false);
    }
    else
    {
        // 跨线程调用需要使用AsyncTask
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            auto EventManager = FGameEventManager::Get();
            EventManager->SendEvent(FEventId(TEXT("Safe.Event")), this, false);
        });
    }
}
```

### 4. 事件参数优化

```cpp
// ✅ 推荐：传递引用避免拷贝
void SendLargeDataEvent(const FLargeDataStruct& Data)
{
    auto EventManager = FGameEventManager::Get();
    EventManager->SendEvent(FEventId(TEXT("Data.Large")), this, false, Data);
}

// ✅ 推荐：使用智能指针传递UObject
void SendObjectEvent(TWeakObjectPtr<UMyObject> Object)
{
    auto EventManager = FGameEventManager::Get();
    EventManager->SendEvent(FEventId(TEXT("Object.Updated")), this, false, Object);
}
```

## 常见应用场景

### 1. 玩家状态系统

```cpp
// 血量系统
void AHealthComponent::TakeDamage(float Damage)
{
    CurrentHealth -= Damage;
    
    auto EventManager = FGameEventManager::Get();
    
    // 发送血量变化事件
    EventManager->SendEvent(
        FEventId(TEXT("Player.Health.Changed")), 
        GetOwner(), 
        false,
        CurrentHealth, MaxHealth, Damage
    );
    
    // 如果死亡，发送死亡事件
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

### 2. UI响应系统

```cpp
// UI控制器监听游戏事件
void AUIController::BeginPlay()
{
    Super::BeginPlay();
    
    auto EventManager = FGameEventManager::Get();
    
    // 监听血量变化更新UI
    EventManager->AddLambdaListener(
        FEventId(TEXT("Player.Health.Changed")), 
        this,
        [this](float Current, float Max, float Damage) {
            UpdateHealthBar(Current / Max);
            ShowDamageNumber(Damage);
        }
    );
    
    // 监听金币变化
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

### 3. 成就系统

```cpp
void AAchievementManager::SetupAchievementListeners()
{
    auto EventManager = FGameEventManager::Get();
    
    // 击杀成就
    EventManager->AddLambdaListener(
        FEventId(TEXT("Enemy.Killed")), 
        this,
        [this](FString EnemyType) {
            IncrementKillCount(EnemyType);
            CheckKillAchievements();
        }
    );
    
    // 收集成就
    EventManager->AddLambdaListener(
        FEventId(TEXT("Item.Collected")), 
        this,
        [this](FString ItemType, int32 Quantity) {
            UpdateCollectionProgress(ItemType, Quantity);
            CheckCollectionAchievements();
        }
    );
}
```

### 4. 音效和特效系统

```cpp
void AAudioManager::SetupAudioEvents()
{
    auto EventManager = FGameEventManager::Get();
    
    // 武器音效
    EventManager->AddLambdaListener(
        FEventId(TEXT("Weapon.Fired")), 
        this,
        [this](FVector Location, FString WeaponType) {
            PlaySoundAtLocation(GetFireSound(WeaponType), Location);
        }
    );
    
    // 环境音效
    EventManager->AddLambdaListener(
        FEventId(TEXT("Player.Footstep")), 
        this,
        [this](FVector Location, FString SurfaceType) {
            PlayFootstepSound(Location, SurfaceType);
        }
    );
}

void AEffectManager::SetupEffectEvents()
{
    auto EventManager = FGameEventManager::Get();
    
    // 爆炸特效
    EventManager->AddLambdaListener(
        FEventId(TEXT("Explosion.Triggered")), 
        this,
        [this](FVector Location, float Radius, FString ExplosionType) {
            SpawnExplosionEffect(Location, Radius, ExplosionType);
        }
    );
}
```

## 故障排除

### 常见问题

1. **监听器没有触发**
    - 检查事件名称是否完全一致
    - 确认接收者对象仍然有效
    - 验证函数签名是否匹配

2. **内存泄漏**
    - 确保在对象销毁时调用 `RemoveAllListenersForReceiver`
    - 检查Lambda监听器是否正确移除

3. **编译错误**
    - 确认已添加模块依赖
    - 检查头文件包含是否正确

4. **蓝图节点不显示**
    - 确认GameEventNode模块已启用
    - 检查插件是否正确安装

## 技术规格

- **最低UE版本**: 5.0+
- **支持平台**: Windows, Mac, Linux
- **线程安全**: 是
- **蓝图支持**: 完整支持
- **性能**: 优化的哈希表和内存池
- **内存占用**: 轻量级设计，最小内存开销

## 许可证

Copyright LetsGo. All Rights Reserved.

## 技术支持

如果您在使用过程中遇到问题，请：

1. 查看本文档的故障排除部分
2. 检查代码示例和最佳实践
3. 确认插件版本和UE版本兼容性****

---

**GameEventSystem** - 让您的UE5项目事件通信更简单、更高效！
