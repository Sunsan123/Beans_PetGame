# BeansPetGame Arduino Guide

这份说明对应中文主程序 [JurassicLifeCN.ino](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\JurassicLifeCN.ino)。

## 当前状态

- 品牌和 UI 语境已切到 `BeansPetGame / 龙泡泡 / 博士 / 罗德岛`
- 主属性保留 6 项：`饱腹 / 水分 / 疲劳 / 卫生 / 心情 / 亲密`
- 交互保留 7 项：`休息 / 喂食 / 喝水 / 洗澡 / 玩耍 / 清理 / 互动`
- 已有 RTC 自动探测、RTC 校时入口、离岗报告弹窗、离线结算
- 已加入 `龙门币` 货币系统，按分钟自然增长，离线结算封顶 24h
- 已加入场景切换系统，默认场景 `宿舍`，当前支持 `活动室 / 甲板`
- 自动行为和每日事件都改成头文件表驱动

## 支持板型

当前代码支持：

- `DISPLAY_PROFILE_2432S022`
- `DISPLAY_PROFILE_2432S028`
- `DISPLAY_PROFILE_ILI9341_320x240`
- `DISPLAY_PROFILE_ESP32S3_ST7789`

默认启用：

```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_ESP32S3_ST7789
```

## 常用配置

可直接在程序顶部修改：

```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_ESP32S3_ST7789
#define ENABLE_AUDIO 1
```

存档后端默认规则：

- `ESP32-S3 + ST7789`：`SD`
- 其他板型：`SD`

## ESP32-S3 默认按键与 RTC 引脚

当前 `ESP32-S3_ST7789` 默认按键：

- `BTN_LEFT = GPIO7`
- `BTN_RIGHT = GPIO5`
- `BTN_OK = GPIO8`

当前 `ESP32-S3_ST7789` 默认 RTC I2C：

- `RTC SDA = GPIO9`
- `RTC SCL = GPIO10`

接 `DS3231` 时建议：

- `VCC -> 3.3V`
- `GND -> GND`
- `SDA -> GPIO9`
- `SCL -> GPIO10`

如果是 `2432S022`，RTC 会复用触摸 I2C。

## RTC 与离线待机

当前实现支持：

- 启动时自动探测 `DS3231`
- 读取 RTC 时间并进行离线结算
- RTC 无效时自动跳过结算
- RTC 已接入但未校时时自动弹出校时页
- 开机弹出“博士离岗报告”摘要页

离线结算目前会处理：

- 主属性自然流逝
- 休整中的疲劳恢复
- 健康惩罚
- 成长和阶段变化
- 补给点 / 饮水站刷新
- 虚弱 / 离岛 / 记录完成

## 场景切换

当前第一版场景：

- `宿舍`
- `活动室`
- `甲板`

当前实现范围：

- 场景名称、背景配色、装饰层、补给点和饮水点位置跟随场景切换
- 场景写入存档，启动后自动恢复
- 默认场景为 `宿舍`

当前未做：

- 场景专属行为池
- 场景专属事件池
- 场景专属收益倍率

切换入口：

- 触屏：点击底栏最左或最右的场景箭头
- 按键：当底栏选中最左或最右交互位后，继续按 `LEFT / RIGHT` 切换上一场景或下一场景

## 自动行为表

自动行为现在由头文件表驱动，不再使用 CSV。

唯一编辑源：

- [AutoBehaviorTableCN.h](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\AutoBehaviorTableCN.h)

文件顶部已经写好字段说明，策划直接改数组内容即可。

核心字段：

- `enabled`：是否启用
- `weight`：抽取权重
- `behavior`：行为名称
- `moveMode`：`AUTO_MOVE_IDLE` 或 `AUTO_MOVE_WALK`
- `art`：动作素材键
- `bubbleText`：气泡文案，多条候选用 `|` 分隔
- `minMs / maxMs`：持续时间

当前默认行为池包含：

- 散步
- 发呆
- 期待
- 调皮
- 休息
- 睡觉
- 撒娇

## 每日事件表

每日事件也改为头文件表驱动，不再使用 CSV。

唯一编辑源：

- [DailyEventTableCN.h](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\DailyEventTableCN.h)

核心字段：

- `enabled`：是否启用
- `weight`：抽取权重
- `eventName`：事件名称
- `art`：弹窗素材键
- `summary`：事件说明
- `deltaFaim / deltaSoif / deltaFatigue / deltaHygiene / deltaHumeur / deltaAmour / deltaSante`：事件收益或损失

当前默认事件池包含：

- 补给到达
- 干员来访
- 偷吃被发现
- 房间弄乱
- 获得玩具
- 解锁观察记录

## 存档字段

当前存档已经写入以下关键字段：

- `timeValid`
- `lastUnixTime`
- `lastSleeping`
- `dailyLastDay`
- `dailyPending`
- `phase`
- `stage`
- `pet`

## 建议调试顺序

1. 先确认板型和屏幕正常启动
2. 再确认按键输入
3. 再确认存档能正常创建
4. 然后接入 `DS3231`
5. 最后再调两个 `.h` 配置表

## 备注

- 目前不再有 `CSV -> .h` 的生成链路，也不再依赖 Arduino 平台预编译钩子
- 本机当前没有 `arduino-cli`，仓库内最近修改主要做了静态检查

## RTC 调试补充

最近对 RTC 相关流程补了两点：

- 主程序启动时会对 `DS3231` 做短暂重试，不再只探测一次
- 离线结算提示现在会区分“未检测到 RTC 模块”和“RTC 已检测到但时间无效”

如果你怀疑 RTC 接线、供电或掉电保时异常，可以先单独烧录这个测试程序：

- [RTC_DS3231_Test.ino](C:\Users\17787\Desktop\Beans_PetGame\arduino\RTC_DS3231_Test\RTC_DS3231_Test.ino)

默认测试引脚按当前仓库默认板型 `DISPLAY_PROFILE_ESP32S3_ST7789` 设置：

- `SDA = GPIO9`
- `SCL = GPIO10`
- `I2C = 400kHz`

串口监视器使用：

- 波特率 `115200`
- 烧录后会自动执行一次测试
- 发送 `r` 可重新测试
- 发送 `s` 可把 RTC 设为编译时间后再重测

测试程序会做这些检查：

- 扫描 I2C 设备，确认是否出现 `0x68`
- 读取 `DS3231` 时间寄存器和状态寄存器
- 检查是否 `running=yes`
- 检查是否 `osf=no`
- 间隔约 1.2 秒再次读取，确认秒数有继续前进

结果判读：

- 出现 `PASS: RTC is connected and ticking.`：说明 RTC 已正确连接并正常走时
- 出现 `FAIL: DS3231 not found at 0x68`：优先检查 `VCC / GND / SDA / SCL`
- 出现 `WARN: RTC is connected, but time is not valid yet.`：说明模块在线，但时间无效，通常要先校时，或检查后备电池

补充说明：

- 很多 `DS3231` 小板在 I2C 扫描时会同时出现 `0x57` 和 `0x68`
- `0x68` 是 `DS3231`
- `0x57` 通常是板载 `AT24C32 EEPROM`，属于正常现象
