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

struct MenuItem {
  const char* label;
  uint8_t targetPage;
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

using LabelProvider = const char* (*)(uint8_t index);

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

enum {
  M_LOGO,
  M_SELECT,
  M_PID,
  M_PID_EDIT,
  M_ICON,
  M_CHART,
  M_TEXT_EDIT,
  M_VIDEO,
  M_ABOUT,
};

enum {
  S_NONE,
  S_DISAPPEAR,
  S_SWITCH,
  S_MENU_TO_MENU,
  S_MENU_TO_PIC,
  S_PIC_TO_MENU,
};
