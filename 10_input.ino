typedef struct {
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
      return digitalRead(BTN3);
    default:
      return false;
  }
}

void key_init() {
  for (uint8_t i = 0; i < (sizeof(key) / sizeof(KEY)); ++i) {
    key[i].val = key[i].last_val = get_key_val(i);
  }
}

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
