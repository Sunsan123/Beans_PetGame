# BeansPetGame

BeansPetGame 是一个运行在 ESP32 小屏设备上的龙泡泡养成放置游戏。当前中文主线版本采用“博士在罗德岛照顾龙泡泡”的世界观语境，主程序位于 [arduino/JurassicLifeCN](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN)。

## 当前版本

- 主属性保留 6 项：`饱腹 / 水分 / 疲劳 / 卫生 / 心情 / 亲密`
- 主交互保留 7 项：`休息 / 喂食 / 喝水 / 洗澡 / 玩耍 / 清理 / 互动`
- 中文 UI 已切换到 `Bean / 龙泡泡 / 博士 / 罗德岛` 语境
- 已接入 `DS3231 RTC` 自动探测、离线待机结算、RTC 未校时入口、离岗报告弹窗
- 已接入 `龙门币` 货币系统，在线与离线都会随时间增长，离线结算上限 24 小时
- 已接入场景切换系统，默认场景为 `宿舍`，当前可切到 `活动室 / 甲板`
- 已接入自动行为池，支持行为名、动作素材、气泡文案、时长配置
- 已接入每日事件系统，支持每天 1 到 3 个事件、在线弹窗和离线补发提醒

## 策划配置方式

现在不再使用 CSV，也不再依赖预编译生成。

策划只需要直接维护两个头文件：

- 行为表：[AutoBehaviorTableCN.h](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\AutoBehaviorTableCN.h)
- 每日事件表：[DailyEventTableCN.h](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\DailyEventTableCN.h)

这两个文件都采用“数组表”格式，字段名固定，注释写在文件顶部，非程序人员也可以直接照着改。

### 行为表字段

`enabled, weight, behavior, moveMode, art, bubbleText, minMs, maxMs`

- `enabled`：是否启用
- `weight`：抽取权重
- `behavior`：行为名称
- `moveMode`：`AUTO_MOVE_IDLE` 或 `AUTO_MOVE_WALK`
- `art`：`AUTO_ART_AUTO / AUTO_ART_MARCHE / AUTO_ART_ASSIE / AUTO_ART_CLIGNE / AUTO_ART_DODO / AUTO_ART_AMOUR / AUTO_ART_MANGE`
- `bubbleText`：气泡文案，多条候选用 `|` 分隔
- `minMs / maxMs`：行为持续时间，单位毫秒

### 每日事件表字段

`enabled, weight, eventName, art, summary, deltaFaim, deltaSoif, deltaFatigue, deltaHygiene, deltaHumeur, deltaAmour, deltaSante`

- `enabled`：是否启用
- `weight`：抽取权重
- `eventName`：事件标题
- `art`：事件弹窗对应素材
- `summary`：事件描述
- `delta*`：对属性或健康的收益/损失

## RTC 与离线待机

当前实现为 `RTC-ready` 架构，推荐硬件为 `DS3231`。

已支持：

- 启动时自动探测 RTC
- 读取真实时间并执行离线结算
- RTC 时间无效时跳过离线结算，不影响正常进入游戏
- RTC 未校时时自动弹出校时入口
- 开机显示“博士离岗报告”

离线结算会覆盖：

- 主属性自然流逝
- 休整中的疲劳恢复
- 健康惩罚
- 成长推进
- 补给点与饮水站刷新
- 虚弱 / 离岛 / 记录完成判定

## 场景切换

当前第一版已支持 3 个场景：

- `宿舍`
- `活动室`
- `甲板`

规则说明：

- 新档和老档升级后默认进入 `宿舍`
- 场景会写入存档，重启后会恢复到上次所在场景
- 切换场景后，会同步切换背景风格、补给点位置、饮水点位置和场景名称显示
- 当前第一版不包含场景专属收益倍率、场景专属行为池、场景专属事件池

切换方式：

- 触屏设备：点击底栏最左或最右的场景箭头
- 三按键设备：当底栏选中最左或最右交互位后，继续按 `LEFT / RIGHT` 切换上一场景或下一场景

## 主要目录

- [arduino/JurassicLifeCN](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN)：中文主程序
- [JurassicLifeCN.ino](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\JurassicLifeCN.ino)：当前主入口
- [README.md](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\README.md)：Arduino、板型、RTC、配置表说明
- [screenshots](C:\Users\17787\Desktop\Beans_PetGame\screenshots)：演示图和录屏素材

## 快速开始

1. 打开 [JurassicLifeCN.ino](C:\Users\17787\Desktop\Beans_PetGame\arduino\JurassicLifeCN\JurassicLifeCN.ino)
2. 选择板型和音频配置
3. 烧录到设备
4. 如需真实离线待机，再接入 `DS3231 RTC`
5. 如需调自动行为和每日事件，直接编辑两个 `.h` 配置表

## 备注

- 仓库里仍保留一些历史命名，例如 `JurassicLifeCN.ino` 和部分 `Dino` 内部变量名；玩家可见品牌和世界观已切到 `BeansPetGame`
- 本机当前没有 `arduino-cli`，最近几次修改主要做了静态检查，未做本地完整编译验证

## RTC 调试说明

仓库里现在额外提供了一个独立的 `DS3231` 测试草图：

- [RTC_DS3231_Test.ino](C:\Users\17787\Desktop\Beans_PetGame\arduino\RTC_DS3231_Test\RTC_DS3231_Test.ino)

用途：

- 单独验证 RTC 是否接通
- 单独验证 RTC 是否真的在走时
- 区分“未检测到模块”和“RTC 已接上但时间无效”

当前默认按 `ESP32-S3_ST7789` 这套接线测试：

- `SDA -> GPIO9`
- `SCL -> GPIO10`
- `VCC -> 3.3V`
- `GND -> GND`

使用方法：

1. 单独烧录 `RTC_DS3231_Test.ino`
2. 打开串口监视器，波特率设为 `115200`
3. 观察是否扫描到 `0x68`
4. 观察是否最终输出 `PASS: RTC is connected and ticking.`

串口命令：

- `r`：重新执行一次测试
- `s`：把 RTC 设置为编译时间后再重测

补充说明：

- 扫描到 `0x68` 表示 `DS3231` 在线
- 同时扫描到 `0x57` 也正常，通常是模块板上的 `AT24C32 EEPROM`
- 主游戏现在也已补充 RTC 启动重试，并把“未检测到模块”和“RTC 时间无效”分成两种提示
