#include <U8g2lib.h>

#define SPEED 2  // 16的因数
#define ICON_SPEED 8
#define ICON_SPACE 48  // 图标间隔，speed 倍数

#define BTN3 D3  // back
#define BTN0 8   // up
#define BTN1 9   // down
#define BTN2 10  // ok

const float PID_MAX = 10.00;  // PID最大允许值
const uint8_t kMenuTextOffsetX = 4;
const uint8_t BLINK_SPEED = 16;  // 2的倍数
const uint8_t name_len = 12;     // 0-11 for name  12 for return
const unsigned long debounce_delay = 100;  // 防抖时间
