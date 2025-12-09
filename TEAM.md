# 👥 悟空项目 C++ 成员详细职责清单

以下分工旨在最大化 C++ 的使用，确保核心逻辑的性能和架构的优雅性。

### 1. 成员 A：主角核心动作系统 (Character Core)

**核心目标：** 在 C++ 中实现角色的所有核心状态、连击逻辑和属性管理，作为其他所有模块的基石。

| 类别 | C++ 具体任务 | 蓝图交互与依赖 | 关键 C++ 类/组件 |
| :--- | :--- | :--- | :--- |
| **状态机** | **Enums 定义：** 在 C++ 中定义 `ECharacterState` 枚举（`Idle`, `Run`, `Attack`, `Dodge`, `HitStun`, 等），作为角色的唯一权威状态。 | 蓝图只负责根据 C++ 的状态枚举播放相应的动画蒙太奇。 | `AWukongCharacter.h` |
| **连击系统** | **连击判定：** 实现 `LightAttack()` C++ 函数，使用定时器和布尔值管理连击窗口和输入缓冲，判断是否进入下一段攻击。 | 蓝图动画通知（Anim Notifies）需回调 C++ 函数（如 `AttackWindowEnd()`）以结束连击判定。 | `AWukongCharacter.cpp` |
| **核心属性** | **属性定义：** 在 C++ 中定义 `CurrentHealth`, `CurrentStamina` 等变量，并实现 C++ `ApplyDamage()` 或 `RecoverStamina()` 函数。 | 使用 `BlueprintReadOnly` 暴露给蓝图，供 UI 模块（成员 D）安全读取。 | `AWukongCharacter.h` |
| **碰撞/伤害** | **碰撞箱：** 在 C++ 中创建并配置攻击碰撞组件（`USphereComponent`），用于在连击特定帧开启和关闭。 | 蓝图用于设置碰撞组件的相对位置和大小，C++ 负责激活和失活。 | 继承自 `ACharacter` |

---

### 2. 成员 B：输入与战斗技能 (Input & Combat)

**核心目标：** 将所有进阶输入（技能、闪避）绑定到 C++，并设计技能的基类结构，实现技能的资源消耗和冷却管理。

| 类别 | C++ 具体任务 | 蓝图交互与依赖 | 关键 C++ 类/组件 |
| :--- | :--- | :--- | :--- |
| **技能基类** | **抽象类设计：** 创建 `UWukongAbilityBase` 基类，包含纯虚函数 `ExecuteAbility()`。在基类中实现 C++ 函数 `CanActivate()`，用于检查技能是否在冷却中、气力是否足够。 | **数据配置：** 蓝图技能（继承自 C++ 基类）用于配置冷却时间、消耗的气力值等属性。 | `UWukongAbilityBase` |
| **输入绑定** | **进阶输入：** 在 `SetupPlayerInputComponent` 中绑定所有技能、闪避 (`DodgeAction`) 和防御 (`BlockAction`) 的输入到 C++ 函数。 | 蓝图用于创建 `Input Action` 资产，并将其赋值给主角蓝图上的 C++ 变量。 | `AWukongCharacter.cpp` |
| **资源消耗** | **消耗实现：** 在 C++ 技能函数中，调用 **成员 A** 的 C++ 属性函数（如 `ConsumeStamina()`），进行气力消耗的计算和处理。 | 蓝图不需要处理消耗逻辑，只负责调用 C++ 技能函数。 | `UWukongAbilityBase` |

---

### 3. 成员 C：敌人 AI 与互动 (AI & Interaction)

**核心目标：** 在 C++ 中实现敌人的核心角色和 AI 逻辑，包括寻路、行为决策和战斗响应。

| 类别 | C++ 具体任务 | 蓝图交互与依赖 | 关键 C++ 类/组件 |
| :--- | :--- | :--- | :--- |
| **敌人基类** | **角色创建：** 创建 `AEnemyCharacter` C++ 类。在其中定义敌人的 AI 状态枚举（`EEnemyState { Idle, Patrol, Chase, Attack, Flee }`）。 | 蓝图敌人（继承自 C++ 基类）用于配置敌人的网格体、动画和基础属性。 | `AEnemyCharacter` |
| **AI 控制器** | **行为逻辑：** 创建 `AEnemyAIController` C++ 类。使用 C++ 实现寻路（`MoveToLocation`）和决策逻辑，驱动行为树（Behavior Tree）的关键任务节点。 | 行为树（Behavior Tree）资产用于可视化的流程控制，但其核心任务节点由 C++ 实现。 | `AEnemyAIController` |
| **伤害处理** | **伤害接收：** 重写 C++ 的 `TakeDamage()` 函数，在 C++ 中计算伤害值、判断是否进入硬直状态（`HitStun`），并更新敌人的生命值。 | 蓝图用于播放受伤和死亡动画，但逻辑判断必须在 C++ 中完成。 | `AEnemyCharacter.cpp` |
| **目标感知** | **感知组件：** 在 C++ 中添加和配置 `UPawnSensingComponent`，用于实现敌人对主角的视觉或听觉感知。 | 蓝图用于调整感知范围和角度的数值。 | 继承自 `ACharacter` |

---

### 4. 成员 D：游戏系统与环境 (Systems & World)

**核心目标：** 搭建游戏规则的 C++ 架构，并作为数据接口将 C++ 核心数据安全地连接到 UI 和其他系统。

| 类别 | C++ 具体任务 | 蓝图交互与依赖 | 关键 C++ 类/组件 |
| :--- | :--- | :--- | :--- |
| **游戏模式** | **规则定义：** 创建 `AWukongGameMode`，在 C++ `BeginPlay()` 中定义游戏的初始设置，如生成默认角色（Pawn）的逻辑。**流程控制：** 实现 C++ `GameOver()` 和 `GameWin()` 函数，控制游戏状态。 | 蓝图只用于继承 `AWukongGameMode`，并设置默认使用的 Pawn 类 和 Controller 类。 | `AWukongGameMode` |
| **UI 数据源** | **数据桥梁：** 创建 `UWukongHUDWidget` C++ 基类。在其中实现 C++ 函数 `GetWukongHealth()` 和 `GetWukongStamina()`，用于安全地获取 **成员 A** 的主角数据。 | UMG 蓝图（UI 界面）必须继承自 `UWukongHUDWidget`，通过调用 C++ 函数来更新血条、气力条的显示。 | `UWukongHUDWidget` |
| **世界状态** | **全局数据：** 创建 `AWukongGameState`，在其中定义和管理所有玩家都应知的全局数据（例如，Boss 是否被击败、关卡中的剩余敌人数量）。 | 蓝图可安全地从 `GameState` 中获取全局信息，用于事件触发。 | `AWukongGameState` |
| **存读档系统** | **保存基类：** 创建 `USaveGame` 的 C++ 派生类，在其中定义需要保存的 C++ 变量（如主角属性、关卡进度）。 | 蓝图调用 C++ `SaveGame` 和 `LoadGame` 函数，但 C++ 定义了要保存的数据结构。 | `UWukongSaveGame` |