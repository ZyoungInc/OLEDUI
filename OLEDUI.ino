#include <U8g2lib.h>

#define SPEED 2  // 16的因数
#define ICON_SPEED 8
#define ICON_SPACE 48  // 图标间隔，speed 倍数

#define BTN3 D3  // back
#define BTN0 8   // up
#define BTN1 9   // down
#define BTN2 10  // ok

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/D5, /* data=*/D4);

enum class InputAction : uint8_t {
  None,
  Up,
  Down,
  Select,
  Back,
};

struct InputEvent {
  InputAction action = InputAction::None;
  bool repeated = false;
};

// 菜单项定义，关联标题与切换目标，方便扩展新的页面
struct MenuItem {
  const char* label;
  uint8_t targetPage;  // 使用 ui_index 枚举值
};

struct PidMenuItem {
  const char* label;
  bool editable;
};

struct IconItem {
  const uint8_t* bitmap;
  uint8_t width;
  uint8_t height;
  const char* label;
};

// 菜单显示状态集中管理，便于在不同输入设备之间复用
struct MenuState {
  int16_t scroll = 0;
  int16_t scrollTarget = 0;
  uint8_t indicatorY = 0;
  uint8_t indicatorTarget = 0;
  uint8_t boxWidth = 0;
  uint8_t boxWidthTarget = 0;
  int16_t boxY = 0;
  int16_t boxYTarget = 0;
  int8_t selected = 0;
};

InputEvent g_inputEvent;
InputAction g_lastMenuDirection = InputAction::None;

PROGMEM const uint8_t icon_pic[][200]{
  {
    0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x3E,
    0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00,
    0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00,
    0x7F, 0x00, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00,
    0x00, 0x80, 0x7F, 0x00, 0x00, 0x00, 0xC0, 0x7F,
    0x00, 0x00, 0x00, 0xE0, 0x7F, 0x00, 0x00, 0x00,
    0xF8, 0x7F, 0x00, 0x00, 0xF0, 0xF8, 0xFF, 0xFF,
    0x01, 0xFC, 0xF8, 0xFF, 0xFF, 0x07, 0xFC, 0xF8,
    0xFF, 0xFF, 0x07, 0xFE, 0xF8, 0xFF, 0xFF, 0x07,
    0xFE, 0xF8, 0xFF, 0xFF, 0x07, 0xFE, 0xF8, 0xFF,
    0xFF, 0x07, 0xFE, 0xF8, 0xFF, 0xFF, 0x07, 0xFE,
    0xF8, 0xFF, 0xFF, 0x07, 0xFE, 0xF8, 0xFF, 0xFF,
    0x03, 0xFE, 0xF8, 0xFF, 0xFF, 0x03, 0xFE, 0xF8,
    0xFF, 0xFF, 0x03, 0xFE, 0xF8, 0xFF, 0xFF, 0x03,
    0xFE, 0xF8, 0xFF, 0xFF, 0x01, 0xFE, 0xF8, 0xFF,
    0xFF, 0x01, 0xFE, 0xF8, 0xFF, 0xFF, 0x01, 0xFE,
    0xF8, 0xFF, 0xFF, 0x01, 0xFE, 0xF8, 0xFF, 0xFF,
    0x00, 0xFE, 0xF8, 0xFF, 0xFF, 0x00, 0xFC, 0xF8,
    0xFF, 0x7F, 0x00, 0xFC, 0xF8, 0xFF, 0x3F, 0x00,
    0xF8, 0xF8, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /*"C:\Users\ROG\Desktop\三连\点赞.bmp",0*/
    /* (36 X 35 )*/
  },
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00,
    0x00, 0x1F, 0x00, 0x00, 0x00, 0x80, 0x1F, 0x00,
    0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0xC0,
    0x3F, 0x00, 0x00, 0x00, 0xC0, 0x3F, 0x00, 0x00,
    0x00, 0xC0, 0x7F, 0x00, 0x00, 0x00, 0xE0, 0xFF,
    0x00, 0x00, 0x00, 0xF0, 0xFF, 0x01, 0x00, 0x00,
    0xFC, 0xFF, 0x03, 0x00, 0xE0, 0xFF, 0xFF, 0xFF,
    0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0x07, 0xFE, 0xFF,
    0xFF, 0xFF, 0x07, 0xFC, 0xFF, 0xFF, 0xFF, 0x07,
    0xFC, 0xFF, 0xFF, 0xFF, 0x03, 0xF8, 0xFF, 0xFF,
    0xFF, 0x01, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0xE0,
    0xFF, 0xFF, 0x7F, 0x00, 0xC0, 0xFF, 0xFF, 0x3F,
    0x00, 0x80, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0xFF,
    0xFF, 0x1F, 0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00,
    0x00, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0xFF, 0xFF,
    0x0F, 0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x00, 0x00,
    0xFF, 0xFF, 0x0F, 0x00, 0x00, 0xFF, 0xFF, 0x0F,
    0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0xFF,
    0xFF, 0x1F, 0x00, 0x80, 0xFF, 0xF0, 0x1F, 0x00,
    0x80, 0x3F, 0xC0, 0x1F, 0x00, 0x80, 0x1F, 0x00,
    0x1F, 0x00, 0x00, 0x07, 0x00, 0x1C, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00 /*"C:\Users\ROG\Desktop\三连\收藏.bmp",0*/
    /* (36 X 37 )*/
  },
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x1F,
    0x00, 0x00, 0x00, 0xF0, 0xFF, 0x01, 0x00, 0x00,
    0xFC, 0xFF, 0x07, 0x00, 0x00, 0xFF, 0xFF, 0x0F,
    0x00, 0x80, 0xFF, 0xFF, 0x1F, 0x00, 0xC0, 0xFF,
    0xFF, 0x7F, 0x00, 0xE0, 0x07, 0x00, 0x7C, 0x00,
    0xF0, 0x03, 0x00, 0xFC, 0x00, 0xF0, 0x03, 0x00,
    0xFC, 0x01, 0xF8, 0xFF, 0xF1, 0xFF, 0x01, 0xF8,
    0xFF, 0xF1, 0xFF, 0x03, 0xF8, 0x7F, 0xC0, 0xFF,
    0x03, 0xFC, 0x1F, 0x00, 0xFF, 0x03, 0xFC, 0x07,
    0x00, 0xFE, 0x07, 0xFC, 0x07, 0x01, 0xFC, 0x07,
    0xFC, 0xC3, 0x31, 0xF8, 0x07, 0xFC, 0xE1, 0xF1,
    0xF8, 0x07, 0xFC, 0xF1, 0xF1, 0xF0, 0x07, 0xFC,
    0xF1, 0xF1, 0xF0, 0x07, 0xFC, 0xF1, 0xF1, 0xF1,
    0x07, 0xFC, 0xF1, 0xF1, 0xF1, 0x07, 0xFC, 0xF1,
    0xF1, 0xF1, 0x03, 0xF8, 0xF1, 0xF1, 0xF1, 0x03,
    0xF8, 0xFF, 0xF1, 0xFF, 0x03, 0xF8, 0xFF, 0xF1,
    0xFF, 0x01, 0xF0, 0xFF, 0xF1, 0xFF, 0x01, 0xF0,
    0xFF, 0xF1, 0xFF, 0x00, 0xE0, 0xFF, 0xF1, 0x7F,
    0x00, 0xC0, 0xFF, 0xFF, 0x7F, 0x00, 0x80, 0xFF,
    0xFF, 0x3F, 0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x00,
    0x00, 0xFC, 0xFF, 0x07, 0x00, 0x00, 0xF0, 0xFF,
    0x01, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 /*"C:\Users\ROG\Desktop\三连\投币.bmp",0*/
    /* (36 X 36 )*/
  },
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00,
    0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00,
    0xFC, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x01, 0x00,
    0x00, 0x00, 0xFC, 0x03, 0x00, 0x00, 0x00, 0xFC,
    0x07, 0x00, 0x00, 0x00, 0xFC, 0x0F, 0x00, 0x00,
    0x00, 0xFE, 0x1F, 0x00, 0x00, 0xF8, 0xFF, 0x3F,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xC0, 0xFF,
    0xFF, 0xFF, 0x01, 0xE0, 0xFF, 0xFF, 0xFF, 0x03,
    0xF0, 0xFF, 0xFF, 0xFF, 0x07, 0xF0, 0xFF, 0xFF,
    0xFF, 0x0F, 0xF8, 0xFF, 0xFF, 0xFF, 0x0F, 0xFC,
    0xFF, 0xFF, 0xFF, 0x07, 0xFC, 0xFF, 0xFF, 0xFF,
    0x03, 0xFE, 0xFF, 0xFF, 0xFF, 0x01, 0xFE, 0xFF,
    0xFF, 0xFF, 0x00, 0xFF, 0x03, 0xFE, 0x3F, 0x00,
    0xFF, 0x00, 0xFC, 0x1F, 0x00, 0x3F, 0x00, 0xFC,
    0x0F, 0x00, 0x1F, 0x00, 0xFC, 0x07, 0x00, 0x07,
    0x00, 0xFC, 0x03, 0x00, 0x03, 0x00, 0xFC, 0x01,
    0x00, 0x01, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00,
    0x3C, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00,
    0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /*"C:\Users\13944\Desktop\fenxiang.bmp",0*/
  },
};

uint8_t icon_width[] = { 35, 37, 36, 36 };

//main界面图片
PROGMEM const uint8_t LOGO[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x10, 0x01, 0x24, 0x00, 0x44, 0x04, 0x80, 0x20, 0x00, 0x00, 0x10, 0xF2, 0x3C, 0x20, 0xE0, 0x00,
  0x10, 0x01, 0x24, 0x00, 0xC4, 0x04, 0x80, 0x20, 0x00, 0x00, 0x10, 0x11, 0x24, 0x30, 0x10, 0x01,
  0x10, 0x71, 0x24, 0x0F, 0xC4, 0xE2, 0x99, 0x3C, 0x00, 0x00, 0x20, 0x11, 0x24, 0x20, 0x10, 0x01,
  0xF0, 0x89, 0xA4, 0x10, 0xA8, 0x12, 0x8A, 0x22, 0x00, 0x00, 0x20, 0xF1, 0x1C, 0x20, 0x10, 0x01,
  0x10, 0xF9, 0xA4, 0x10, 0x98, 0x13, 0x8A, 0x22, 0x00, 0x00, 0xA0, 0x10, 0x24, 0x20, 0x10, 0x01,
  0x10, 0x09, 0xA4, 0x10, 0x98, 0x11, 0x8A, 0x22, 0x00, 0x00, 0xA0, 0x10, 0x24, 0x20, 0x10, 0x01,
  0x10, 0xF1, 0x24, 0x0F, 0x10, 0xE1, 0x89, 0x3C, 0x00, 0x00, 0x40, 0xF0, 0x44, 0x20, 0xE2, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xE7, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x0F, 0xFE, 0x99, 0xFF, 0xE4, 0x1F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xE7, 0xFD, 0xBD, 0xFF, 0xDE, 0xDF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0xFE,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xDF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFD,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xDF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFD,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0x1F, 0xF0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFD,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xFF, 0x1F, 0xFE, 0xFF, 0x83, 0xFF, 0xE3, 0xF1,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xFF, 0xE3, 0xF1, 0xFF, 0x7C, 0xF8, 0xF9, 0xEF,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xFF, 0xFD, 0xEF, 0x3F, 0xFF, 0xF3, 0xFD, 0xDF,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xFF, 0x1E, 0xDC, 0xBF, 0x03, 0xF7, 0xE3, 0xE1,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0x7F, 0xEF, 0xB9, 0xDF, 0xFB, 0xF0, 0xEF, 0xFD,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0x7F, 0xE7, 0xBB, 0xDF, 0xF3, 0xFF, 0xEF, 0xFD,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xBF, 0xF7, 0x7B, 0xDF, 0x0F, 0xFF, 0xEF, 0xFD,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xBF, 0x0F, 0x7C, 0xBF, 0xFF, 0xFC, 0xEF, 0xFD,
  0xF7, 0xFD, 0x7D, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xBF, 0xFF, 0xBF, 0x7F, 0xFE, 0xF3, 0xEF, 0xFD,
  0xF7, 0xFD, 0xBE, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xBF, 0x07, 0xC0, 0xFF, 0xE1, 0xF7, 0xEF, 0xFD,
  0xEF, 0xFB, 0xBE, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xBF, 0xEF, 0xFF, 0xFF, 0x9F, 0xEF, 0xEF, 0xFD,
  0xEF, 0x07, 0xBF, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0x7F, 0xEF, 0xC7, 0x3F, 0x3E, 0xEF, 0xEF, 0xFD,
  0xDF, 0xFF, 0xDF, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xFF, 0x1E, 0xB8, 0xDF, 0xB9, 0xF7, 0xEF, 0xE3,
  0x9F, 0xFF, 0xEF, 0x7F, 0xDF, 0xFF, 0xF7, 0xFD, 0xFF, 0xFC, 0xBF, 0x9F, 0xC7, 0xF7, 0xDF, 0xCF,
  0x7F, 0xFF, 0xF3, 0xFF, 0xDE, 0xFF, 0xEF, 0xFD, 0xFF, 0xFB, 0xCF, 0x3F, 0xFF, 0xF9, 0x9F, 0xEF,
  0xFF, 0x00, 0xFC, 0xFF, 0xE1, 0xFF, 0x1F, 0xFE, 0xFF, 0x07, 0xF0, 0xFF, 0x00, 0xFE, 0x7F, 0xF0,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x00, 0x40, 0x00, 0x02, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x88, 0x9C, 0x24, 0xC7, 0x1C, 0x0F, 0x68, 0x22, 0xA0, 0xCC, 0x23, 0x91, 0xC7, 0x01,
  0x00, 0xFE, 0x88, 0xA2, 0xAA, 0x48, 0xA2, 0x08, 0x98, 0x14, 0xB0, 0x24, 0x22, 0x91, 0x24, 0x02,
  0x00, 0x00, 0x78, 0xA2, 0xAB, 0x4F, 0xBE, 0x08, 0x88, 0x14, 0xF0, 0x24, 0x22, 0x91, 0x24, 0x02,
  0x00, 0x00, 0x08, 0x22, 0x9B, 0x40, 0x82, 0x08, 0x88, 0x0C, 0x08, 0x25, 0x22, 0x91, 0x24, 0x02,
  0x00, 0x00, 0x08, 0x1C, 0x11, 0x4F, 0x3C, 0x0F, 0x78, 0x08, 0x08, 0xC5, 0xE3, 0x91, 0xC4, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const float PID_MAX = 10.00;  //PID最大允许值
//PID变量
float Kpid[3] = { 9.97, 0.2, 0.01 };  //Kp,Ki,Kd
// float Kp=8.96;
// float Ki=0.2;
// float Kd=0.01;


uint8_t disappear_step = 1;

float angle, angle_last;
uint8_t chart_x;
bool frame_is_drawed = false;

uint8_t* buf_ptr;
uint16_t buf_len;

MenuState mainMenuState;
MenuState pidMenuState;

const uint8_t kMenuTextOffsetX = 4;

int16_t icon_x, icon_x_trg;
int16_t icon_label_offset, icon_label_offset_trg;
int8_t icon_select;

uint8_t ui_index, ui_state;


enum  //ui_index
{
  M_LOGO,       //开始界面
  M_SELECT,     //选择界面
  M_PID,        //PID界面
  M_PID_EDIT,   //pid编辑
  M_ICON,       //icon界面
  M_CHART,      //图表
  M_TEXT_EDIT,  //文字编辑
  M_VIDEO,      //视频显示
  M_ABOUT,      //关于本机
};


enum  //ui_state
{
  S_NONE,
  S_DISAPPEAR,
  S_SWITCH,
  S_MENU_TO_MENU,
  S_MENU_TO_PIC,
  S_PIC_TO_MENU,
};

//const char* text="This is a text Hello world ! follow up one two three four may jun july";

const PidMenuItem kPidMenu[] = {
  { "-Proportion", true },
  { "-Integral", true },
  { "-Derivative", true },
  { "Return", false },
};

const uint8_t kPidMenuCount = sizeof(kPidMenu) / sizeof(PidMenuItem);

// 主菜单配置，可直接在此处增删条目
const MenuItem kMainMenu[] = {
  { "MainUI", M_LOGO },
  { "+PID Editor", M_PID },
  { "-Icon Test", M_ICON },
  { "-Chart Test", M_CHART },
  { "-Text Edit", M_TEXT_EDIT },
  { "-Play Video", M_VIDEO },
  { "{ About }", M_ABOUT },
};

const uint8_t kMainMenuCount = sizeof(kMainMenu) / sizeof(MenuItem);
const uint8_t kMainMenuLineSegment = 63 / kMainMenuCount;
const uint8_t kMainMenuLineTotal = kMainMenuLineSegment * kMainMenuCount + 1;
const uint8_t kMainMenuVisibleRows = 4;

const uint8_t kPidLineSegment = 15;

// 图标资源配置，集中管理图片与标题
const IconItem kIconMenu[] = {
  { icon_pic[0], 36, 35, "Likes" },
  { icon_pic[1], 36, 37, "Collection" },
  { icon_pic[2], 36, 36, "Slot" },
  { icon_pic[3], 36, 36, "Share" },
};

const uint8_t kIconCount = sizeof(kIconMenu) / sizeof(IconItem);

//设备名称
char name[] = "Hello World ";
//允许名字的最大长度
const uint8_t name_len = 12;  //0-11for name  12 for return
uint8_t edit_index = 0;
bool edit_flag = false;          //默认不在编辑
uint8_t blink_flag;              //默认高亮
const uint8_t BLINK_SPEED = 16;  //2的倍数

//按键变量
typedef struct
{
  bool val;
  bool last_val;
} KEY;

KEY key[4] = { false };

bool get_key_val(uint8_t ch) {
  switch (ch) {
    case 0:
      return digitalRead(BTN0);
    case 1:
      return digitalRead(BTN1);
    case 2:
      return digitalRead(BTN2);
    case 3:
      return digitalRead(BTN3);  // 获取返回按钮的状态
    default:
      return false;  // 如果ch不是0、1、2或3，返回false
  }
}

void key_init() {
  for (uint8_t i = 0; i < (sizeof(key) / sizeof(KEY)); ++i) {
    key[i].val = key[i].last_val = get_key_val(i);
  }
}

const unsigned long debounce_delay = 100;  // 防抖时间

InputAction actionFromKeyIndex(uint8_t index) {
  switch (index) {
    case 0:
      return InputAction::Up;
    case 1:
      return InputAction::Down;
    case 2:
      return InputAction::Select;
    case 3:
      return InputAction::Back;
    default:
      return InputAction::None;
  }
}

// 统一的输入轮询，后续可替换为编码器或其他设备，仅需在此处映射动作
InputEvent pollInput() {
  static unsigned long lastRepeatTime[4] = { 0 };
  InputEvent event;
  unsigned long current_time = millis();

  for (uint8_t i = 0; i < (sizeof(key) / sizeof(KEY)); ++i) {
    bool value = get_key_val(i);
    key[i].val = value;

    if (value != key[i].last_val) {
      key[i].last_val = value;
      if (value == LOW) {
        lastRepeatTime[i] = current_time;
        event.action = actionFromKeyIndex(i);
        event.repeated = false;
        return event;
      }
    } else if ((i == 0 || i == 1) && value == LOW && (current_time - lastRepeatTime[i] > debounce_delay)) {
      lastRepeatTime[i] = current_time;
      event.action = actionFromKeyIndex(i);
      event.repeated = true;
      return event;
    }
  }

  return event;
}




//移动函数
bool move(int16_t* a, int16_t* a_trg) {
  if (*a < *a_trg) {
    *a += SPEED;
    if (*a > *a_trg) *a = *a_trg;  //加完超过
  } else if (*a > *a_trg) {
    *a -= SPEED;
    if (*a < *a_trg) *a = *a_trg;  //减完不足
  } else {
    return true;  //到达目标
  }
  return false;  //未到达
}

//移动函数
bool move_icon(int16_t* a, int16_t* a_trg) {
  if (*a < *a_trg) {
    *a += ICON_SPEED;
    if (*a > *a_trg) *a = *a_trg;  //加完超过
  } else if (*a > *a_trg) {
    *a -= ICON_SPEED;
    if (*a < *a_trg) *a = *a_trg;  //减完不足
  } else {
    return true;  //到达目标
  }
  return false;  //未到达
}

typedef const char* (*LabelProvider)(uint8_t index);

const char* getMainMenuLabel(uint8_t index) {
  return kMainMenu[index].label;
}

const char* getPidMenuLabel(uint8_t index) {
  return kPidMenu[index].label;
}

bool move_width(uint8_t* current, uint8_t* target, uint8_t selected, InputAction direction, LabelProvider labelProvider, uint8_t itemCount) {
  if (itemCount == 0) {
    return true;
  }

  auto labelWidth = [](const char* text) -> uint8_t {
    return text ? u8g2.getStrWidth(text) : 0;
  };

  int8_t neighborIndex = -1;
  if (direction == InputAction::Up) {
    neighborIndex = (selected + 1 < itemCount) ? selected + 1 : selected;
  } else if (direction == InputAction::Down) {
    neighborIndex = (selected > 0) ? selected - 1 : selected;
  }

  uint8_t selectedWidth = labelWidth(labelProvider(selected));
  uint8_t neighborWidth = (neighborIndex >= 0) ? labelWidth(labelProvider(neighborIndex)) : selectedWidth;
  uint8_t step = 16 / SPEED;
  uint8_t diff = abs(selectedWidth - neighborWidth);
  uint8_t width_speed = (diff % step == 0) ? (diff / step) : (diff / step + 1);
  if (width_speed == 0) {
    width_speed = 1;
  }

  if (*current < *target) {
    *current += width_speed;
    if (*current > *target) {
      *current = *target;
    }
  } else if (*current > *target) {
    *current -= width_speed;
    if (*current < *target) {
      *current = *target;
    }
  } else {
    return true;
  }

  return false;
}

bool move_indicator(uint8_t* current, uint8_t* target, uint8_t segmentLength) {
  uint8_t step = 16 / SPEED;
  uint8_t delta = (segmentLength % step == 0) ? (segmentLength / step) : (segmentLength / step + 1);

  if (*current < *target) {
    *current += delta;
    if (*current > *target) {
      *current = *target;
    }
  } else if (*current > *target) {
    *current -= delta;
    if (*current < *target) {
      *current = *target;
    }
  } else {
    return true;
  }

  return false;
}

//文字编辑函数
void text_edit(bool dir, uint8_t index) {
  if (!dir) {
    if (name[index] >= 'A' && name[index] <= 'Z')  //大写字母
    {
      if (name[index] == 'A') {
        name[index] = 'z';
      } else {
        name[index] -= 1;
      }
    } else if (name[index] >= 'a' && name[index] <= 'z')  //小写字母
    {
      if (name[index] == 'a') {
        name[index] = ' ';
      } else {
        name[index] -= 1;
      }
    } else {
      name[index] = 'Z';
    }
  } else {
    if (name[index] >= 'A' && name[index] <= 'Z')  //大写字母
    {
      if (name[index] == 'Z') {
        name[index] = ' ';
      } else {
        name[index] += 1;
      }
    } else if (name[index] >= 'a' && name[index] <= 'z')  //小写字母
    {
      if (name[index] == 'z') {
        name[index] = 'A';
      } else {
        name[index] += 1;
      }
    } else {
      name[index] = 'a';
    }
  }
}

//消失函数
void disappear() {
  switch (disappear_step) {
    case 1:
      for (uint16_t i = 0; i < buf_len; ++i) {
        if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55;
      }
      break;
    case 2:
      for (uint16_t i = 0; i < buf_len; ++i) {
        if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA;
      }
      break;
    case 3:
      for (uint16_t i = 0; i < buf_len; ++i) {
        if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00;
      }
      break;
    case 4:
      for (uint16_t i = 0; i < buf_len; ++i) {
        if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00;
      }
      break;
    default:
      ui_state = S_NONE;
      disappear_step = 0;
      break;
  }
  disappear_step++;
}



/**************************界面显示*******************************/

void logo_ui_show()  //显示logo{
  u8g2.drawXBMP(0, 0, 128, 64, LOGO);

// for(uint16_t i=0;i<buf_len;++i)
// {
//   if(i%4==0||i%4==1)
//   {
//   buf_ptr[i]=buf_ptr[i] & 0x33;
//   }
//   else
//   {
//   buf_ptr[i]=buf_ptr[i] & 0xCC;
//   }
// }
}

void select_ui_show() {
  move_indicator(&mainMenuState.indicatorY, &mainMenuState.indicatorTarget, kMainMenuLineSegment);
  move(&mainMenuState.scroll, &mainMenuState.scrollTarget);
  move(&mainMenuState.boxY, &mainMenuState.boxYTarget);
  move_width(&mainMenuState.boxWidth, &mainMenuState.boxWidthTarget, mainMenuState.selected, g_lastMenuDirection, getMainMenuLabel, kMainMenuCount);

  u8g2.drawVLine(126, 0, kMainMenuLineTotal);
  u8g2.drawPixel(125, 0);
  u8g2.drawPixel(127, 0);
  for (uint8_t i = 0; i < kMainMenuCount; ++i) {
    u8g2.drawStr(kMenuTextOffsetX, 16 * i + mainMenuState.scroll + 12, kMainMenu[i].label);
    u8g2.drawPixel(125, kMainMenuLineSegment * (i + 1));
    u8g2.drawPixel(127, kMainMenuLineSegment * (i + 1));
  }

  u8g2.drawVLine(125, mainMenuState.indicatorY, kMainMenuLineSegment - 1);
  u8g2.drawVLine(127, mainMenuState.indicatorY, kMainMenuLineSegment - 1);
  u8g2.setDrawColor(2);
  u8g2.drawRBox(0, mainMenuState.boxY, mainMenuState.boxWidth, 16, 1);
  u8g2.setDrawColor(1);
}

void pid_ui_show() {
  move_indicator(&pidMenuState.indicatorY, &pidMenuState.indicatorTarget, kPidLineSegment);
  move(&pidMenuState.boxY, &pidMenuState.boxYTarget);
  move_width(&pidMenuState.boxWidth, &pidMenuState.boxWidthTarget, pidMenuState.selected, g_lastMenuDirection, getPidMenuLabel, kPidMenuCount);

  u8g2.drawVLine(126, 0, 61);
  u8g2.drawPixel(125, 0);
  u8g2.drawPixel(127, 0);
  for (uint8_t i = 0; i < kPidMenuCount; ++i) {
    u8g2.drawStr(kMenuTextOffsetX, 16 * i + 12, kPidMenu[i].label);
    u8g2.drawPixel(125, kPidLineSegment * (i + 1));
    u8g2.drawPixel(127, kPidLineSegment * (i + 1));
  }

  u8g2.setDrawColor(2);
  u8g2.drawRBox(0, pidMenuState.boxY, pidMenuState.boxWidth, 16, 1);
  u8g2.setDrawColor(1);
  u8g2.drawVLine(125, pidMenuState.indicatorY, 14);
  u8g2.drawVLine(127, pidMenuState.indicatorY, 14);
}

void pid_edit_ui_show() {  // 显示 PID 编辑界面
  u8g2.drawBox(16, 16, 96, 31);
  u8g2.setDrawColor(2);
  u8g2.drawBox(17, 17, 94, 29);
  u8g2.setDrawColor(1);

  //u8g2.drawBox(17,17,(int)(Kpid[pidMenuState.selected]/PID_MAX)),30);
  u8g2.drawFrame(18, 36, 60, 8);
  u8g2.drawBox(20, 38, (uint8_t)(Kpid[pidMenuState.selected] / PID_MAX * 56), 4);

  u8g2.setCursor(22, 30);
  switch (pidMenuState.selected) {
    case 0:
      u8g2.print("Editing Kp");
      break;
    case 1:
      u8g2.print("Editing Ki");
      break;
    case 2:
      u8g2.print("Editing Kd");
      break;
    default:
      break;
  }

  u8g2.setCursor(81, 44);
  u8g2.print(Kpid[pidMenuState.selected]);
}

void icon_ui_show() {
  move_icon(&icon_x, &icon_x_trg);
  move(&icon_label_offset, &icon_label_offset_trg);

  for (uint8_t i = 0; i < kIconCount; ++i) {
    const IconItem& item = kIconMenu[i];
    u8g2.drawXBMP(46 + icon_x + i * ICON_SPACE, 6, item.width, item.height, item.bitmap);
    u8g2.setClipWindow(0, 48, 128, 64);
    u8g2.drawStr((128 - u8g2.getStrWidth(item.label)) / 2, 62 - icon_label_offset + i * 16, item.label);
    u8g2.setMaxClipWindow();
  }
}

void chart_ui_show() {
  if (!frame_is_drawed) {
    u8g2.clearBuffer();
    chart_draw_frame();
    angle_last = 20.00 + (float)random(0, 1024) / 100.00;
    frame_is_drawed = true;
  }

  u8g2.drawBox(96, 0, 30, 14);
  u8g2.drawVLine(chart_x + 10, 59, 3);
  if (chart_x == 100) {
    chart_x = 0;
  }

  u8g2.drawVLine(chart_x + 11, 24, 34);
  u8g2.drawVLine(chart_x + 12, 24, 34);
  u8g2.drawVLine(chart_x + 13, 24, 34);
  u8g2.drawVLine(chart_x + 14, 24, 34);

  u8g2.setDrawColor(2);
  angle = 20.00 + (float)random(0, 1024) / 100.00;
  u8g2.drawLine(chart_x + 11, 58 - (int)angle_last / 2, chart_x + 12, 58 - (int)angle / 2);
  u8g2.drawVLine(chart_x + 12, 59, 3);
  angle_last = angle;
  chart_x += 2;
  u8g2.drawBox(96, 0, 30, 14);
  u8g2.setDrawColor(1);
  u8g2.setCursor(96, 12);
  u8g2.print(angle);
}

void chart_draw_frame()  //chart界面
{

  u8g2.drawStr(4, 12, "Real time angle :");
  u8g2.drawRBox(4, 18, 120, 46, 8);
  u8g2.setDrawColor(2);
  u8g2.drawHLine(10, 58, 108);
  u8g2.drawVLine(10, 24, 34);
  //箭头
  u8g2.drawPixel(7, 27);
  u8g2.drawPixel(8, 26);
  u8g2.drawPixel(9, 25);

  u8g2.drawPixel(116, 59);
  u8g2.drawPixel(115, 60);
  u8g2.drawPixel(114, 61);
  u8g2.setDrawColor(1);
}

void text_edit_ui_show() {
  u8g2.drawRFrame(4, 6, 120, 52, 8);
  u8g2.drawStr((128 - u8g2.getStrWidth("--Text Editor--")) / 2, 20, "--Text Editor--");
  u8g2.drawStr(10, 38, name);
  u8g2.drawStr(80, 50, "-Return");

  uint8_t box_x = 9;

  //绘制光标
  if (edit_index < name_len) {
    if (blink_flag < BLINK_SPEED / 2) {
      for (uint8_t i = 0; i < edit_index; ++i) {
        char temp[2] = { name[i], '\0' };
        box_x += u8g2.getStrWidth(temp);
        if (name[i] != ' ') {
          box_x++;
        }
      }
      char temp[2] = { name[edit_index], '\0' };
      u8g2.setDrawColor(2);
      u8g2.drawBox(box_x, 26, u8g2.getStrWidth(temp) + 2, 16);
      u8g2.setDrawColor(1);
    }
  } else {
    u8g2.setDrawColor(2);
    u8g2.drawRBox(78, 38, u8g2.getStrWidth("-Return") + 4, 16, 1);
    u8g2.setDrawColor(1);
  }

  if (edit_flag)  //处于编辑状态
  {
    if (blink_flag < BLINK_SPEED) {
      blink_flag += 1;
    } else {
      blink_flag = 0;
    }
  } else {
    blink_flag = 0;
  }
}

void about_ui_show()  //about界面
{

  u8g2.drawStr(2, 12, "CocoFactory Sleep Porj");
  u8g2.drawStr(2, 28, "    Rortable Radar Kit");
  u8g2.drawStr(2, 44, "FreeRAM : 517KB");
  u8g2.drawStr(2, 60, "Sensor : R60A");

  // u8g2.drawStr(2,12,"MCU : MEGA2560");
  // u8g2.drawStr(2,28,"FLASH : 256KB");
  // u8g2.drawStr(2,44,"SRAM : 8KB");
  // u8g2.drawStr(2,60,"EEPROM : 4KB");
}

/**************************界面处理*******************************/

void logo_proc() {
  if (g_inputEvent.action != InputAction::None) {
    g_inputEvent.action = InputAction::None;
    ui_state = S_DISAPPEAR;
    ui_index = M_SELECT;
  }
  logo_ui_show();
}

void pid_edit_proc() {
  if (g_inputEvent.action != InputAction::None) {
    switch (g_inputEvent.action) {
      case InputAction::Up:
        if (Kpid[pidMenuState.selected] > 0) {
          Kpid[pidMenuState.selected] -= 0.01;
        }
        break;
      case InputAction::Down:
        if (Kpid[pidMenuState.selected] < PID_MAX) {
          Kpid[pidMenuState.selected] += 0.01;
        }
        break;
      case InputAction::Select:
      case InputAction::Back:
        ui_index = M_PID;
        break;
      default:
        break;
    }
    g_inputEvent.action = InputAction::None;
  }

  pid_ui_show();
  for (uint16_t i = 0; i < buf_len; ++i) {
    buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);
  }
  pid_edit_ui_show();
}

void pid_proc() {
  pid_ui_show();

  if (g_inputEvent.action == InputAction::None) {
    return;
  }

  InputAction action = g_inputEvent.action;
  g_inputEvent.action = InputAction::None;

  switch (action) {
    case InputAction::Up:
      if (pidMenuState.selected == 0) {
        break;
      }
      pidMenuState.selected -= 1;
      pidMenuState.indicatorTarget -= 15;
      pidMenuState.boxYTarget -= 16;
      break;
    case InputAction::Down:
      if (pidMenuState.selected >= kPidMenuCount - 1) {
        break;
      }
      pidMenuState.selected += 1;
      pidMenuState.indicatorTarget += 15;
      pidMenuState.boxYTarget += 16;
      break;
    case InputAction::Select:
      if (kPidMenu[pidMenuState.selected].editable) {
        ui_index = M_PID_EDIT;
      } else {
        ui_state = S_DISAPPEAR;
        ui_index = M_SELECT;
        pidMenuState.selected = 0;
        pidMenuState.indicatorY = pidMenuState.indicatorTarget = 1;
        pidMenuState.boxY = pidMenuState.boxYTarget = 0;
        pidMenuState.boxWidth = pidMenuState.boxWidthTarget = u8g2.getStrWidth(kPidMenu[pidMenuState.selected].label) + kMenuTextOffsetX * 2;
      }
      break;
    case InputAction::Back:
      ui_state = S_DISAPPEAR;
      ui_index = M_SELECT;
      pidMenuState.selected = 0;
      pidMenuState.indicatorY = pidMenuState.indicatorTarget = 1;
      pidMenuState.boxY = pidMenuState.boxYTarget = 0;
      pidMenuState.boxWidth = pidMenuState.boxWidthTarget = u8g2.getStrWidth(kPidMenu[pidMenuState.selected].label) + kMenuTextOffsetX * 2;
      break;
    default:
      break;
  }

  pidMenuState.boxWidthTarget = u8g2.getStrWidth(kPidMenu[pidMenuState.selected].label) + kMenuTextOffsetX * 2;
}

void select_proc() {
  if (g_inputEvent.action != InputAction::None) {
    InputAction action = g_inputEvent.action;
    g_inputEvent.action = InputAction::None;

    switch (action) {
      case InputAction::Up:
        if (mainMenuState.selected < 1) {
          break;
        }
        mainMenuState.selected -= 1;
        mainMenuState.indicatorTarget -= kMainMenuLineSegment;
        if (mainMenuState.selected < -(mainMenuState.scroll / 16)) {
          mainMenuState.scrollTarget += 16;
        } else {
          mainMenuState.boxYTarget -= 16;
        }
        break;
      case InputAction::Down:
        if (mainMenuState.selected >= kMainMenuCount - 1) {
          break;
        }
        mainMenuState.selected += 1;
        mainMenuState.indicatorTarget += kMainMenuLineSegment;
        if ((mainMenuState.selected + 1) > (kMainMenuVisibleRows - mainMenuState.scroll / 16)) {
          mainMenuState.scrollTarget -= 16;
        } else {
          mainMenuState.boxYTarget += 16;
        }
        break;
      case InputAction::Select: {
        const MenuItem& item = kMainMenu[mainMenuState.selected];
        ui_state = S_DISAPPEAR;
        ui_index = item.targetPage;
        break;
      }
      case InputAction::Back:
        ui_state = S_DISAPPEAR;
        ui_index = M_SELECT;
        break;
      default:
        break;
    }

    mainMenuState.boxWidthTarget = u8g2.getStrWidth(kMainMenu[mainMenuState.selected].label) + kMenuTextOffsetX * 2;
  }

  select_ui_show();
}


void icon_proc() {
  icon_ui_show();

  if (g_inputEvent.action == InputAction::None) {
    return;
  }

  InputAction action = g_inputEvent.action;
  g_inputEvent.action = InputAction::None;

  switch (action) {
    case InputAction::Down:
      if (icon_select < static_cast<int8_t>(kIconCount) - 1) {
        icon_select += 1;
        icon_label_offset_trg += 16;
        icon_x_trg -= ICON_SPACE;
      }
      break;
    case InputAction::Up:
      if (icon_select > 0) {
        icon_select -= 1;
        icon_label_offset_trg -= 16;
        icon_x_trg += ICON_SPACE;
      }
      break;
    case InputAction::Select:
    case InputAction::Back:
      ui_state = S_DISAPPEAR;
      ui_index = M_SELECT;
      icon_select = 0;
      icon_x = icon_x_trg = 0;
      icon_label_offset = icon_label_offset_trg = 0;
      break;
    default:
      break;
  }
}

void chart_proc() {
  chart_ui_show();
  if (g_inputEvent.action != InputAction::None) {
    g_inputEvent.action = InputAction::None;
    ui_state = S_DISAPPEAR;
    ui_index = M_SELECT;
    frame_is_drawed = false;
    chart_x = 0;
  }
}

void text_edit_proc() {
  text_edit_ui_show();

  if (g_inputEvent.action == InputAction::None) {
    return;
  }

  InputAction action = g_inputEvent.action;
  g_inputEvent.action = InputAction::None;

  switch (action) {
    case InputAction::Up:
      if (edit_flag) {
        text_edit(false, edit_index);
      } else {
        if (edit_index == 0) {
          edit_index = name_len;
        } else {
          edit_index -= 1;
        }
      }
      break;
    case InputAction::Down:
      if (edit_flag) {
        text_edit(true, edit_index);
      } else {
        if (edit_index == name_len) {
          edit_index = 0;
        } else {
          edit_index += 1;
        }
      }
      break;
    case InputAction::Select:
      if (edit_index == name_len) {
        ui_state = S_DISAPPEAR;
        ui_index = M_SELECT;
        edit_index = 0;
        edit_flag = false;
      } else {
        edit_flag = !edit_flag;
      }
      break;
    case InputAction::Back:
      ui_state = S_DISAPPEAR;
      ui_index = M_SELECT;
      edit_index = 0;
      edit_flag = false;
      break;
    default:
      break;
  }
}

void about_proc() {
  if (g_inputEvent.action != InputAction::None) {
    g_inputEvent.action = InputAction::None;
    ui_state = S_DISAPPEAR;
    ui_index = M_SELECT;
  }
  about_ui_show();
}
/********************************总的UI显示************************************/

void ui_proc()  //总的UI进程
{
  switch (ui_state) {
    case S_NONE:
      if (ui_index != M_CHART) u8g2.clearBuffer();
      switch (ui_index) {
        case M_LOGO:
          logo_proc();
          break;
        case M_SELECT:
          select_proc();
          break;
        case M_PID:
          pid_proc();
          break;
        case M_ICON:
          icon_proc();
          break;
        case M_CHART:
          chart_proc();
          break;
        case M_TEXT_EDIT:
          text_edit_proc();
          break;
        case M_PID_EDIT:
          pid_edit_proc();
          break;
        case M_ABOUT:
          about_proc();
          break;
        default:
          break;
      }
      break;
    case S_DISAPPEAR:
      disappear();
      break;
    default:
      break;
  }
  u8g2.sendBuffer();
}


void setup() {
  Serial.begin(9600);
  //Wire.begin(21,22,400000);
  pinMode(BTN0, INPUT_PULLUP);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);  // 设置返回按钮
  key_init();
  u8g2.setBusClock(2000000);
  u8g2.begin();
  u8g2.setFont(u8g2_font_wqy12_t_chinese1);
  //u8g2.setContrast(10);

  buf_ptr = u8g2.getBufferPtr();  //拿到buffer首地址
  buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();

  mainMenuState.scroll = mainMenuState.scrollTarget = 0;
  mainMenuState.indicatorY = mainMenuState.indicatorTarget = 1;
  mainMenuState.boxY = mainMenuState.boxYTarget = 0;
  mainMenuState.selected = 0;
  mainMenuState.boxWidth = mainMenuState.boxWidthTarget = u8g2.getStrWidth(kMainMenu[0].label) + kMenuTextOffsetX * 2;

  pidMenuState.scroll = pidMenuState.scrollTarget = 0;
  pidMenuState.indicatorY = pidMenuState.indicatorTarget = 1;
  pidMenuState.boxY = pidMenuState.boxYTarget = 0;
  pidMenuState.selected = 0;
  pidMenuState.boxWidth = pidMenuState.boxWidthTarget = u8g2.getStrWidth(kPidMenu[0].label) + kMenuTextOffsetX * 2;

  icon_select = 0;
  icon_x = icon_x_trg = 0;
  icon_label_offset = icon_label_offset_trg = 0;

  g_lastMenuDirection = InputAction::None;

  ui_index = M_LOGO;
  //ui_index=M_TEXT_EDIT;
  ui_state = S_NONE;
}

void loop() {
  g_inputEvent = pollInput();
  if (g_inputEvent.action == InputAction::Up || g_inputEvent.action == InputAction::Down) {
    g_lastMenuDirection = g_inputEvent.action;
  }
  ui_proc();
}
