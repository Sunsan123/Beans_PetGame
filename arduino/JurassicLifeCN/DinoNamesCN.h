// DinoNamesCN.h
#pragma once

#include <Arduino.h>
#include <pgmspace.h>

// 200个中文恐龙名字（逗号分隔）
// 使用UTF-8编码
static const char DINO_NAMES_CSV[] PROGMEM =
  "小豆,阿宝,团子,糯米,芝麻,花生,核桃,板栗,红豆,绿豆,"
  "黄豆,毛毛,球球,豆豆,泡泡,圆圆,乐乐,欢欢,喜喜,多多,"
  "旺旺,福福,贝贝,莉莉,甜甜,蜜蜜,糖糖,果果,朵朵,芽芽,"
  "苗苗,叶叶,花花,草草,星星,月月,阳阳,雨雨,雪雪,风风,"
  "云云,霜霜,露露,虹虹,光光,辉辉,亮亮,明明,晨晨,曦曦,"
  "大壮,小强,阿力,铁蛋,石头,山山,岩岩,峰峰,河河,海海,"
  "波波,浪浪,涛涛,溪溪,泉泉,湖湖,江江,川川,林林,森森,"
  "木木,竹竹,松松,柏柏,梅梅,兰兰,菊菊,荷荷,莲莲,桃桃,"
  "杏杏,李李,桂桂,枫枫,柳柳,杨杨,榕榕,椿椿,桐桐,樱樱,"
  "龙龙,凤凤,虎虎,豹豹,鹰鹰,鹤鹤,雀雀,燕燕,鸽鸽,莺莺,"
  "蝶蝶,蜂蜂,蚁蚁,鱼鱼,虾虾,蟹蟹,贝贝,螺螺,蛙蛙,龟龟,"
  "兔兔,猫猫,狗狗,熊熊,鹿鹿,马马,羊羊,牛牛,猪猪,鸡鸡,"
  "金金,银银,铜铜,铁铁,玉玉,珠珠,翠翠,瑶瑶,琳琳,琪琪,"
  "瑞瑞,祥祥,吉吉,庆庆,宁宁,安安,康康,健健,壮壮,强强,"
  "勇勇,智智,慧慧,聪聪,灵灵,巧巧,敏敏,捷捷,飞飞,翔翔,"
  "奔奔,跑跑,跳跳,舞舞,歌歌,琴琴,棋棋,书书,画画,诗诗,"
  "文文,武武,仁仁,义义,礼礼,信信,忠忠,孝孝,善善,德德,"
  "美美,丽丽,秀秀,雅雅,静静,柔柔,温温,暖暖,清清,纯纯,"
  "真真,诚诚,正正,直直,方方,平平,顺顺,通通,达达,发发,"
  "旺财,来福,如意,吉祥,太平,长安,永乐,天佑,地利,人和";

// UTF-8中文字符可能是2-4字节，这里统一处理
static inline uint16_t dinoCountCsvItems_P(const char* csvP) {
  if (!csvP) return 0;
  if (pgm_read_byte(csvP) == '\0') return 0;

  uint16_t count = 1;
  for (uint32_t i = 0;; i++) {
    char c = (char)pgm_read_byte(csvP + i);
    if (c == '\0') break;
    if (c == ',') count++;
  }
  return count;
}

// 复制一个随机名字到 out（outSize 包含 '\0'）
static inline void getRandomDinoName(char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  out[0] = '\0';

  const uint16_t n = dinoCountCsvItems_P(DINO_NAMES_CSV);
  if (n == 0) return;

  const uint16_t target = (uint16_t)random(n);

  // 跳到目标名字的开头
  uint16_t idx = 0;
  uint32_t i = 0;
  while (idx < target) {
    char c = (char)pgm_read_byte(DINO_NAMES_CSV + i++);
    if (c == '\0') return;
    if (c == ',') idx++;
  }

  // 复制到逗号或结尾
  size_t w = 0;
  while (w + 1 < outSize) {
    char c = (char)pgm_read_byte(DINO_NAMES_CSV + i++);
    if (c == '\0' || c == ',') break;
    out[w++] = c;
  }
  out[w] = '\0';
}
