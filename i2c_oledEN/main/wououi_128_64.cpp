
/*
  此项目模仿自稚晖君暂未开源的 MonoUI，用于实现类似 UltraLink 的丝滑界面

  有四个版本：

    * 分辨率 128 * 128 ：主菜单，列表和关于本机页基本还原了 UltraLink 的界面和动态效果，电压测量页为原创设计
    * 分辨率 128 * 64 ：主菜单模仿 UltraLink 重新设计，去掉了小标题，电压测量页重新设计，关于本机页面改为列表，列表适配了该分辨率
    * 分辨率 128 * 32 ：在 128 * 64 分辨率的基础上，主界面只保留图标，电压测量页重新设计
    * 通用版本：仅保留列表，主菜单也改为列表，删除电压测量页和与列表无关的动画（保留弹窗效果），经过简单修改可以适配任何分辨率，任何行高度的情况

  WouoUI v2 功能：

    * 全部使用非线性的平滑缓动动画，包括列表，弹窗，甚至进度条
    * 优化平滑动画算法到只有两行，分类别定义平滑权重，并且每个权重值都分别可调
    * 可以打断的非线性动画，当前动画未结束但下一次动画已经被触发时，动画可以自然过渡
    * 非通用版本分别适配了类似 UltraLink 主菜单的磁贴界面（因为让我想起 WP7 的 Metron 风格，所以称之为磁贴）
    * 通用版本仅保留列表类界面，经过简单修改可以适配所有分辨率的屏幕，包括屏幕内行数不是整数的情况
    * 列表菜单，列表可以无限延长
    * 列表文字选择框，选择框可根据选择的字符串长度自动伸缩，进入菜单时从列表开头从长度 0 展开，转到上一级列表时，长度和纵坐标平滑移动到上一级选择的位置
    * 列表单选框，储存数据时也储存该值在列表中所在的位置，展开列表时根据每行开头的字符判断是否绘制外框，再根据位置数据判断是否绘制里面的点
    * 列表多选框，储存数据的数组跟多选框列表的行数对应，不要求连续排列，展开列表时根据每行开头的字符判断是否绘制外框，再根据行数对应的储存数据位置的数值是否为1判断是否绘制里面的点
    * 列表显示数值，与多选框原理相似，但不涉及修改操作
    * 列表展开动画，初始化列表时，可以选择列表从头开始展开，或者从上次选中的位置展开
    * 图标展开动画，初始化磁贴类界面时，可以选择图标从头开始展开，或者从上次选中的位置展开
    * 弹出窗口，实现了窗口弹出的动画效果，可以自定义最大值，最小值，步进值，需要修改的参数等，窗口独立运行，调用非常简单
    * 弹出窗口背景虚化可选项，背景虚化会产生卡顿感，但删掉代码有些可惜，因此做成可选项，默认关闭
    * 亮度调节，在弹出窗口中调节亮度值可以实时改变当前亮度值
  * 旋钮功能，使用EC11旋钮控制，旋钮方向可以软件调整。原版包含 USB 控制电脑的示例，但这份 ESP32-C3 移植默认不启用；当前保留的是本地菜单与按键配置逻辑，按键支持短按输入与返回，旋钮消抖时长等参数可以在弹出窗口中调整
    * 循环模式，选择项超过头尾时，选择项跳转到另一侧继续，列表和磁贴类可以分别选择
    * 黑暗模式，其实本来就是黑暗模式，是增加了白天模式，默认开启黑暗模式
    * 消失动画适配两种模式，一种是渐变成全黑，另一种渐变成全白
    * 断电存储，用简单直观的方式将每种功能参数写入EEPROM，只在修改过参数，进入睡眠模式时写入，避免重复擦写，初始化时检查11个标志位，允许一位误码

  项目参考：

    * B站：路徍要什么自行车：在线仿真：https://wokwi.com/projects/350306511434547796，https://www.bilibili.com/video/BV1HA411S7pv/
    * Github：createskyblue：OpenT12：https://github.com/createskyblue/OpenT12
    * Github：peng-zhihui：OpenHeat：https://github.com/peng-zhihui/OpenHeat

  注意事项：

    * 这份 ESP32-C3 移植默认不启用 USB HID；如果后续要恢复电脑控制功能，需要补入 USB 设备栈并重新整理按键映射

  本项目不设置开源协议，如需商用或借鉴，在醒目处标注本项目开源地址即可
  欢迎关注我的B站账号，用户名：音游玩的人，B站主页：https://space.bilibili.com/9182439?spm_id_from=..0.0
*/

/************************************* 屏幕驱动 *************************************/

// 分辨率128*64，使用 ESP-IDF + U8g2 兼容封装

#include <assert.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"

#define PROGMEM

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wwrite-strings"

#ifndef U8X8_PIN_NONE
#define U8X8_PIN_NONE (-1)
#endif

static const char *TAG = "wououi_128_64";

#define SCL   11
#define SDA   10
#define RST   U8X8_PIN_NONE

static constexpr int OLED_HW_ADDR_PRIMARY = 0x3C;
static constexpr int OLED_HW_ADDR_SECONDARY = 0x3D;
static constexpr int I2C_BUS_PORT = 0;
static constexpr uint32_t I2C_FREQ_HZ = 800000;
static constexpr int I2C_TIMEOUT_MS = 1000;
static constexpr int OLED_PIN_NUM_SDA = SDA;
static constexpr int OLED_PIN_NUM_SCL = SCL;

static i2c_master_bus_handle_t s_i2c_bus = nullptr;
static i2c_master_dev_handle_t s_display_dev = nullptr;
static uint8_t s_oled_addr = OLED_HW_ADDR_PRIMARY;
static uint32_t s_i2c_speed_hz = I2C_FREQ_HZ;

static uint8_t u8g2_byte_i2c_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
static uint8_t u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

class U8G2_SH1106_128X64_NONAME_F_HW_I2C {
public:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C(const u8g2_cb_t *rotation, int, int, int)
  {
    u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, rotation, u8g2_byte_i2c_cb, u8g2_gpio_and_delay_cb);
  }

  void begin()
  {
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
  }

  void setBusClock(uint32_t hz) { s_i2c_speed_hz = hz; }
  void setContrast(uint8_t c) { u8g2_SetContrast(&u8g2, c); }
  void clearBuffer() { u8g2_ClearBuffer(&u8g2); }
  void sendBuffer() { u8g2_SendBuffer(&u8g2); }
  void setPowerSave(uint8_t on) { u8g2_SetPowerSave(&u8g2, on); }
  void setDrawColor(uint8_t c) { u8g2_SetDrawColor(&u8g2, c); }
  void setFont(const uint8_t *font) { u8g2_SetFont(&u8g2, font); }
  void setFontDirection(uint8_t dir) { u8g2_SetFontDirection(&u8g2, dir); }
  void setCursor(int16_t x, int16_t y) { cursor_x = x; cursor_y = y; }
  uint8_t *getBufferPtr() { return u8g2_GetBufferPtr(&u8g2); }
  uint16_t getBufferTileHeight() { return u8g2_GetBufferTileHeight(&u8g2); }
  uint16_t getBufferTileWidth() { return u8g2_GetBufferTileWidth(&u8g2); }
  uint16_t getStrWidth(const char *str) { return u8g2_GetStrWidth(&u8g2, str); }
  void drawStr(int16_t x, int16_t y, const char *str) { u8g2_DrawStr(&u8g2, x, y, str); }
  void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) { u8g2_DrawBox(&u8g2, x, y, w, h); }
  void drawFrame(int16_t x, int16_t y, int16_t w, int16_t h) { u8g2_DrawFrame(&u8g2, x, y, w, h); }
  void drawRFrame(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r) { u8g2_DrawRFrame(&u8g2, x, y, w, h, r); }
  void drawRBox(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r) { u8g2_DrawRBox(&u8g2, x, y, w, h, r); }
  void drawHLine(int16_t x, int16_t y, int16_t w) { u8g2_DrawHLine(&u8g2, x, y, w); }
  void drawVLine(int16_t x, int16_t y, int16_t h) { u8g2_DrawVLine(&u8g2, x, y, h); }
  void drawPixel(int16_t x, int16_t y) { u8g2_DrawPixel(&u8g2, x, y); }
  void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) { u8g2_DrawLine(&u8g2, x1, y1, x2, y2); }
  void drawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bmp) { u8g2_DrawXBMP(&u8g2, x, y, w, h, bmp); }
  int16_t getAscent() const { return u8g2_GetAscent(&u8g2); }
  int16_t getDescent() const { return u8g2_GetDescent(&u8g2); }
  void setClipWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) { u8g2_SetClipWindow(&u8g2, x0, y0, x1, y1); }
  void setMaxClipWindow() { u8g2_SetMaxClipWindow(&u8g2); }

  void print(const char *s)
  {
    u8g2_DrawStr(&u8g2, cursor_x, cursor_y, s);
    cursor_x += u8g2_GetStrWidth(&u8g2, s);
  }

  void print(char c)
  {
    char s[2] = { c, '\0' };
    print(s);
  }

  void print(int v)
  {
    char s[16];
    std::snprintf(s, sizeof(s), "%d", v);
    print(s);
  }

  void print(unsigned int v)
  {
    char s[16];
    std::snprintf(s, sizeof(s), "%u", v);
    print(s);
  }

  void print(long v)
  {
    char s[24];
    std::snprintf(s, sizeof(s), "%ld", v);
    print(s);
  }

  void print(unsigned long v)
  {
    char s[24];
    std::snprintf(s, sizeof(s), "%lu", v);
    print(s);
  }

  void print(float v)
  {
    char s[24];
    std::snprintf(s, sizeof(s), "%.2f", v);
    print(s);
  }

  u8g2_t u8g2{};
  int16_t cursor_x = 0;
  int16_t cursor_y = 0;
};

static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, SCL, SDA, RST);

static esp_err_t scan_oled_address(uint8_t *found_addr)
{
  const uint8_t candidates[] = { OLED_HW_ADDR_PRIMARY, OLED_HW_ADDR_SECONDARY };
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    esp_err_t err = i2c_master_probe(s_i2c_bus, candidates[i], 100);
    if (err == ESP_OK) {
      *found_addr = candidates[i];
      return ESP_OK;
    }
  }
  *found_addr = OLED_HW_ADDR_PRIMARY;
  return ESP_ERR_NOT_FOUND;
}

static uint8_t u8g2_byte_i2c_cb(u8x8_t *, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t buffer[132];
  static uint8_t buf_idx;

  switch (msg) {
    case U8X8_MSG_BYTE_INIT: {
      if (s_display_dev != nullptr) {
        return 1;
      }
      i2c_device_config_t dev_config = {};
      dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
      dev_config.device_address = s_oled_addr;
      dev_config.scl_speed_hz = s_i2c_speed_hz;
      dev_config.scl_wait_us = 0;
      dev_config.flags.disable_ack_check = false;
      esp_err_t ret = i2c_master_bus_add_device(s_i2c_bus, &dev_config, &s_display_dev);
      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(ret));
        return 0;
      }
      return 1;
    }
    case U8X8_MSG_BYTE_START_TRANSFER:
      buf_idx = 0;
      return 1;
    case U8X8_MSG_BYTE_SET_DC:
      return 1;
    case U8X8_MSG_BYTE_SEND:
      for (uint8_t i = 0; i < arg_int; ++i) {
        if (buf_idx < sizeof(buffer)) {
          buffer[buf_idx++] = reinterpret_cast<uint8_t *>(arg_ptr)[i];
        }
      }
      return 1;
    case U8X8_MSG_BYTE_END_TRANSFER:
      if (buf_idx > 0 && s_display_dev != nullptr) {
        esp_err_t ret = i2c_master_transmit(s_display_dev, buffer, buf_idx, I2C_TIMEOUT_MS);
        if (ret != ESP_OK) {
          ESP_LOGE(TAG, "I2C transmit failed: %s", esp_err_to_name(ret));
          return 0;
        }
      }
      return 1;
    default:
      return 0;
  }
}

static uint8_t u8g2_gpio_and_delay_cb(u8x8_t *, uint8_t msg, uint8_t arg_int, void *)
{
  switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      return 1;
    case U8X8_MSG_DELAY_MILLI:
      vTaskDelay(pdMS_TO_TICKS(arg_int));
      return 1;
    case U8X8_MSG_DELAY_10MICRO:
      esp_rom_delay_us(arg_int * 10);
      return 1;
    case U8X8_MSG_DELAY_100NANO:
      __asm__ __volatile__("nop");
      return 1;
    case U8X8_MSG_DELAY_I2C:
      esp_rom_delay_us(5);
      return 1;
    case U8X8_MSG_GPIO_RESET:
      return 1;
    default:
      return 1;
  }
}

static inline void delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
static inline int map(long value, long fromLow, long fromHigh, long toLow, long toHigh)
{
  return static_cast<int>((value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow);
}

#ifndef INPUT
#define INPUT 0
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 1
#endif
#ifndef OUTPUT
#define OUTPUT 2
#endif
#ifndef CHANGE
#define CHANGE 3
#endif
#ifndef LOW
#define LOW 0
#endif
#ifndef HIGH
#define HIGH 1
#endif

#ifndef KEY_ESC
#define KEY_ESC 0
#define KEY_F1 0
#define KEY_F2 0
#define KEY_F3 0
#define KEY_F4 0
#define KEY_F5 0
#define KEY_F6 0
#define KEY_F7 0
#define KEY_F8 0
#define KEY_F9 0
#define KEY_F10 0
#define KEY_F11 0
#define KEY_F12 0
#define KEY_LEFT_CTRL 0
#define KEY_LEFT_SHIFT 0
#define KEY_LEFT_ALT 0
#define KEY_LEFT_GUI 0
#define KEY_RIGHT_CTRL 0
#define KEY_RIGHT_SHIFT 0
#define KEY_RIGHT_ALT 0
#define KEY_RIGHT_GUI 0
#define KEY_CAPS_LOCK 0
#define KEY_BACKSPACE 0
#define KEY_RETURN 0
#define KEY_INSERT 0
#define KEY_DELETE 0
#define KEY_TAB 0
#define KEY_HOME 0
#define KEY_END 0
#define KEY_PAGE_UP 0
#define KEY_PAGE_DOWN 0
#define KEY_UP_ARROW 0
#define KEY_DOWN_ARROW 0
#define KEY_LEFT_ARROW 0
#define KEY_RIGHT_ARROW 0
#endif

static inline void pinMode(int pin, int mode)
{
  gpio_config_t io_conf = {
    .pin_bit_mask = 1ULL << pin,
    .mode = (mode == INPUT_PULLUP || mode == INPUT) ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT,
    .pull_up_en = (mode == INPUT_PULLUP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static inline int digitalRead(int pin) { return gpio_get_level(static_cast<gpio_num_t>(pin)); }

static inline void digitalWrite(int pin, int level) { gpio_set_level(static_cast<gpio_num_t>(pin), level); }

static inline int digitalPinToInterrupt(int pin) { return pin; }

static void attachInterrupt(int pin, void (*fn)(void), int mode)
{
  gpio_set_intr_type(static_cast<gpio_num_t>(pin), mode == CHANGE ? GPIO_INTR_ANYEDGE : GPIO_INTR_POSEDGE);
  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
  ESP_ERROR_CHECK(gpio_isr_handler_add(static_cast<gpio_num_t>(pin), reinterpret_cast<gpio_isr_t>(fn), nullptr));
}

static inline uint32_t millis() { return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL); }

static inline int analogRead(int pin)
{
  static float phase = 0.0f;
  phase += 0.15f + (pin * 0.01f);
  return static_cast<int>(2048.0f + 1800.0f * std::sin(phase));
}

// EEPROM 兼容层，使用 RAM 缓存，后续可替换成 NVS 持久化
class EEPROMClass {
public:
  void begin(size_t size) { len = size < sizeof(mem) ? size : sizeof(mem); initialized = true; }
  uint8_t read(int addr) const { return (addr >= 0 && addr < static_cast<int>(len)) ? mem[addr] : 0xFF; }
  void write(int addr, uint8_t val) { if (addr >= 0 && addr < static_cast<int>(len)) mem[addr] = val; }
  void commit() {}
  uint8_t *raw() { return mem; }

private:
  bool initialized = false;
  size_t len = 0;
  uint8_t mem[512] = {0xFF};
};

static EEPROMClass EEPROM;


/************************************* 定义页面 *************************************/

//总目录，缩进表示页面层级
enum 
{
  M_WINDOW,
  M_SLEEP,
    M_MAIN, 
      M_EDITOR,
        M_KNOB,
          M_KRF,
          M_KPF,
      M_VOLT,
      M_LIKES,
        M_PID,
          M_PID_EDIT,
      M_COLLECTION,
        M_CHART,
      M_SLOT,
        M_TEXT_EDIT,
      M_SHARE,
      M_SETTING,
        M_ABOUT,
};

//状态，初始化标签
enum
{
  S_FADE,       //转场动画
  S_WINDOW,     //弹窗初始化
  S_LAYER_IN,   //层级初始化
  S_LAYER_OUT,  //层级初始化
  S_NONE,       //直接选择页面
};

//菜单结构体
typedef struct MENU
{
  const char *m_select;
} M_SELECT;

/************************************* 定义内容 *************************************/

/************************************* 文字内容 *************************************/

M_SELECT main_menu[]
{
  {"Sleep"},
  {"Editor"},
  {"Volt"},
  {"Likes"},
  {"Collection"},
  {"Slot"},
  {"Share"},
  {"Setting"},
};

M_SELECT editor_menu[]
{
  {"[ Editor ]"},
  {"- Function 0"},
  {"- Function 1"},
  {"- Function 2"},
  {"- Function 3"},
  {"- Function 4"},
  {"- Function 5"},
  {"- Function 6"},
  {"- Function 7"},
  {"- Function 8"},
  {"- Function 9"},
  {"- Knob"},
};

M_SELECT knob_menu[]
{
  {"[ Knob ]"},
  {"# Rotate Func"},
  {"$ Press Func"},
};

M_SELECT krf_menu[]
{
  {"[ Rotate Function ]"},
  {"--------------------------"},
  {"= Disable"},
  {"--------------------------"},
  {"= Volume"},
  {"= Brightness"},
  {"--------------------------"},
};

M_SELECT kpf_menu[]
{
  {"[ Press Function ]"},
  {"--------------------------"},
  {"= Disable"},
  {"--------------------------"},
  {"= A"},
  {"= B"},
  {"= C"},
  {"= D"},
  {"= E"},
  {"= F"},
  {"= G"},
  {"= H"},
  {"= I"},
  {"= J"},
  {"= K"},
  {"= L"},
  {"= M"},
  {"= N"},
  {"= O"},
  {"= P"},
  {"= Q"},
  {"= R"},
  {"= S"},
  {"= T"},
  {"= U"},
  {"= V"},
  {"= W"},
  {"= X"},
  {"= Y"},
  {"= Z"},
  {"--------------------------"},
  {"= 0"},
  {"= 1"},
  {"= 2"},
  {"= 3"},
  {"= 4"},
  {"= 5"},
  {"= 6"},
  {"= 7"},
  {"= 8"},
  {"= 9"},
  {"--------------------------"},
  {"= Esc"},
  {"= F1"},
  {"= F2"},
  {"= F3"},
  {"= F4"},
  {"= F5"},
  {"= F6"},
  {"= F7"},
  {"= F8"},
  {"= F9"},
  {"= F10"},
  {"= F11"},
  {"= F12"},
  {"--------------------------"},
  {"= Left Ctrl"},
  {"= Left Shift"},
  {"= Left Alt"},
  {"= Left Win"},
  {"= Right Ctrl"},
  {"= Right Shift"},
  {"= Right Alt"},
  {"= Right Win"},
  {"--------------------------"},
  {"= Caps Lock"},
  {"= Backspace"},
  {"= Return"},
  {"= Insert"},
  {"= Delete"},
  {"= Tab"},
  {"--------------------------"},
  {"= Home"},
  {"= End"},
  {"= Page Up"},
  {"= Page Down"},
  {"--------------------------"},
  {"= Up Arrow"},
  {"= Down Arrow"},
  {"= Left Arrow"},
  {"= Right Arrow"},
  {"--------------------------"},
};

M_SELECT volt_menu[]
{
  {"A0"},
  {"A1"},
  {"A2"},
  {"A3"},
  {"A4"},
  {"A5"},
  {"A6"},
  {"A7"},
  {"B0"},
  {"B1"},
};

M_SELECT likes_menu[]
{
  {"[ Likes ]"},
  {"- PID Editor"},
  {"- [ Back ]"},
};

M_SELECT pid_menu[]
{
  {"[ PID Editor ]"},
  {"- Proportion"},
  {"- Integral"},
  {"- Derivative"},
  {"- [ Return ]"},
};

M_SELECT collection_menu[]
{
  {"[ Collection ]"},
  {"- Chart Test"},
  {"- [ Back ]"},
};

M_SELECT slot_menu[]
{
  {"[ Slot ]"},
  {"- Text Edit"},
  {"- [ Back ]"},
};

M_SELECT share_menu[]
{
  {"[ Share ]"},
};

M_SELECT setting_menu[]
{
  {"[ Setting ]"},
  {"~ Disp Bri"},
  {"~ Tile Ani"},
  {"~ List Ani"},
  {"~ Win Ani"},
  {"+ Bar Style"},
  {"~ Tag Ani"},
  {"~ Fade Ani"},
  {"+ T Ufd Fm Scr"},
  {"+ L Ufd Fm Scr"},
  {"+ T Loop Mode"},
  {"+ L Loop Mode"},
  {"+ Win Bokeh Bg"},
  {"+ Knob Rot Dir"},
  {"+ Dark Mode"},
  {"- [ About ]"},
};

M_SELECT about_menu[]
{
  {"[ Z-UI V2.4 ]"},
  {"- CPU: ESP32C3"},
  {"- Flash: 4MB"},
  {"- SRAM: 400KB"},
  {"- Freq: 160Mhz"},
  {"- Creator: ZYoung"},
  {"- Build: 202606"},  
};

/************************************* 图片内容 *************************************/

PROGMEM const uint8_t main_icon_pic[][120]
{
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xF1,0x3F,
    0xFF,0xFF,0xC3,0x3F,0xFF,0xFF,0x87,0x3F,0xFF,0xFF,0x07,0x3F,0xFF,0xFF,0x0F,0x3E,
    0xFF,0xFF,0x0F,0x3E,0xFF,0xFF,0x0F,0x3C,0xFF,0xFF,0x0F,0x3C,0xFF,0xFF,0x0F,0x38,
    0xFF,0xFF,0x0F,0x38,0xFF,0xFF,0x0F,0x38,0xFF,0xFF,0x07,0x38,0xFF,0xFF,0x07,0x38,
    0xFF,0xFF,0x03,0x38,0xF7,0xFF,0x01,0x38,0xE7,0xFF,0x00,0x3C,0x87,0x3F,0x00,0x3C,
    0x0F,0x00,0x00,0x3E,0x0F,0x00,0x00,0x3E,0x1F,0x00,0x00,0x3F,0x3F,0x00,0x80,0x3F,
    0x7F,0x00,0xC0,0x3F,0xFF,0x01,0xF0,0x3F,0xFF,0x07,0xFC,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F
  },
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xF9,0xE7,0x3F,
    0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF0,0xE7,0x3F,0x7F,0xE0,0xE7,0x3F,
    0x7F,0xE0,0xC3,0x3F,0x7F,0xE0,0xC3,0x3F,0x7F,0xE0,0xC3,0x3F,0x7F,0xE0,0xE7,0x3F,
    0xFF,0xF0,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,
    0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xC3,0x3F,0xFF,0xF9,0x81,0x3F,0xFF,0xF0,0x81,0x3F,
    0xFF,0xF0,0x81,0x3F,0xFF,0xF0,0x81,0x3F,0xFF,0xF9,0x81,0x3F,0xFF,0xF9,0xC3,0x3F,
    0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F
  },
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xEF,0xFF,0xFF,0x3F,0xC7,0xFF,0xFF,0x3F,
    0xC7,0xF3,0xFF,0x3F,0x83,0xC0,0xFF,0x3F,0xEF,0xCC,0xFF,0x3F,0x6F,0x9E,0xFF,0x3F,
    0x6F,0x9E,0xFF,0x3F,0x2F,0x3F,0xFF,0x3F,0x2F,0x3F,0xFF,0x3F,0x8F,0x7F,0xFE,0x3F,
    0x8F,0x7F,0xFE,0x39,0x8F,0x7F,0xFE,0x39,0xCF,0xFF,0xFC,0x3C,0xCF,0xFF,0xFC,0x3C,
    0xEF,0xFF,0xFC,0x3C,0xEF,0xFF,0x79,0x3E,0xEF,0xFF,0x79,0x3E,0xEF,0xFF,0x33,0x3F,
    0xEF,0xFF,0x33,0x3F,0xEF,0xFF,0x87,0x3F,0xEF,0xFF,0xCF,0x3F,0xEF,0xFF,0x7F,0x3E,
    0xEF,0xFF,0x7F,0x38,0x0F,0x00,0x00,0x30,0xFF,0xFF,0x7F,0x38,0xFF,0xFF,0x7F,0x3E,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,
  },
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0x1F,0xFE,0x3F,0xFF,0x1F,0xFE,0x3F,0xFF,0x0C,0xCC,0x3F,0x7F,0x00,0x80,0x3F,
    0x3F,0x00,0x00,0x3F,0x3F,0xE0,0x01,0x3F,0x7F,0xF8,0x87,0x3F,0x7F,0xFC,0x8F,0x3F,
    0x3F,0xFC,0x0F,0x3F,0x0F,0x3E,0x1F,0x3C,0x0F,0x1E,0x1E,0x3C,0x0F,0x1E,0x1E,0x3C,
    0x0F,0x3E,0x1F,0x3C,0x3F,0xFC,0x0F,0x3F,0x7F,0xFC,0x8F,0x3F,0x7F,0xF8,0x87,0x3F,
    0x3F,0xE0,0x01,0x3F,0x3F,0x00,0x00,0x3F,0x7F,0x00,0x80,0x3F,0xFF,0x0C,0xCC,0x3F,
    0xFF,0x1F,0xFE,0x3F,0xFF,0x1F,0xFE,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F
  },
};

struct MAIN_ICON
{
  const uint8_t *bmp;
  uint8_t w;
  uint8_t h;
};

PROGMEM const uint8_t likes_icon_pic[] =
{
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0x7F, 0xFE, 0x3F,
  0xFF, 0x7F, 0xFC, 0x3F, 0xFF, 0x7F, 0xF8, 0x3F,
  0xFF, 0x7F, 0xF8, 0x3F, 0xFF, 0x3F, 0xF8, 0x3F,
  0xFF, 0x1F, 0xF8, 0x3F, 0xFF, 0x1F, 0xF8, 0x3F,
  0xFF, 0x0F, 0xF8, 0x3F, 0xFF, 0x07, 0xF8, 0x3F,
  0x9F, 0x03, 0x00, 0x3C, 0x87, 0x03, 0x00, 0x30,
  0x87, 0x03, 0x00, 0x30, 0x83, 0x03, 0x00, 0x30,
  0x83, 0x03, 0x00, 0x30, 0x83, 0x03, 0x00, 0x38,
  0x83, 0x03, 0x00, 0x38, 0x83, 0x03, 0x00, 0x38,
  0x83, 0x03, 0x00, 0x38, 0x83, 0x03, 0x00, 0x3C,
  0x83, 0x03, 0x00, 0x3C, 0x83, 0x03, 0x00, 0x3E,
  0x83, 0x03, 0x00, 0x3E, 0x87, 0x03, 0x00, 0x3F,
  0x8F, 0x03, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
};

/* (30 X 30) */

PROGMEM const uint8_t collection_icon_pic[] =
{
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0x3F, 0xFE, 0x3F,
  0xFF, 0x1F, 0xFE, 0x3F, 0xFF, 0x1F, 0xFE, 0x3F,
  0xFF, 0x0F, 0xFC, 0x3F, 0xFF, 0x0F, 0xFC, 0x3F,
  0xFF, 0x0F, 0xF8, 0x3F, 0xFF, 0x01, 0xE0, 0x3F,
  0x3F, 0x00, 0x00, 0x3E, 0x07, 0x00, 0x00, 0x30,
  0x03, 0x00, 0x00, 0x30, 0x07, 0x00, 0x00, 0x38,
  0x0F, 0x00, 0x00, 0x3C, 0x1F, 0x00, 0x00, 0x3E,
  0x3F, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x3F,
  0xFF, 0x00, 0x80, 0x3F, 0xFF, 0x00, 0x80, 0x3F,
  0xFF, 0x00, 0xC0, 0x3F, 0xFF, 0x00, 0xC0, 0x3F,
  0xFF, 0x00, 0x80, 0x3F, 0xFF, 0x00, 0x80, 0x3F,
  0x7F, 0xE0, 0x81, 0x3F, 0x7F, 0xF8, 0x8F, 0x3F,
  0xFF, 0xFE, 0x9F, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
};

/* (30 X 30) */

PROGMEM const uint8_t slot_icon_pic[] =
{
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0x0F, 0xFF, 0x3F, 0xFF, 0x01, 0xF8, 0x3F,
  0xFF, 0x00, 0xE0, 0x3F, 0x3F, 0x00, 0xC0, 0x3F,
  0x1F, 0xFE, 0x8F, 0x3F, 0x0F, 0xFF, 0x0F, 0x3F,
  0x0F, 0xFF, 0x0F, 0x3E, 0x07, 0x40, 0x00, 0x3E,
  0x07, 0xF0, 0x01, 0x3C, 0x03, 0xFC, 0x03, 0x3C,
  0x03, 0xFE, 0x07, 0x38, 0x03, 0xDE, 0x0F, 0x38,
  0x03, 0x47, 0x1E, 0x38, 0x83, 0x41, 0x3C, 0x38,
  0x83, 0x41, 0x38, 0x38, 0x83, 0x41, 0x38, 0x38,
  0x83, 0x41, 0x38, 0x3C, 0x07, 0x40, 0x00, 0x3C,
  0x07, 0x40, 0x00, 0x3E, 0x0F, 0x40, 0x00, 0x3E,
  0x0F, 0x40, 0x00, 0x3F, 0x1F, 0x40, 0x80, 0x3F,
  0x3F, 0x00, 0xC0, 0x3F, 0xFF, 0x00, 0xE0, 0x3F,
  0xFF, 0x01, 0xF8, 0x3F, 0xFF, 0x1F, 0xFF, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
};

/* (30 X 30) */

PROGMEM const uint8_t share_icon_pic[] =
{
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0x7F, 0xFE, 0x3F, 0xFF, 0x7F, 0xFC, 0x3F,
  0xFF, 0x7F, 0xF8, 0x3F, 0xFF, 0x7F, 0xF0, 0x3F,
  0xFF, 0x7F, 0xC0, 0x3F, 0xFF, 0x7F, 0x80, 0x3F,
  0xFF, 0x07, 0x00, 0x3F, 0xFF, 0x00, 0x00, 0x3E,
  0x3F, 0x00, 0x00, 0x3C, 0x3F, 0x00, 0x00, 0x38,
  0x1F, 0x00, 0x00, 0x30, 0x0F, 0x00, 0x00, 0x30,
  0x0F, 0x00, 0x00, 0x38, 0x07, 0x00, 0x00, 0x3C,
  0x07, 0x00, 0x00, 0x3E, 0x03, 0x7F, 0x00, 0x3F,
  0xC3, 0x7F, 0x80, 0x3F, 0xC3, 0x7F, 0xC0, 0x3F,
  0xF3, 0x7F, 0xF0, 0x3F, 0xFF, 0x7F, 0xFC, 0x3F,
  0xFF, 0x7F, 0xFC, 0x3F, 0xFF, 0x7F, 0xFE, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
  0xFF, 0xFF, 0xFF, 0x3F, 0xFF, 0xFF, 0xFF, 0x3F,
};

/* (30 X 30) */

static const MAIN_ICON main_icons[] =
{
  { main_icon_pic[0], 30, 30 },
  { main_icon_pic[1], 30, 30 },
  { main_icon_pic[2], 30, 30 },
  { likes_icon_pic, 30, 30 },
  { collection_icon_pic, 30, 30 },
  { slot_icon_pic, 30, 30 },
  { share_icon_pic, 30, 30 },
  { main_icon_pic[3], 30, 30 },
};

/************************************* 页面变量 *************************************/

//OLED变量
#define   DISP_H              64    //屏幕高度
#define   DISP_W              128   //屏幕宽度
uint8_t   *buf_ptr;                 //指向屏幕缓冲的指针
uint16_t  buf_len;                  //缓冲长度

//UI变量
#define   UI_DEPTH            20    //最深层级数
#define   UI_MNUMB            100   //菜单数量
#define   UI_PARAM            16    //参数数量
enum 
{
  DISP_BRI,     //屏幕亮度
  TILE_ANI,     //磁贴动画速度
  LIST_ANI,     //列表动画速度
  WIN_ANI,      //弹窗动画速度
  SPOT_ANI,     //列表右侧样式开关
  TAG_ANI,      //标签动画速度
  FADE_ANI,     //消失动画速度
  RESV_0,       //保留: 旧按键短按时长槽位
  RESV_1,       //保留: 旧按键长按时长槽位
  TILE_UFD,     //磁贴图标从头展开开关
  LIST_UFD,     //菜单列表从头展开开关
  TILE_LOOP,    //磁贴图标循环模式开关
  LIST_LOOP,    //菜单列表循环模式开关
  WIN_BOK,      //弹窗背景虚化开关
  KNOB_DIR,     //旋钮方向切换开关
  DARK_MODE,    //黑暗模式开关
};
struct 
{
  bool      init;
  uint8_t   num[UI_MNUMB];
  uint8_t   select[UI_DEPTH];
  uint8_t   layer;
  uint8_t   index = M_SLEEP;
  uint8_t   state = S_NONE;
  bool      sleep = true;
  uint8_t   fade = 1;
  uint8_t   param[UI_PARAM];
} ui;

//磁贴变量
//所有磁贴页面都使用同一套参数
#define   TILE_B_FONT         u8g2_font_helvB18_tr        //磁贴大标题字体
#define   TILE_TITLE_Y        58                          //磁贴标题当前对齐基准
#define   TILE_TITLE_STEP     24                          //磁贴标题列表步进，也就是切换距离
#define   TILE_TITLE_CLIP_BOTTOM_TRIM 1                   //标题裁剪底边内缩 1 像素，避免下边缘残点
#define   TILE_ICON_H         37                          //磁贴图标显示框高度
#define   TILE_ICON_W         36                          //磁贴图标显示框宽度
#define   TILE_ICON_S         48                          //磁贴图标间距
struct 
{
  int16_t temp;
  float   title_scroll;
  float   title_scroll_trg;
  float   icon_x;
  float   icon_x_trg;
  float   icon_y;
  float   icon_y_trg;
} tile;

//列表变量
//默认参数

#define   LIST_FONT           u8g2_font_HelvetiPixel_tr   //列表字体
#define   LIST_TEXT_H         8                           //列表每行文字字体的高度
#define   LIST_LINE_H         16                          //列表单行高度
#define   LIST_TEXT_S         4                           //列表每行文字的上边距，左边距和右边距，下边距由它和字体高度和行高度决定
#define   LIST_BAR_W          5                           //列表进度条宽度，需要是奇数，因为正中间有1像素宽度的线
#define   LIST_BOX_R          0.5f                        //列表选择框圆角

/*
//超窄行高度测试
#define   LIST_FONT           u8g2_font_4x6_tr            //列表字体
#define   LIST_TEXT_H         5                           //列表每行文字字体的高度
#define   LIST_LINE_H         7                           //列表单行高度
#define   LIST_TEXT_S         1                           //列表每行文字的上边距，左边距和右边距，下边距由它和字体高度和行高度决定
#define   LIST_BAR_W          7                           //列表进度条宽度，需要是奇数，因为正中间有1像素宽度的线
#define   LIST_BOX_R          0.5f                        //列表选择框圆角
*/
struct
{
  uint8_t line_n = DISP_H / LIST_LINE_H;
  int16_t temp;
  bool    loop;
  float   y;
  float   y_trg;
  float   box_x;
  float   box_x_trg;
  float   box_y;
  float   box_y_trg[UI_DEPTH];
  float   bar_y;
  float   bar_y_trg;
} list;

//电压测量页面变量
//开发板模拟引脚
uint8_t   analog_pin[10] = { 0, 1, 2, 3, 4, 5, 0, 1, 2, 3 };
//曲线相关
#define   WAVE_W              94                          //波形宽度
#define   WAVE_L              24                          //波形左边距
#define   WAVE_U              0                           //波形上边距
#define   WAVE_MAX            27                          //最大值
#define   WAVE_MIN            4                           //最小值
#define   WAVE_BOX_H          32                          //波形边框高度
#define   WAVE_BOX_W          94                          //波形边框宽度
#define   WAVE_BOX_L_S        24                          //波形边框左边距
//列表和文字背景框相关
#define   VOLT_FONT           u8g2_font_helvB18_tr        //电压数字字体
#define   VOLT_TEXT_BG_L_S    24                          //文字背景框左边距
#define   VOLT_TEXT_BG_W      94                          //文字背景框宽度
#define   VOLT_TEXT_BG_H      29                          //文字背景框高度
struct
{
  int     ch0_adc[WAVE_W];
  int     ch0_wave[WAVE_W];
  int     val;
  float   text_bg_r; 
  float   text_bg_r_trg; 
} volt;

//PID变量
#define   PID_MAX            10.00f
static float  pid_k[3] = { 9.97f, 0.20f, 0.01f };
static uint8_t pid_select = 0;

//图表测试变量
#define   CHART_ADC_PIN      0
static float  angle = 0.0f;
static float  angle_last = 0.0f;
static uint8_t chart_x = 0;
static bool   chart_frame_drawed = false;

//文字编辑变量
static char   text_name[] = "Hello World ";
static constexpr uint8_t TEXT_NAME_LEN = 12;
static uint8_t text_edit_index = 0;
static bool   text_edit_flag = false;
static uint8_t text_blink = 0;
static constexpr uint8_t TEXT_BLINK_SPEED = 16;


//选择框变量

//默认参数
#define   CHECK_BOX_L_S       95                          //选择框在每行的左边距
#define   CHECK_BOX_U_S       2                           //选择框在每行的上边距
#define   CHECK_BOX_F_W       12                          //选择框外框宽度
#define   CHECK_BOX_F_H       12                          //选择框外框高度
#define   CHECK_BOX_D_S       2                           //选择框里面的点距离外框的边距

/*
//超窄行高度测试
#define   CHECK_BOX_L_S       99                          //选择框在每行的左边距
#define   CHECK_BOX_U_S       0                           //选择框在每行的上边距
#define   CHECK_BOX_F_W       5                           //选择框外框宽度
#define   CHECK_BOX_F_H       5                           //选择框外框高度
#define   CHECK_BOX_D_S       1                           //选择框里面的点距离外框的边距
*/
struct
{
  uint8_t *v;
  uint8_t *m;
  uint8_t *s;
  uint8_t *s_p;
} check_box;

//弹窗变量
#define   WIN_FONT            u8g2_font_HelvetiPixel_tr   //弹窗字体
#define   WIN_H               32                          //弹窗高度
#define   WIN_W               102                         //弹窗宽度
#define   WIN_BAR_W           92                          //弹窗进度条宽度
#define   WIN_BAR_H           7                           //弹窗进度条高度
#define   WIN_Y               - WIN_H - 2                 //弹窗竖直方向出场起始位置
#define   WIN_Y_TRG           - WIN_H - 2                 //弹窗竖直方向退场终止位置
struct
{
  //uint8_t
  uint8_t   *value;
  uint8_t   max;
  uint8_t   min;
  uint8_t   step;

  MENU      *bg;
  uint8_t   index;
  char      title[20];
  uint8_t   select;
  uint8_t   l = (DISP_W - WIN_W) / 2;
  uint8_t   u = (DISP_H - WIN_H) / 2;
  float     bar;
  float     bar_trg;
  float     y;
  float     y_trg;
} win;

/********************************** 自定义功能变量 **********************************/

//旋钮功能变量
#define   KNOB_PARAM          4
#define   KNOB_DISABLE        0
#define   KNOB_ROT_VOL        1
#define   KNOB_ROT_BRI        2
enum 
{
  KNOB_ROT,       //睡眠下旋转旋钮的功能，0 禁用，1 音量，2 亮度
  KNOB_COD,       //睡眠下短按旋钮输入的字符码，0 禁用
  KNOB_ROT_P,     //旋转旋钮功能在单选框中选择的位置
  KNOB_COD_P,     //字符码在单选框中选择的位置
};
struct 
{
  uint8_t param[KNOB_PARAM] = { KNOB_DISABLE, KNOB_DISABLE, 2, 2 }; //禁用在列表的第2个选项，第0个是标题，第1个是分界线
} knob;

/************************************* 断电保存 *************************************/

//EEPROM变量
#define   EEPROM_CHECK        11
struct
{
  bool    init;
  bool    change;
  int     address;
  uint8_t check;
  uint8_t check_param[EEPROM_CHECK] = { 'a', 'b', 'c', 'd', 'e', 'f','g', 'h', 'i', 'j', 'k' }; 
} eeprom;

void ui_param_init();

//EEPROM写数据，回到睡眠时执行一遍
void eeprom_write_all_data()
{
  eeprom.address = 0;
  for (uint8_t i = 0; i < EEPROM_CHECK; ++i) {
    EEPROM.write(eeprom.address + i, eeprom.check_param[i]);
  }
  eeprom.address += EEPROM_CHECK;
  for (uint8_t i = 0; i < UI_PARAM; ++i) {
    EEPROM.write(eeprom.address + i, ui.param[i]);
  }
  eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i) {
    EEPROM.write(eeprom.address + i, knob.param[i]);
  }
  eeprom.address += KNOB_PARAM;
}

//EEPROM读数据，开机初始化时执行一遍
void eeprom_read_all_data()
{
  eeprom.address = EEPROM_CHECK;   
  for (uint8_t i = 0; i < UI_PARAM; ++i) {
    ui.param[i] = EEPROM.read(eeprom.address + i);
  }
  ui.param[SPOT_ANI] = ui.param[SPOT_ANI] ? 1 : 0;
  eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i) {
    knob.param[i] = EEPROM.read(eeprom.address + i);
  }
  eeprom.address += KNOB_PARAM;
}

//开机检查是否已经修改过，没修改过则跳过读配置步骤，用默认设置
void eeprom_init()
{
  eeprom.check = 0;
  eeprom.address = 0;
  for (uint8_t i = 0; i < EEPROM_CHECK; ++i) {
    if (EEPROM.read(eeprom.address + i) != eeprom.check_param[i]) {
      eeprom.check ++;
    }
  }
  if (eeprom.check <= 1) eeprom_read_all_data();  //允许一位误码
  else ui_param_init();
}

/************************************* 旋钮相关 *************************************/

//可按下旋钮引脚
#define   AIO       6
#define   BIO       7
#define   SW_OK     9
#define   SW_PUSH   2
#define   SW_BACK   8

//按键ID
#define   BTN_ID_CC           0   //逆时针旋转
#define   BTN_ID_CW           1   //顺时针旋转
#define   BTN_ID_SP           2   //短按/确认
#define   BTN_ID_BK           3   //返回

//按键变量
#define   BTN_PARAM_TIMES     2   //由于uint8_t最大值可能不够，但它存储起来方便，这里放大两倍使用
struct
{
  uint8_t   id;
  bool      flag;
  bool      pressed;
  bool      CW_1;
  bool      CW_2;
  bool      alv;  
  bool      blv;
} volatile btn;

void knob_inter() 
{
  btn.alv = digitalRead(AIO);
  btn.blv = digitalRead(BIO);
  if (!btn.flag && btn.alv == LOW) 
  {
    btn.CW_1 = btn.blv;
    btn.flag = true;
  }
  if (btn.flag && btn.alv) 
  {
    btn.CW_2 = !btn.blv;
    if (btn.CW_1 && btn.CW_2)
     {
      btn.id = !ui.param[KNOB_DIR];
      btn.pressed = true;
    }
    if (btn.CW_1 == false && btn.CW_2 == false) 
    {
      btn.id = ui.param[KNOB_DIR];
      btn.pressed = true;
    }
    btn.flag = false;
  }
}

void btn_scan() 
{
  if (btn.pressed) {
    return;
  }

  struct ButtonState {
    uint8_t pin;
    uint8_t id;
    bool last_raw;
    bool active;
    uint32_t change_ms;
  };

  static ButtonState buttons[] = {
    {SW_OK,   BTN_ID_SP, HIGH, false, 0},
    {SW_PUSH, BTN_ID_SP, HIGH, false, 0},
    {SW_BACK, BTN_ID_BK, HIGH, false, 0},
  };
  constexpr uint32_t kDebounceMs = 4;
  const uint32_t now = millis();

  for (auto &button : buttons)
  {
    const bool raw = digitalRead(button.pin);
    if (raw != button.last_raw)
    {
      button.last_raw = raw;
      button.change_ms = now;
      if (raw == HIGH)
      {
        button.active = false;
      }
    }

    if (!button.active && raw == LOW && (now - button.change_ms) >= kDebounceMs)
    {
      button.active = true;
      btn.id = button.id;
      btn.pressed = true;
      break;
    }
  }
}

void btn_init() 
{
  pinMode(AIO, INPUT);
  pinMode(BIO, INPUT);
  pinMode(SW_OK, INPUT_PULLUP);
  pinMode(SW_PUSH, INPUT_PULLUP);
  pinMode(SW_BACK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(AIO), knob_inter, CHANGE);
}

/************************************ 初始化函数 ***********************************/

/********************************* 初始化数据处理函数 *******************************/

//显示数值的初始化
void check_box_v_init(uint8_t *param)
{
  check_box.v = param;
}

//多选框的初始化
void check_box_m_init(uint8_t *param)
{
  check_box.m = param;
}

//单选框时的初始化
void check_box_s_init(uint8_t *param, uint8_t *param_p)
{
  check_box.s = param;
  check_box.s_p = param_p;
}

//多选框处理函数
void check_box_m_select(uint8_t param)
{
  check_box.m[param] = !check_box.m[param];
  eeprom.change = true;
}

static int setting_toggle_param_index(uint8_t row)
{
  switch (row)
  {
    case 5:  return SPOT_ANI;
    case 8:  return TILE_UFD;
    case 9:  return LIST_UFD;
    case 10: return TILE_LOOP;
    case 11: return LIST_LOOP;
    case 12: return WIN_BOK;
    case 13: return KNOB_DIR;
    case 14: return DARK_MODE;
    default: return -1;
  }
}

//单选框处理函数
void check_box_s_select(uint8_t val, uint8_t pos)
{
  *check_box.s = val;
  *check_box.s_p = pos;
  eeprom.change = true;
}

//弹窗数值初始化
void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, MENU *bg, uint8_t index)
{
  strcpy(win.title, title);
  win.select = select;
  win.value = value;
  win.max = max;
  win.min = min;
  win.step = step;
  win.bg = bg;
  win.index = index;  
  ui.index = M_WINDOW;
  ui.state = S_WINDOW;
}

/*********************************** UI 初始化函数 *********************************/

//在初始化EEPROM时，选择性初始化的默认设置
void ui_param_init()
{
  ui.param[DISP_BRI]  = 255;      //屏幕亮度
  ui.param[TILE_ANI]  = 30;       //磁贴动画速度
  ui.param[LIST_ANI]  = 60;       //列表动画速度
  ui.param[WIN_ANI]   = 25;       //弹窗动画速度
  ui.param[SPOT_ANI]  = 0;        //列表右侧样式开关，0=当前样式，1=sketch风格
  ui.param[TAG_ANI]   = 60;       //标签动画速度
  ui.param[FADE_ANI]  = 30;       //消失动画速度
  ui.param[RESV_0]    = 0;        //保留
  ui.param[RESV_1]    = 0;        //保留
  ui.param[TILE_UFD]  = 1;        //磁贴图标从头展开开关
  ui.param[LIST_UFD]  = 1;        //菜单列表从头展开开关
  ui.param[TILE_LOOP] = 0;        //磁贴图标循环模式开关
  ui.param[LIST_LOOP] = 0;        //菜单列表循环模式开关
  ui.param[WIN_BOK]   = 0;        //弹窗背景虚化开关
  ui.param[KNOB_DIR]  = 0;        //旋钮方向切换开关   
  ui.param[DARK_MODE] = 1;        //黑暗模式开关   
}

//列表类页面列表行数初始化，必须初始化的参数
void ui_init()
{
  ui.num[M_MAIN]      = sizeof( main_menu     )   / sizeof(M_SELECT);
  ui.num[M_EDITOR]    = sizeof( editor_menu   )   / sizeof(M_SELECT);
  ui.num[M_KNOB]      = sizeof( knob_menu     )   / sizeof(M_SELECT);
  ui.num[M_KRF]       = sizeof( krf_menu      )   / sizeof(M_SELECT);
  ui.num[M_KPF]       = sizeof( kpf_menu      )   / sizeof(M_SELECT);
  ui.num[M_VOLT]      = sizeof( volt_menu     )   / sizeof(M_SELECT);
  ui.num[M_LIKES]     = sizeof( likes_menu    )   / sizeof(M_SELECT);
  ui.num[M_PID]       = sizeof( pid_menu      )   / sizeof(M_SELECT);
  ui.num[M_PID_EDIT]  = 1;
  ui.num[M_COLLECTION]= sizeof( collection_menu ) / sizeof(M_SELECT);
  ui.num[M_CHART]     = 1;
  ui.num[M_SLOT]      = sizeof( slot_menu     )   / sizeof(M_SELECT);
  ui.num[M_TEXT_EDIT] = 1;
  ui.num[M_SHARE]     = sizeof( share_menu    )   / sizeof(M_SELECT);
  ui.num[M_SETTING]   = sizeof( setting_menu  )   / sizeof(M_SELECT);
  ui.num[M_ABOUT]     = sizeof( about_menu    )   / sizeof(M_SELECT);   
}

/********************************* 分页面初始化函数 ********************************/

//进入磁贴类时的初始化
void tile_param_init()
{
  ui.init = false;
  tile.title_scroll = ui.select[ui.layer] * TILE_TITLE_STEP;
  tile.title_scroll_trg = tile.title_scroll;
  tile.icon_x = 0;
  tile.icon_x_trg = TILE_ICON_S;
  tile.icon_y = -TILE_ICON_H;
  tile.icon_y_trg = 0;
}

//磁贴标题切换初始化
void tile_title_transition(uint8_t to)
{
  tile.title_scroll_trg = to * TILE_TITLE_STEP;
}

//进入睡眠时的初始化
void sleep_param_init()
{
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, DISP_W, DISP_H);
  u8g2.setPowerSave(1);
  ui.state = S_NONE;  
  ui.sleep = true;
  if (eeprom.change)
  {
    eeprom_write_all_data();
    eeprom.change = false;
  }
}

//旋钮设置页初始化
void knob_param_init()
{
  check_box_v_init(knob.param);
  ui.select[ui.layer] = 1;
}

//旋钮旋转页初始化
void krf_param_init()
{
  check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]);
  ui.select[ui.layer] = knob.param[KNOB_ROT_P];
}

//旋钮点按页初始化
void kpf_param_init()
{
  check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]);
  ui.select[ui.layer] = knob.param[KNOB_COD_P];
}

//电压测量页初始化
void volt_param_init()
{
  volt.text_bg_r = 0;
  volt.text_bg_r_trg = VOLT_TEXT_BG_W; 
}

//设置页初始化
void setting_param_init()
{
  check_box_v_init(ui.param);
  check_box_m_init(ui.param);
  ui.select[ui.layer] = 1;
}

/********************************** 通用初始化函数 *********************************/

/*
  页面层级管理逻辑是，把所有页面都先当作列表类初始化，不是列表类按需求再初始化对应函数
  这样做会浪费一些资源，但跳转页面时只需要考虑页面层级，逻辑上更清晰，减少出错
*/

//弹窗动画初始化
void window_param_init()
{
  win.bar = 0;
  win.y = WIN_Y;
  win.y_trg = win.u;
  ui.state = S_NONE;
}

//进入更深层级时的初始化
void layer_init_in()
{
  ui.layer ++;
  ui.init = 0;
  list.y = 0;
  list.y_trg = LIST_LINE_H;
  list.box_x = 0;
  list.box_y = 0;
  list.box_y_trg[ui.layer] = 0;
  list.bar_y = 0;
  ui.state = S_FADE;
  switch (ui.index)
  {
    case M_MAIN:    tile_param_init();    break;  //睡眠进入主菜单，动画初始化   
    case M_EDITOR:  ui.select[ui.layer] = 11; break;  //编辑器页默认落到 Knob
    case M_KNOB:    knob_param_init();    break;  //旋钮设置页，行末尾文字初始化
    case M_KRF:     krf_param_init();     break;  //旋钮旋转页，单选框初始化  
    case M_KPF:     kpf_param_init();     break;  //旋钮点按页，单选框初始化  
    case M_VOLT:    volt_param_init();    break;  //主菜单进入电压测量页，动画初始化
    case M_LIKES:
    case M_COLLECTION:
    case M_SLOT:
      ui.select[ui.layer] = 1;
      break;
    case M_PID:
      ui.select[ui.layer] = 1;
      break;
    case M_PID_EDIT:
      pid_select = ui.select[ui.layer - 1] > 0 ? ui.select[ui.layer - 1] - 1 : 0;
      ui.select[ui.layer] = 0;
      break;
    case M_CHART:
      chart_x = 0;
      angle = 0.0f;
      angle_last = 0.0f;
      chart_frame_drawed = false;
      break;
    case M_TEXT_EDIT:
      text_edit_index = 0;
      text_edit_flag = false;
      text_blink = 0;
      break;
    case M_SHARE:
      ui.select[ui.layer] = 0;
      break;
    case M_SETTING: setting_param_init(); break;  //主菜单进入设置页，单选框初始化
  }
}

//进入更浅层级时的初始化
void layer_init_out()
{
  ui.select[ui.layer] = 0;
  list.box_y_trg[ui.layer] = 0;
  ui.layer --;
  ui.init = 0;
  list.y = 0;
  list.y_trg = LIST_LINE_H;
  list.bar_y = 0;
  ui.state = S_FADE;
  switch (ui.index)
  {
    case M_SLEEP: sleep_param_init(); break;    //主菜单进入睡眠页，检查是否需要写EEPROM
    case M_MAIN:  tile_param_init();  break;    //不管什么页面进入主菜单时，动画初始化
  }
}

/************************************* 动画函数 *************************************/

//动画函数
void animation(float *a, float *a_trg, uint8_t n)
{
  if (*a != *a_trg)
  {
    if (fabs(*a - *a_trg) < 0.15f) *a = *a_trg;
    else *a += (*a_trg - *a) / (ui.param[n] / 10.0f);
  }
}

//消失函数
void fade()
{
  delay(ui.param[FADE_ANI]);
  if (ui.param[DARK_MODE])
  {
    switch (ui.fade)
    {
      case 1: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA; break;
      case 2: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
      case 3: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55; break;
      case 4: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
      default: ui.state = S_NONE; ui.fade = 0; break;
    }
  }
  else
  {
    switch (ui.fade)
    {
      case 1: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0xAA; break;
      case 2: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0x00; break;
      case 3: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x55; break;
      case 4: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x00; break;
      default: ui.state = S_NONE; ui.fade = 0; break;
    }    
  }
  ui.fade++;
}

/************************************* 显示函数 *************************************/

//绘制主菜单标题
void tile_draw_title(struct MENU arr_1[], uint8_t idx, int16_t y)
{
  const char *title = arr_1[idx].m_select;
  u8g2.drawStr((DISP_W - u8g2.getStrWidth(title)) / 2, y, title);
}

//磁贴类页面通用显示函数
void tile_show(struct MENU arr_1[], const MAIN_ICON icon_pic[])
{
  //计算动画过渡值
  animation(&tile.icon_x, &tile.icon_x_trg, TILE_ANI);
  animation(&tile.icon_y, &tile.icon_y_trg, TILE_ANI);
  animation(&tile.title_scroll, &tile.title_scroll_trg, TILE_ANI);

  //设置大标题的颜色，0透显，1实显，2反色，这里用实显
  u8g2.setDrawColor(1);

  u8g2.setFont(TILE_B_FONT); 
  const int16_t title_clip_y0 = TILE_TITLE_Y - u8g2.getAscent();
  const int16_t title_clip_y1 = TILE_TITLE_Y - u8g2.getDescent() - TILE_TITLE_CLIP_BOTTOM_TRIM;
  //标题只在自己的单行带里运动，避免上下两行同时露出
  u8g2.setClipWindow(0, title_clip_y0, DISP_W, title_clip_y1);
  //参考原始 sketch.ino 的做法：把标题当作列表项滚动，当前项始终对齐在基准位
  for (uint8_t i = 0; i < ui.num[ui.index]; ++i)
  {
    const int16_t title_y = TILE_TITLE_Y + (int16_t)i * TILE_TITLE_STEP - (int16_t)lroundf(tile.title_scroll);
    tile_draw_title(arr_1, i, title_y);
  }
  u8g2.setMaxClipWindow();

  //绘制图标
  if (!ui.init)
  {
    for (uint8_t i = 0; i < ui.num[ui.index]; ++i)  
    {
      const uint8_t icon_w = icon_pic[i].w;
      const uint8_t icon_h = icon_pic[i].h;
      const int16_t icon_x = ui.param[TILE_UFD]
        ? (DISP_W - TILE_ICON_W) / 2 + i * tile.icon_x - TILE_ICON_S * ui.select[ui.layer]
        : (DISP_W - TILE_ICON_W) / 2 + (i - ui.select[ui.layer]) * tile.icon_x;
      const int16_t icon_y = (int16_t)tile.icon_y;
      const int16_t draw_x = icon_x + (TILE_ICON_W - icon_w) / 2;
      const int16_t draw_y = icon_y + (TILE_ICON_H - icon_h) / 2;
      const int16_t clip_x0 = icon_x < 0 ? 0 : icon_x;
      const int16_t clip_y0 = icon_y < 0 ? 0 : icon_y;
      const int16_t clip_x1 = icon_x + TILE_ICON_W > DISP_W ? DISP_W : icon_x + TILE_ICON_W;
      const int16_t clip_y1 = icon_y + TILE_ICON_H > DISP_H ? DISP_H : icon_y + TILE_ICON_H;
      u8g2.setClipWindow(clip_x0, clip_y0, clip_x1, clip_y1);
      u8g2.drawXBMP(draw_x, draw_y, icon_w, icon_h, icon_pic[i].bmp); 
      u8g2.setMaxClipWindow();
    }
    if (tile.icon_x == tile.icon_x_trg) 
    {
      ui.init = true;
      tile.icon_x = tile.icon_x_trg = - ui.select[ui.layer] * TILE_ICON_S;
    }
  }
  else for (uint8_t i = 0; i < ui.num[ui.index]; ++i)
  {
    const uint8_t icon_w = icon_pic[i].w;
    const uint8_t icon_h = icon_pic[i].h;
    const int16_t icon_x = (DISP_W - TILE_ICON_W) / 2 + (int16_t)tile.icon_x + i * TILE_ICON_S;
    const int16_t icon_y = 0;
    const int16_t draw_x = icon_x + (TILE_ICON_W - icon_w) / 2;
    const int16_t draw_y = (TILE_ICON_H - icon_h) / 2;
    const int16_t clip_x0 = icon_x < 0 ? 0 : icon_x;
    const int16_t clip_y0 = icon_y < 0 ? 0 : icon_y;
    const int16_t clip_x1 = icon_x + TILE_ICON_W > DISP_W ? DISP_W : icon_x + TILE_ICON_W;
    const int16_t clip_y1 = icon_y + TILE_ICON_H > DISP_H ? DISP_H : icon_y + TILE_ICON_H;
    u8g2.setClipWindow(clip_x0, clip_y0, clip_x1, clip_y1);
    u8g2.drawXBMP(draw_x, draw_y, icon_w, icon_h, icon_pic[i].bmp);
    u8g2.setMaxClipWindow();
  }

  //反转屏幕内元素颜色，白天模式遮罩
  u8g2.setDrawColor(2);
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

/*************** 根据列表每行开头符号，判断每行尾部是否绘制以及绘制什么内容 *************/

//列表显示数值
void list_draw_value(int n) { u8g2.print(check_box.v[n - 1]); }

//绘制外框
void list_draw_check_box_frame() { u8g2.drawRFrame(CHECK_BOX_L_S, list.temp + CHECK_BOX_U_S, CHECK_BOX_F_W, CHECK_BOX_F_H, 1); }

//绘制框里面的点
void list_draw_check_box_dot() { u8g2.drawBox(CHECK_BOX_L_S + CHECK_BOX_D_S + 1, list.temp + CHECK_BOX_U_S + CHECK_BOX_D_S + 1, CHECK_BOX_F_W - (CHECK_BOX_D_S + 1) * 2, CHECK_BOX_F_H - (CHECK_BOX_D_S + 1) * 2); }

//列表显示旋钮功能
void list_draw_krf(int n) 
{ 
  switch (check_box.v[n - 1])
  {
    case 0: u8g2.print("OFF"); break;
    case 1: u8g2.print("VOL"); break;
    case 2: u8g2.print("BRI"); break;
  }
}

//列表显示按键键值
void list_draw_kpf(int n) 
{ 
  if (check_box.v[n - 1] == 0) u8g2.print("OFF");
  else if (check_box.v[n - 1] <= 90) u8g2.print((char)check_box.v[n - 1]);
  else u8g2.print("?");
}

static void list_draw_progress_bar(uint8_t item_count)
{
  if (item_count <= 1) return;

  if (ui.param[SPOT_ANI])
  {
    const int16_t rail_x = DISP_W - 2;
    const int16_t thumb_h = LIST_LINE_H - 2;
    int16_t thumb_y = static_cast<int16_t>(lroundf(list.bar_y)) - thumb_h / 2;
    if (thumb_y < 0) thumb_y = 0;
    if (thumb_y > DISP_H - thumb_h) thumb_y = DISP_H - thumb_h;

    u8g2.drawVLine(rail_x, 0, DISP_H);
    u8g2.drawPixel(rail_x - 1, 0);
    u8g2.drawPixel(rail_x + 1, 0);
    u8g2.drawPixel(rail_x - 1, DISP_H - 1);
    u8g2.drawPixel(rail_x + 1, DISP_H - 1);
    u8g2.drawVLine(rail_x - 1, thumb_y, thumb_h);
    u8g2.drawVLine(rail_x + 1, thumb_y, thumb_h);
    return;
  }

  u8g2.drawHLine(DISP_W - LIST_BAR_W, 0, LIST_BAR_W);
  u8g2.drawHLine(DISP_W - LIST_BAR_W, DISP_H - 1, LIST_BAR_W);
  u8g2.drawVLine(DISP_W - ceil((float)LIST_BAR_W / 2), 0, DISP_H);
  u8g2.drawBox(DISP_W - LIST_BAR_W, 0, LIST_BAR_W, list.bar_y);
}

//判断列表尾部内容
void list_draw_text_and_check_box(struct MENU arr[], int i)
{
  u8g2.drawStr(LIST_TEXT_S, list.temp + LIST_TEXT_H + LIST_TEXT_S, arr[i].m_select);
  u8g2.setCursor(CHECK_BOX_L_S, list.temp + LIST_TEXT_H + LIST_TEXT_S);
  switch (arr[i].m_select[0])
  {
    case '~': list_draw_value(i); break;
    case '+':
      list_draw_check_box_frame();
      if (arr == setting_menu)
      {
        const int param_idx = setting_toggle_param_index(i);
        if (param_idx >= 0 && check_box.m[param_idx] == 1) list_draw_check_box_dot();
      }
      else if (check_box.m[i + 1] == 1) list_draw_check_box_dot();
      break;
    case '=': list_draw_check_box_frame(); if (*check_box.s_p == i)      list_draw_check_box_dot(); break;
    case '#': list_draw_krf(i);   break;
    case '$': list_draw_kpf(i);   break;
  }
}

/******************************** 列表显示函数 **************************************/

//列表类页面通用显示函数
void list_show(struct MENU arr[], uint8_t ui_index)
{
  //更新动画目标值
  u8g2.setFont(LIST_FONT);
  list.box_x_trg = u8g2.getStrWidth(arr[ui.select[ui.layer]].m_select) + LIST_TEXT_S * 2;
  const uint8_t item_count = ui.num[ui_index];
  list.bar_y_trg = (item_count > 1) ? ceil((ui.select[ui.layer]) * ((float)DISP_H / (item_count - 1))) : 0;
  
  //计算动画过渡值
  animation(&list.y, &list.y_trg, LIST_ANI);
  animation(&list.box_x, &list.box_x_trg, LIST_ANI);
  animation(&list.box_y, &list.box_y_trg[ui.layer], LIST_ANI);
  animation(&list.bar_y, &list.bar_y_trg, LIST_ANI);

  //检查循环动画是否结束
  if (list.loop && list.box_y == list.box_y_trg[ui.layer]) list.loop = false;

  //设置文字和进度条颜色，0透显，1实显，2反色，这里都用实显
  u8g2.setDrawColor(1);
  
  //绘制进度条
  list_draw_progress_bar(item_count);

  //绘制列表文字  
  if (!ui.init)
  {
    for (int i = 0; i < ui.num[ui_index]; ++ i)
    {
      if (ui.param[LIST_UFD]) list.temp = i * list.y - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
      else list.temp = (i - ui.select[ui.layer]) * list.y + list.box_y_trg[ui.layer];
      list_draw_text_and_check_box(arr, i);
    }
    if (list.y == list.y_trg) 
    {
      ui.init = true;
      list.y = list.y_trg = - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
    }
  }
  else for (int i = 0; i < ui.num[ui_index]; ++ i)
  {
    list.temp = LIST_LINE_H * i + list.y;
    list_draw_text_and_check_box(arr, i);
  }

  //绘制文字选择框，0透显，1实显，2反色，这里用反色
  u8g2.setDrawColor(2);
  u8g2.drawRBox(0, list.box_y, list.box_x, LIST_LINE_H, LIST_BOX_R);

  //反转屏幕内元素颜色，白天模式遮罩，在这里屏蔽有列表参与的页面，使遮罩作用在那个页面上
  if (!ui.param[DARK_MODE])
  {
    u8g2.drawBox(0, 0, DISP_W, DISP_H);
    switch(ui.index)
    {
      case M_WINDOW: 
      case M_VOLT:
      u8g2.drawBox(0, 0, DISP_W, DISP_H);  
    }
  }
}

//电压页面显示函数
void volt_show()
{
  //使用列表类显示选项
  list_show(volt_menu, M_VOLT); 

  //计算动画过渡值  
  animation(&volt.text_bg_r, &volt.text_bg_r_trg, TAG_ANI);

  //设置曲线颜色，0透显，1实显，2反色，这里用实显
  u8g2.setDrawColor(1);  

  //绘制电压曲线和外框
  volt.val = 0;
  u8g2.drawFrame(WAVE_BOX_L_S, 0, WAVE_BOX_W, WAVE_BOX_H);
  u8g2.drawFrame(WAVE_BOX_L_S + 1, 1, WAVE_BOX_W - 2, WAVE_BOX_H - 2);
  if (list.box_y == list.box_y_trg[ui.layer] && list.y == list.y_trg)
  {
    int sample_sum = 0;
    for (int i = 0; i < WAVE_W; i++)
    {
      volt.ch0_adc[i] = analogRead(analog_pin[ui.select[ui.layer]]);
      sample_sum += volt.ch0_adc[i];
    }
    volt.val = sample_sum / WAVE_W;
    volt.ch0_wave[0] = map(volt.ch0_adc[0], 0, 4095, WAVE_MAX, WAVE_MIN);
    for (int i = 1; i < WAVE_W; i++)
    {
      volt.ch0_wave[i] = map(volt.ch0_adc[i], 0, 4095, WAVE_MAX, WAVE_MIN);
      u8g2.drawLine(WAVE_L + i - 1, WAVE_U + volt.ch0_wave[i - 1], WAVE_L + i, WAVE_U + volt.ch0_wave[i]);
    }
  }

  //绘制电压值
  u8g2.setFontDirection(0);
  u8g2.setFont(VOLT_FONT); 
  u8g2.setCursor(39, DISP_H - 6);
  u8g2.print(volt.val / 4096.0f * 3.3f);
  u8g2.print("V");

  //绘制列表选择框和电压文字背景
  u8g2.setDrawColor(2);
  u8g2.drawBox(VOLT_TEXT_BG_L_S, DISP_H - VOLT_TEXT_BG_H, volt.text_bg_r, VOLT_TEXT_BG_H);

  //反转屏幕内元素颜色，白天模式遮罩
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

//弹窗通用显示函数
void window_show()
{
  //绘制背景列表，根据开关判断背景是否要虚化
  list_show(win.bg, win.index);
  if (ui.param[WIN_BOK]) for (uint16_t i = 0; i < buf_len; ++i)  buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);

  //更新动画目标值
  u8g2.setFont(WIN_FONT);
  win.bar_trg = (float)(*win.value - win.min) / (float)(win.max - win.min) * (WIN_BAR_W - 4);

  //计算动画过渡值
  animation(&win.bar, &win.bar_trg, WIN_ANI);
  animation(&win.y, &win.y_trg, WIN_ANI);

  //绘制窗口
  u8g2.setDrawColor(0); u8g2.drawRBox(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);    //绘制外框背景
  u8g2.setDrawColor(1); u8g2.drawRFrame(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);  //绘制外框描边
  u8g2.drawRFrame(win.l + 5, (int16_t)win.y + 20, WIN_BAR_W, WIN_BAR_H, 1);       //绘制进度条外框
  u8g2.drawBox(win.l + 7, (int16_t)win.y + 22, win.bar, WIN_BAR_H - 4);           //绘制进度条
  u8g2.setCursor(win.l + 5, (int16_t)win.y + 14); u8g2.print(win.title);          //绘制标题
  u8g2.setCursor(win.l + 78, (int16_t)win.y + 14); u8g2.print(*win.value);        //绘制当前值
  
  //需要在窗口修改参数时立即见效的函数
  if (!strcmp(win.title, "Disp Bri")) u8g2.setContrast(ui.param[DISP_BRI]);

  //反转屏幕内元素颜色，白天模式遮罩
  u8g2.setDrawColor(2);
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

/************************************* 处理函数 *************************************/

/*********************************** 通用处理函数 ***********************************/

//磁贴类页面旋转时判断通用函数
void tile_rotate_switch()
{
  switch (btn.id)
  { 
    case BTN_ID_CC:
      if (ui.init)
      {
        if (ui.select[ui.layer] > 0)
        {
          ui.select[ui.layer] -= 1;
          tile.icon_x_trg += TILE_ICON_S;
          tile_title_transition(ui.select[ui.layer]);
        }
        else 
        {
          if (ui.param[TILE_LOOP])
          {
            ui.select[ui.layer] = ui.num[ui.index] - 1;
            tile.icon_x_trg = - TILE_ICON_S * (ui.num[ui.index] - 1);
            tile_title_transition(ui.select[ui.layer]);
            break;
          }
        }
      }
      break;

    case BTN_ID_CW:
      if (ui.init)
      {
        if (ui.select[ui.layer] < (ui.num[ui.index] - 1)) 
        {
          ui.select[ui.layer] += 1;
          tile.icon_x_trg -= TILE_ICON_S;
          tile_title_transition(ui.select[ui.layer]);
        }
        else 
        {
          if (ui.param[TILE_LOOP])
          {
            ui.select[ui.layer] = 0;
            tile.icon_x_trg = 0;
            tile_title_transition(ui.select[ui.layer]);
            break;
          }
        }
      }
      break;
  }
}

//列表类页面旋转时判断通用函数
void list_rotate_switch()
{
  if (!list.loop)
  {
    switch (btn.id)
    {
      case BTN_ID_CC:
        if (ui.select[ui.layer] == 0)
        {
          if (ui.param[LIST_LOOP] && ui.init)
          {
            list.loop = true;
            ui.select[ui.layer] = ui.num[ui.index] - 1;
            if (ui.num[ui.index] > list.line_n) 
            {
              list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              list.y_trg = DISP_H - ui.num[ui.index] * LIST_LINE_H;
            }
            else list.box_y_trg[ui.layer] = (ui.num[ui.index] - 1) * LIST_LINE_H;
            break;
          }
          else break;
        }
        if (ui.init)
        {
          ui.select[ui.layer] -= 1;
          if (ui.select[ui.layer] < - (list.y_trg / LIST_LINE_H)) 
          {
            if (!(DISP_H % LIST_LINE_H)) list.y_trg += LIST_LINE_H;
            else
            {
              if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * list.line_n)
              {
                list.y_trg += (list.line_n + 1) * LIST_LINE_H - DISP_H;
                list.box_y_trg[ui.layer] = 0;
              }
              else if (list.box_y_trg[ui.layer] == LIST_LINE_H)
              {
                list.box_y_trg[ui.layer] = 0;
              }
              else list.y_trg += LIST_LINE_H;
            }
          }
          else list.box_y_trg[ui.layer] -= LIST_LINE_H;
          break;
        }

      case BTN_ID_CW:
        if (ui.select[ui.layer] == (ui.num[ui.index] - 1))
        {
          if (ui.param[LIST_LOOP] && ui.init)
          {
            list.loop = true;
            ui.select[ui.layer] = 0;
            list.y_trg = 0;
            list.box_y_trg[ui.layer] = 0;
            break;
          }
          else break;
        }
        if (ui.init)
        {
          ui.select[ui.layer] += 1;
          if ((ui.select[ui.layer] + 1) > (list.line_n - list.y_trg / LIST_LINE_H))
          {
            if (!(DISP_H % LIST_LINE_H)) list.y_trg -= LIST_LINE_H;
            else
            {
              if (list.box_y_trg[ui.layer] == LIST_LINE_H * (list.line_n - 1))
              {
                list.y_trg -= (list.line_n + 1) * LIST_LINE_H - DISP_H;
                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              }
              else if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * 2)
              {
                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              }
              else list.y_trg -= LIST_LINE_H;
            }
          }
          else list.box_y_trg[ui.layer] += LIST_LINE_H;
          break;
        }
        break;
    }
  }
}

//弹窗通用处理函数
void window_proc()
{
  window_show();
  if (win.y == WIN_Y_TRG) ui.index = win.index;
  if (btn.pressed && win.y == win.y_trg && win.y != WIN_Y_TRG)
  {
    btn.pressed = false;
    switch (btn.id)
    {
      case BTN_ID_CW: if (*win.value < win.max)  *win.value += win.step;  eeprom.change = true;  break;
      case BTN_ID_CC: if (*win.value > win.min)  *win.value -= win.step;  eeprom.change = true;  break;  
      case BTN_ID_SP:
      case BTN_ID_BK:
        win.y_trg = WIN_Y_TRG;
        break;
    }
  }
}

/********************************** 分页面处理函数 **********************************/

//睡眠页面处理函数
void sleep_proc()
{
  while (ui.sleep)
  {
    //睡眠时循环执行的函数

    //睡眠时轮询输入，按下即可唤醒
    btn_scan();

    //当旋钮有动作时
    if (btn.pressed) { btn.pressed = false; switch (btn.id) {    

        //睡眠时顺时针旋转执行的函数
        case BTN_ID_CW:
          switch (knob.param[KNOB_ROT])
          {
            case KNOB_ROT_VOL: break;
            case KNOB_ROT_BRI: break;
          }
          break;

        //睡眠时逆时针旋转执行的函数
        case BTN_ID_CC:
          switch (knob.param[KNOB_ROT])
          {
            case KNOB_ROT_VOL: break;
            case KNOB_ROT_BRI: break;
          }
          break;

        //睡眠时按键执行的函数
        case BTN_ID_SP:
        case BTN_ID_BK:
          ui.index = M_MAIN;
          ui.state = S_LAYER_IN;
          u8g2.setPowerSave(0);
          ui.sleep = false;
          break;
        
        //旋转仍然先保留为空动作，避免睡眠误触亮屏
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

//主菜单处理函数，磁贴类模板
void main_proc()
{
  tile_show(main_menu, main_icons);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: tile_rotate_switch(); break; case BTN_ID_SP: switch (ui.select[ui.layer]) {

        case 0: ui.index = M_SLEEP;   ui.state = S_LAYER_OUT; break;
        case 1: ui.index = M_EDITOR;  ui.state = S_LAYER_IN;  break;
        case 2: ui.index = M_VOLT;    ui.state = S_LAYER_IN;  break;
        case 3: ui.index = M_PID;     ui.state = S_LAYER_IN;  break;
        case 4: ui.index = M_CHART;   ui.state = S_LAYER_IN;  break;
        case 5: ui.index = M_TEXT_EDIT;ui.state = S_LAYER_IN;  break;
        case 6: ui.index = M_SHARE;   ui.state = S_LAYER_IN;  break;
        case 7: ui.index = M_SETTING;      ui.state = S_LAYER_IN;  break;
      }
    }
  }
}

static void blank_page_proc(M_SELECT arr[], uint8_t page_index)
{
  list_show(arr, page_index);
  if (!btn.pressed) return;
  btn.pressed = false;
  switch (btn.id)
  {
    case BTN_ID_SP:
    case BTN_ID_BK:
      ui.index = M_MAIN;
      ui.state = S_LAYER_OUT;
      break;
    default:
      break;
  }
}

static void direct_menu_proc(M_SELECT arr[], uint8_t page_index, uint8_t child_page)
{
  list_show(arr, page_index);
  if (!btn.pressed) return;
  btn.pressed = false;
  switch (btn.id)
  {
    case BTN_ID_CW:
    case BTN_ID_CC:
      list_rotate_switch();
      break;
    case BTN_ID_BK:
      ui.select[ui.layer] = 0;
      ui.index = M_MAIN;
      ui.state = S_LAYER_OUT;
      break;
    case BTN_ID_SP:
      switch (ui.select[ui.layer])
      {
        case 1:
          ui.index = child_page;
          ui.state = S_LAYER_IN;
          break;
        case 2:
          ui.select[ui.layer] = 0;
          ui.index = M_MAIN;
          ui.state = S_LAYER_OUT;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

static const char *pid_title_for(uint8_t idx)
{
  switch (idx)
  {
    case 0: return "Editing Kp";
    case 1: return "Editing Ki";
    case 2: return "Editing Kd";
    default: return "Editing PID";
  }
}

static void pid_edit_step(bool forward)
{
  if (forward)
  {
    if (pid_k[pid_select] < PID_MAX) pid_k[pid_select] += 0.01f;
    if (pid_k[pid_select] > PID_MAX) pid_k[pid_select] = PID_MAX;
  }
  else
  {
    if (pid_k[pid_select] > 0.0f) pid_k[pid_select] -= 0.01f;
    if (pid_k[pid_select] < 0.0f) pid_k[pid_select] = 0.0f;
  }
}

static void text_edit_step(bool forward, uint8_t index)
{
  char &c = text_name[index];
  if (forward)
  {
    if (c >= 'A' && c <= 'Z')
    {
      if (c == 'Z') c = ' ';
      else c += 1;
    }
    else if (c >= 'a' && c <= 'z')
    {
      if (c == 'z') c = 'A';
      else c += 1;
    }
    else
    {
      c = 'a';
    }
  }
  else
  {
    if (c >= 'A' && c <= 'Z')
    {
      if (c == 'A') c = 'z';
      else c -= 1;
    }
    else if (c >= 'a' && c <= 'z')
    {
      if (c == 'a') c = ' ';
      else c -= 1;
    }
    else
    {
      c = 'Z';
    }
  }
}

static void pid_ui_show()
{
  list_show(pid_menu, M_PID);
}

static void pid_edit_ui_show()
{
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setDrawColor(1);
  u8g2.drawBox(16, 16, 96, 31);
  u8g2.setDrawColor(2);
  u8g2.drawBox(17, 17, 94, 29);
  u8g2.setDrawColor(1);

  u8g2.drawFrame(18, 36, 60, 8);
  u8g2.drawBox(20, 38, static_cast<uint8_t>(pid_k[pid_select] / PID_MAX * 56.0f), 4);

  u8g2.setCursor(22, 30);
  u8g2.print(pid_title_for(pid_select));

  u8g2.setCursor(81, 44);
  u8g2.print(pid_k[pid_select]);
}

static void chart_draw_frame()
{
  u8g2.drawStr(4, 12, "Real time angle :");
  u8g2.drawRBox(4, 18, 120, 46, 1);
  u8g2.setDrawColor(2);
  u8g2.drawHLine(10, 58, 108);
  u8g2.drawVLine(10, 24, 34);
  u8g2.drawPixel(7, 27);
  u8g2.drawPixel(8, 26);
  u8g2.drawPixel(9, 25);
  u8g2.drawPixel(116, 59);
  u8g2.drawPixel(115, 60);
  u8g2.drawPixel(114, 61);
  u8g2.setDrawColor(1);
}

static void chart_ui_show()
{
  u8g2.setFont(u8g2_font_wqy12_t_chinese1);
  if (!chart_frame_drawed)
  {
    u8g2.clearBuffer();
    chart_draw_frame();
    angle_last = 20.0f + static_cast<float>(analogRead(CHART_ADC_PIN)) / 100.0f;
    chart_frame_drawed = true;
  }

  u8g2.drawBox(96, 0, 30, 14);
  u8g2.drawVLine(chart_x + 10, 59, 3);
  if (chart_x == 100) chart_x = 0;
  u8g2.drawVLine(chart_x + 11, 24, 34);
  u8g2.drawVLine(chart_x + 12, 24, 34);
  u8g2.drawVLine(chart_x + 13, 24, 34);
  u8g2.drawVLine(chart_x + 14, 24, 34);
  u8g2.setDrawColor(2);
  angle = 20.0f + static_cast<float>(analogRead(CHART_ADC_PIN)) / 100.0f;
  u8g2.drawLine(chart_x + 11, 58 - static_cast<int>(angle_last) / 2, chart_x + 12, 58 - static_cast<int>(angle) / 2);
  u8g2.drawVLine(chart_x + 12, 59, 3);
  angle_last = angle;
  chart_x += 2;
  u8g2.drawBox(96, 0, 30, 14);
  u8g2.setDrawColor(1);
  u8g2.setCursor(96, 12);
  u8g2.print(angle);
}

static void text_edit_ui_show()
{
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setDrawColor(1);
  u8g2.drawRFrame(4, 6, 120, 52, 8);
  u8g2.drawStr((DISP_W - u8g2.getStrWidth("--Text Editor--")) / 2, 20, "--Text Editor--");
  u8g2.drawStr(10, 38, text_name);
  u8g2.drawStr(80, 50, "-Return");

  uint8_t box_x = 9;

  if (text_edit_index < TEXT_NAME_LEN)
  {
    if (text_blink < TEXT_BLINK_SPEED / 2)
    {
      for (uint8_t i = 0; i < text_edit_index; ++i)
      {
        char temp[2] = { text_name[i], '\0' };
        box_x += u8g2.getStrWidth(temp);
        if (text_name[i] != ' ') box_x++;
      }
      char temp[2] = { text_name[text_edit_index], '\0' };
      u8g2.setDrawColor(2);
      u8g2.drawBox(box_x, 26, u8g2.getStrWidth(temp) + 2, 16);
      u8g2.setDrawColor(1);
    }
  }
  else
  {
    u8g2.setDrawColor(2);
    u8g2.drawRBox(78, 38, u8g2.getStrWidth("-Return") + 4, 16, 1);
    u8g2.setDrawColor(1);
  }

  if (text_edit_flag)
  {
    if (text_blink < TEXT_BLINK_SPEED) text_blink += 1;
    else text_blink = 0;
  }
  else
  {
    text_blink = 0;
  }
}

void pid_proc()
{
  pid_ui_show();
  if (!btn.pressed) return;
  btn.pressed = false;
  switch (btn.id)
  {
    case BTN_ID_CC:
    case BTN_ID_CW:
      list_rotate_switch();
      break;
    case BTN_ID_BK:
      ui.index = M_MAIN;
      ui.state = S_LAYER_OUT;
      break;
    case BTN_ID_SP:
      switch (ui.select[ui.layer])
      {
        case 1:
        case 2:
        case 3:
          ui.index = M_PID_EDIT;
          ui.state = S_LAYER_IN;
          break;
        case 4:
          ui.index = M_MAIN;
          ui.state = S_LAYER_OUT;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void pid_edit_proc()
{
  if (btn.pressed)
  {
    btn.pressed = false;
    switch (btn.id)
    {
      case BTN_ID_CC:
        pid_edit_step(false);
        break;
      case BTN_ID_CW:
        pid_edit_step(true);
        break;
      case BTN_ID_SP:
      case BTN_ID_BK:
        ui.index = M_PID;
        ui.state = S_LAYER_OUT;
        break;
      default:
        break;
    }
  }
  pid_edit_ui_show();
}

void chart_proc()
{
  chart_ui_show();
  if (!btn.pressed) return;
  btn.pressed = false;
  ui.index = M_MAIN;
  ui.state = S_LAYER_OUT;
  chart_frame_drawed = false;
  chart_x = 0;
}

void text_edit_proc()
{
  text_edit_ui_show();
  if (!btn.pressed) return;
  btn.pressed = false;
  switch (btn.id)
  {
    case BTN_ID_CC:
      if (text_edit_flag)
      {
        text_edit_step(false, text_edit_index);
      }
      else
      {
        if (text_edit_index == 0) text_edit_index = TEXT_NAME_LEN;
        else text_edit_index -= 1;
      }
      break;
    case BTN_ID_CW:
      if (text_edit_flag)
      {
        text_edit_step(true, text_edit_index);
      }
      else
      {
        if (text_edit_index == TEXT_NAME_LEN) text_edit_index = 0;
        else text_edit_index += 1;
      }
      break;
    case BTN_ID_SP:
      if (text_edit_index == TEXT_NAME_LEN)
      {
        ui.index = M_MAIN;
        ui.state = S_LAYER_OUT;
        text_edit_index = 0;
        text_edit_flag = false;
      }
      else
      {
        text_edit_flag = !text_edit_flag;
      }
      break;
    case BTN_ID_BK:
      ui.index = M_MAIN;
      ui.state = S_LAYER_OUT;
      text_edit_index = 0;
      text_edit_flag = false;
      break;
    default:
      break;
  }
}

void likes_proc()
{
  direct_menu_proc(likes_menu, M_LIKES, M_PID);
}

void collection_proc()
{
  direct_menu_proc(collection_menu, M_COLLECTION, M_CHART);
}

void slot_proc()
{
  direct_menu_proc(slot_menu, M_SLOT, M_TEXT_EDIT);
}

void share_proc()
{
  blank_page_proc(share_menu, M_SHARE);
}

//编辑器菜单处理函数
void editor_proc()
{
  list_show(editor_menu, M_EDITOR); 
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_BK: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        case 0:   ui.index = M_MAIN;  ui.state = S_LAYER_OUT; break;
        case 11:  ui.index = M_KNOB;  ui.state = S_LAYER_IN;  break;
      }
    }
  }
}

//旋钮编辑菜单处理函数
void knob_proc()
{
  list_show(knob_menu, M_KNOB);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_BK: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        case 0: ui.index = M_EDITOR;  ui.state = S_LAYER_OUT; break;
        case 1: ui.index = M_KRF;     ui.state = S_LAYER_IN;  check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]); break;
        case 2: ui.index = M_KPF;     ui.state = S_LAYER_IN;  check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]); break;
      }
    }
  }
}

//旋钮旋转功能菜单处理函数
void krf_proc()
{
  list_show(krf_menu, M_KRF);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_BK: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        case 0: ui.index = M_KNOB;  ui.state = S_LAYER_OUT; break;
        case 1: break;
        case 2: check_box_s_select(KNOB_DISABLE, ui.select[ui.layer]); break;
        case 3: break;
        case 4: check_box_s_select(KNOB_ROT_VOL, ui.select[ui.layer]); break;
        case 5: check_box_s_select(KNOB_ROT_BRI, ui.select[ui.layer]); break;
        case 6: break;
      }
    }
  }
}

//旋钮点按功能菜单处理函数
void kpf_proc()
{
  list_show(kpf_menu, M_KPF);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break;  case BTN_ID_BK: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
    
        case 0:   ui.index = M_KNOB;  ui.state = S_LAYER_OUT; break;
        case 1:   break;
        case 2:   check_box_s_select(KNOB_DISABLE, ui.select[ui.layer]); break;
        case 3:   break;
        case 4:   check_box_s_select('A', ui.select[ui.layer]); break;
        case 5:   check_box_s_select('B', ui.select[ui.layer]); break;
        case 6:   check_box_s_select('C', ui.select[ui.layer]); break;
        case 7:   check_box_s_select('D', ui.select[ui.layer]); break;
        case 8:   check_box_s_select('E', ui.select[ui.layer]); break;
        case 9:   check_box_s_select('F', ui.select[ui.layer]); break;
        case 10:  check_box_s_select('G', ui.select[ui.layer]); break;
        case 11:  check_box_s_select('H', ui.select[ui.layer]); break;
        case 12:  check_box_s_select('I', ui.select[ui.layer]); break;
        case 13:  check_box_s_select('J', ui.select[ui.layer]); break;
        case 14:  check_box_s_select('K', ui.select[ui.layer]); break;
        case 15:  check_box_s_select('L', ui.select[ui.layer]); break;
        case 16:  check_box_s_select('M', ui.select[ui.layer]); break;
        case 17:  check_box_s_select('N', ui.select[ui.layer]); break;
        case 18:  check_box_s_select('O', ui.select[ui.layer]); break;
        case 19:  check_box_s_select('P', ui.select[ui.layer]); break;
        case 20:  check_box_s_select('Q', ui.select[ui.layer]); break;
        case 21:  check_box_s_select('R', ui.select[ui.layer]); break;
        case 22:  check_box_s_select('S', ui.select[ui.layer]); break;
        case 23:  check_box_s_select('T', ui.select[ui.layer]); break;
        case 24:  check_box_s_select('U', ui.select[ui.layer]); break;
        case 25:  check_box_s_select('V', ui.select[ui.layer]); break;
        case 26:  check_box_s_select('W', ui.select[ui.layer]); break;
        case 27:  check_box_s_select('X', ui.select[ui.layer]); break;
        case 28:  check_box_s_select('Y', ui.select[ui.layer]); break;
        case 29:  check_box_s_select('Z', ui.select[ui.layer]); break;
        case 30:  break;
        case 31:  check_box_s_select('0', ui.select[ui.layer]); break;
        case 32:  check_box_s_select('1', ui.select[ui.layer]); break;
        case 33:  check_box_s_select('2', ui.select[ui.layer]); break;
        case 34:  check_box_s_select('3', ui.select[ui.layer]); break;
        case 35:  check_box_s_select('4', ui.select[ui.layer]); break;
        case 36:  check_box_s_select('5', ui.select[ui.layer]); break;
        case 37:  check_box_s_select('6', ui.select[ui.layer]); break;
        case 38:  check_box_s_select('7', ui.select[ui.layer]); break;
        case 39:  check_box_s_select('8', ui.select[ui.layer]); break;
        case 40:  check_box_s_select('9', ui.select[ui.layer]); break;
        case 41:  break;
        case 42:  check_box_s_select( KEY_ESC, ui.select[ui.layer]); break;
        case 43:  check_box_s_select( KEY_F1,  ui.select[ui.layer]); break;
        case 44:  check_box_s_select( KEY_F2,  ui.select[ui.layer]); break;
        case 45:  check_box_s_select( KEY_F3,  ui.select[ui.layer]); break;
        case 46:  check_box_s_select( KEY_F4,  ui.select[ui.layer]); break;
        case 47:  check_box_s_select( KEY_F5,  ui.select[ui.layer]); break;
        case 48:  check_box_s_select( KEY_F6,  ui.select[ui.layer]); break;
        case 49:  check_box_s_select( KEY_F7,  ui.select[ui.layer]); break;
        case 50:  check_box_s_select( KEY_F8,  ui.select[ui.layer]); break;
        case 51:  check_box_s_select( KEY_F9,  ui.select[ui.layer]); break;
        case 52:  check_box_s_select( KEY_F10, ui.select[ui.layer]); break;
        case 53:  check_box_s_select( KEY_F11, ui.select[ui.layer]); break;
        case 54:  check_box_s_select( KEY_F12, ui.select[ui.layer]); break;
        case 55:  break;
        case 56:  check_box_s_select( KEY_LEFT_CTRL,   ui.select[ui.layer]); break;
        case 57:  check_box_s_select( KEY_LEFT_SHIFT,  ui.select[ui.layer]); break;
        case 58:  check_box_s_select( KEY_LEFT_ALT,    ui.select[ui.layer]); break;
        case 59:  check_box_s_select( KEY_LEFT_GUI,    ui.select[ui.layer]); break;
        case 60:  check_box_s_select( KEY_RIGHT_CTRL,  ui.select[ui.layer]); break;
        case 61:  check_box_s_select( KEY_RIGHT_SHIFT, ui.select[ui.layer]); break;
        case 62:  check_box_s_select( KEY_RIGHT_ALT,   ui.select[ui.layer]); break;
        case 63:  check_box_s_select( KEY_RIGHT_GUI,   ui.select[ui.layer]); break;
        case 64:  break;
        case 65:  check_box_s_select( KEY_CAPS_LOCK,   ui.select[ui.layer]); break;
        case 66:  check_box_s_select( KEY_BACKSPACE,   ui.select[ui.layer]); break;
        case 67:  check_box_s_select( KEY_RETURN,      ui.select[ui.layer]); break;
        case 68:  check_box_s_select( KEY_INSERT,      ui.select[ui.layer]); break;
        case 69:  check_box_s_select( KEY_DELETE,      ui.select[ui.layer]); break;
        case 70:  check_box_s_select( KEY_TAB,         ui.select[ui.layer]); break;
        case 71:  break;
        case 72:  check_box_s_select( KEY_HOME,        ui.select[ui.layer]); break;
        case 73:  check_box_s_select( KEY_END,         ui.select[ui.layer]); break;
        case 74:  check_box_s_select( KEY_PAGE_UP,     ui.select[ui.layer]); break;
        case 75:  check_box_s_select( KEY_PAGE_DOWN,   ui.select[ui.layer]); break;
        case 76:  break;
        case 77:  check_box_s_select( KEY_UP_ARROW,    ui.select[ui.layer]); break;
        case 78:  check_box_s_select( KEY_DOWN_ARROW,  ui.select[ui.layer]); break;
        case 79:  check_box_s_select( KEY_LEFT_ARROW,  ui.select[ui.layer]); break;
        case 80:  check_box_s_select( KEY_RIGHT_ARROW, ui.select[ui.layer]); break;
        case 81:  break;
      }
    }
  }
}

//电压测量页处理函数
void volt_proc()
{
  if (btn.pressed) {
    btn.pressed = false;
    switch (btn.id) {
      case BTN_ID_CW:
      case BTN_ID_CC:
        list_rotate_switch();
        break;
      case BTN_ID_SP:
      case BTN_ID_BK:
        ui.index = M_MAIN;
        ui.state = S_LAYER_OUT;
        return;
    }
  }
  volt_show();
}

//设置菜单处理函数，多选框列表类模板，弹窗模板
void setting_proc()
{
  list_show(setting_menu, M_SETTING);
  if (!btn.pressed) return;
  btn.pressed = false;
  switch (btn.id)
  {
    case BTN_ID_CW:
    case BTN_ID_CC:
      list_rotate_switch();
      break;
    case BTN_ID_BK:
      ui.select[ui.layer] = 0;
      // 退回时直接复用确认分支，保持行为一致
    case BTN_ID_SP:
      switch (ui.select[ui.layer])
      {
        case 0:
          ui.index = M_MAIN;
          ui.state = S_LAYER_OUT;
          break;

        //弹出窗口，参数初始化：标题，参数名，参数值，最大值，最小值，步长，背景列表名，背景列表标签
        case 1:   window_value_init("Disp Bri", DISP_BRI, &ui.param[DISP_BRI],  255,  0,  5, setting_menu, M_SETTING);  break;
        case 2:   window_value_init("Tile Ani", TILE_ANI, &ui.param[TILE_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 3:   window_value_init("List Ani", LIST_ANI, &ui.param[LIST_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 4:   window_value_init("Win Ani",  WIN_ANI,  &ui.param[WIN_ANI],   100, 10,  1, setting_menu, M_SETTING);  break;
        case 5:   check_box_m_select( SPOT_ANI );  break;
        case 6:   window_value_init("Tag Ani",  TAG_ANI,  &ui.param[TAG_ANI],   100, 10,  1, setting_menu, M_SETTING);  break;
        case 7:   window_value_init("Fade Ani", FADE_ANI, &ui.param[FADE_ANI],  255,  0,  1, setting_menu, M_SETTING);  break;

        //多选框
        case 8:   check_box_m_select( TILE_UFD  );  break;
        case 9:   check_box_m_select( LIST_UFD  );  break;
        case 10:  check_box_m_select( TILE_LOOP );  break;
        case 11:  check_box_m_select( LIST_LOOP );  break;
        case 12:  check_box_m_select( WIN_BOK   );  break;
        case 13:  check_box_m_select( KNOB_DIR  );  break;
        case 14:  check_box_m_select( DARK_MODE );  break;

        //关于本机
        case 15:  ui.index = M_ABOUT; ui.state = S_LAYER_IN; break;
      }
      break;
  }
}

//关于本机页
void about_proc()
{
  list_show(about_menu, M_ABOUT);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_BK: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {

        case 0:   ui.index = M_SETTING;  ui.state = S_LAYER_OUT; break;
      }
    }
  }
}

//总的UI进程
void ui_proc()
{
  switch (ui.state)
  {
    case S_FADE:          fade();                   break;  //转场动画
    case S_WINDOW:        window_param_init();      break;  //弹窗初始化
    case S_LAYER_IN:      layer_init_in();          break;  //层级初始化
    case S_LAYER_OUT:     layer_init_out();         break;  //层级初始化
  
    case S_NONE:
      if (ui.index != M_CHART) u8g2.clearBuffer();
      switch (ui.index)      //直接选择页面
    {
      case M_WINDOW:      window_proc();            break;
      case M_SLEEP:       sleep_proc();             break;
      case M_MAIN:        main_proc();              break;
      case M_EDITOR:      editor_proc();            break;
      case M_KNOB:        knob_proc();              break;
      case M_KRF:         krf_proc();               break;
      case M_KPF:         kpf_proc();               break;
      case M_VOLT:        volt_proc();              break;
      case M_LIKES:       likes_proc();             break;
      case M_PID:         pid_proc();               break;
      case M_PID_EDIT:    pid_edit_proc();          break;
      case M_COLLECTION:  collection_proc();        break;
      case M_CHART:       chart_proc();             break;
      case M_SLOT:        slot_proc();              break;
      case M_TEXT_EDIT:   text_edit_proc();         break;
      case M_SHARE:       share_proc();             break;
      case M_SETTING:     setting_proc();           break;
      case M_ABOUT:       about_proc();             break;
    }
  }
  u8g2.sendBuffer();
}

//OLED初始化函数
void oled_init()
{
  i2c_master_bus_config_t bus_config = {};
  bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
  bus_config.glitch_ignore_cnt = 7;
  bus_config.i2c_port = I2C_BUS_PORT;
  bus_config.sda_io_num = static_cast<gpio_num_t>(OLED_PIN_NUM_SDA);
  bus_config.scl_io_num = static_cast<gpio_num_t>(OLED_PIN_NUM_SCL);
  bus_config.flags.enable_internal_pullup = true;
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &s_i2c_bus));
  if (scan_oled_address(&s_oled_addr) != ESP_OK) {
    ESP_LOGE(TAG, "OLED probe failed on 0x%02X and 0x%02X, defaulting to 0x%02X",
             OLED_HW_ADDR_PRIMARY, OLED_HW_ADDR_SECONDARY, s_oled_addr);
  }
  u8g2.setBusClock(800000);  // 硬件 IIC 接口使用
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.setContrast(ui.param[DISP_BRI]);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  buf_ptr = u8g2.getBufferPtr();
  buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();
}

void setup() 
{
  eeprom_init();
  ui_init();
  oled_init();
  btn_init();
  ui.sleep = false;
  ui.index = M_MAIN;
  ui.state = S_LAYER_IN;
  ui.select[ui.layer] = 1;
  tile_param_init();
}

void loop() 
{
  btn_scan();
  ui_proc();
}

extern "C" void app_main(void)
{
  setup();
  while (true)
  {
    loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
