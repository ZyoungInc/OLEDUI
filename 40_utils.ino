bool move(int16_t* a, int16_t* a_trg) {
  if (*a < *a_trg) {
    *a += SPEED;
    if (*a > *a_trg) *a = *a_trg;
  } else if (*a > *a_trg) {
    *a -= SPEED;
    if (*a < *a_trg) *a = *a_trg;
  } else {
    return true;
  }
  return false;
}

bool move_icon(int16_t* a, int16_t* a_trg) {
  if (*a < *a_trg) {
    *a += ICON_SPEED;
    if (*a > *a_trg) *a = *a_trg;
  } else if (*a > *a_trg) {
    *a -= ICON_SPEED;
    if (*a < *a_trg) *a = *a_trg;
  } else {
    return true;
  }
  return false;
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

void text_edit(bool dir, uint8_t index) {
  if (!dir) {
    if (name[index] >= 'A' && name[index] <= 'Z') {
      if (name[index] == 'A') {
        name[index] = 'z';
      } else {
        name[index] -= 1;
      }
    } else if (name[index] >= 'a' && name[index] <= 'z') {
      if (name[index] == 'a') {
        name[index] = ' ';
      } else {
        name[index] -= 1;
      }
    } else {
      name[index] = 'Z';
    }
  } else {
    if (name[index] >= 'A' && name[index] <= 'Z') {
      if (name[index] == 'Z') {
        name[index] = ' ';
      } else {
        name[index] += 1;
      }
    } else if (name[index] >= 'a' && name[index] <= 'z') {
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

void chart_draw_frame() {
  u8g2.drawStr(4, 12, "Real time angle :");
  u8g2.drawRBox(4, 18, 120, 46, 8);
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
