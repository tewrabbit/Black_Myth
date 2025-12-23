# 黑神话：悟空 - 项目说明

## 项目概述
基于Unreal Engine 5和C++开发的动作角色扮演游戏，复刻《黑神话：悟空》核心玩法。

## 团队成员
- 邹世豪：敌人AI与互动
- 赵佑赫：主角核心动作系统
- 罗强：UI系统
- 齐欣然：游戏地图与关卡设计

## 技术栈
- 引擎：Unreal Engine 5
- 语言：C++
- 输入系统：Enhanced Input System
- 动画系统：Animation Montage

## 编译运行
1. 安装UE5引擎
2. 安装Visual Studio 2022
3. 从epic games中的fab里下载ParagonSunWukong，ParagonFengmao，ParagonGideon,ParagonNarbash,ParagonRampage五个动作资源拖入content文件夹中
4. 右键WUKONGPROJECT.uproject - 生成Visual Studio项目文件
5. 打开.sln文件，编译WUKONGPROJECT项目
6. 双击WUKONGPROJECT.uproject启动游戏

## 基础功能
- 角色系统：悟空模型，基础动作（移动、跳跃、闪避、攻击）
- 战斗系统：生命值、攻击判定、技能系统
- 敌人系统：封魔敌人，AI行为（巡逻、追逐、攻击、闪避）
- UI系统：血条、蓝条、背包、技能槽、暂停菜单

## 操作说明
- WASD：移动
- 鼠标：视角
- 空格：跳跃
- 左Shift：冲刺
- 鼠标左键：轻攻击
- 鼠标右键：重攻击
- F：闪避
- Q：定身法
- E：喝药
- V：隐身
- R：变身