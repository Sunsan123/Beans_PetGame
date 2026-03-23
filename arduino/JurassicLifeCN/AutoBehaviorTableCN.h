#pragma once

// 行为表唯一编辑源。
// 策划只需要改下面这张数组表，不需要 CSV，不需要额外生成步骤。
//
// 字段说明：
// enabled    : 是否启用该行为，false 时整行忽略
// weight     : 抽取权重，越大越容易被选中
// behavior   : 行为名称，会显示在状态和调试信息里
// moveMode   : AUTO_MOVE_IDLE 原地待机，AUTO_MOVE_WALK 左右散步
// art        : 使用哪套现有动作素材
// bubbleText : 气泡文案，多条候选用 | 分隔
// minMs/maxMs: 行为持续时间，单位毫秒

struct AutoBehaviorConfigEntry {
  bool enabled;
  uint16_t weight;
  const char* behavior;
  AutoBehaviorMoveMode moveMode;
  AutoBehaviorArt art;
  const char* bubbleText;
  uint16_t minMs;
  uint16_t maxMs;
};

static const AutoBehaviorConfigEntry kAutoBehaviorTableCN[] = {
  { true, 28, "散步", AUTO_MOVE_WALK, AUTO_ART_AUTO,   "去甲板绕一圈|去罗德岛走走",         2800, 7200 },
  { true, 18, "发呆", AUTO_MOVE_IDLE, AUTO_ART_ASSIE,  "发会儿呆|等博士来摸摸头",           1400, 3200 },
  { true, 14, "期待", AUTO_MOVE_IDLE, AUTO_ART_AMOUR,  "今天会有新的陪伴吗|博士会来看我吗", 1200, 2600 },
  { true, 12, "调皮", AUTO_MOVE_IDLE, AUTO_ART_CLIGNE, "嘿嘿 先装乖一下|今天想调皮一点",     900, 1800 },
  { true, 14, "休息", AUTO_MOVE_IDLE, AUTO_ART_ASSIE,  "先趴一会儿|补充一点精神",           1800, 3600 },
  { true,  8, "睡觉", AUTO_MOVE_IDLE, AUTO_ART_DODO,   "呼……补个觉|龙泡泡进入待机",         3200, 7600 },
  { true,  6, "撒娇", AUTO_MOVE_IDLE, AUTO_ART_AMOUR,  "抱抱我嘛|博士别走",                 1200, 2400 },
};

static const uint8_t kAutoBehaviorTableCNCount =
    (uint8_t)(sizeof(kAutoBehaviorTableCN) / sizeof(kAutoBehaviorTableCN[0]));
