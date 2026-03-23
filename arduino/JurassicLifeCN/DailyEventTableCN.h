#pragma once

// 每日事件表唯一编辑源。
// 策划只需要改下面这张数组表，不需要 CSV，不需要额外生成步骤。
//
// 字段说明：
// enabled   : 是否启用该事件
// weight    : 被抽中的概率权重
// eventName : 事件标题
// art       : 弹窗和演示使用的现有动作素材
// summary   : 事件描述文案
// delta*    : 事件结算后对属性和健康的改变量，可填正数或负数

struct DailyEventConfigEntry {
  bool enabled;
  uint16_t weight;
  const char* eventName;
  AutoBehaviorArt art;
  const char* summary;
  int8_t deltaFaim;
  int8_t deltaSoif;
  int8_t deltaFatigue;
  int8_t deltaHygiene;
  int8_t deltaHumeur;
  int8_t deltaAmour;
  int8_t deltaSante;
};

static const DailyEventConfigEntry kDailyEventTableCN[] = {
  { true, 20, "补给到达",     AUTO_ART_MARCHE, "罗德岛补给舱送来新的储备",        12, 10,  0,   0,  6,  2,  0 },
  { true, 16, "干员来访",     AUTO_ART_AMOUR,  "有干员来陪龙泡泡玩了一会儿",       0,  0, -4,   0, 12, 10,  2 },
  { true, 12, "偷吃被发现",   AUTO_ART_MANGE,  "龙泡泡偷吃储备餐被博士当场发现",  10,  0, -2,  -4, -6, -2,  0 },
  { true, 12, "房间弄乱",     AUTO_ART_CLIGNE, "宿舍被闹得乱糟糟 需要重新整理",     0,  0, -4, -14, -8, -3, -2 },
  { true, 14, "获得玩具",     AUTO_ART_AMOUR,  "后勤送来一件新的玩具样品",         0,  0,  2,   0, 14,  8,  0 },
  { true,  8, "解锁观察记录", AUTO_ART_ASSIE,  "观察记录补完了一页 龙泡泡很得意",   0,  0,  0,   0,  8,  6,  4 },
};

static const uint8_t kDailyEventTableCNCount =
    (uint8_t)(sizeof(kDailyEventTableCN) / sizeof(kDailyEventTableCN[0]));
