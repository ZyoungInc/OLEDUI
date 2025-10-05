// ==== LOGO PAGE ====

void logo_ui_show() {
  u8g2.drawXBMP(0, 0, 128, 64, LOGO);
}

void logo_proc() {
  if (g_inputEvent.action != InputAction::None) {
    g_inputEvent.action = InputAction::None;
    ui_state = S_DISAPPEAR;
    ui_index = M_SELECT;
  }
  logo_ui_show();
}

// ==== MAIN MENU PAGE ====

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

const char* getMainMenuLabel(uint8_t index) {
  return kMainMenu[index].label;
}

void select_ui_show() {
  move_indicator(&mainMenuState.indicatorY, &mainMenuState.indicatorTarget, kMainMenuLineSegment);
  move(&mainMenuState.scroll, &mainMenuState.scrollTarget);
  move(&mainMenuState.boxY, &mainMenuState.boxYTarget);
  move_width(&mainMenuState.boxWidth, &mainMenuState.boxWidthTarget, mainMenuState.selected, g_lastMenuDirection, getMainMenuLabel,
             kMainMenuCount);

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

// ==== PID MENU PAGE ====

const PidMenuItem kPidMenu[] = {
  { "-Proportion", true },
  { "-Integral", true },
  { "-Derivative", true },
  { "Return", false },
};

const uint8_t kPidMenuCount = sizeof(kPidMenu) / sizeof(PidMenuItem);
const uint8_t kPidLineSegment = 15;

const char* getPidMenuLabel(uint8_t index) {
  return kPidMenu[index].label;
}

void pid_ui_show() {
  move_indicator(&pidMenuState.indicatorY, &pidMenuState.indicatorTarget, kPidLineSegment);
  move(&pidMenuState.boxY, &pidMenuState.boxYTarget);
  move_width(&pidMenuState.boxWidth, &pidMenuState.boxWidthTarget, pidMenuState.selected, g_lastMenuDirection, getPidMenuLabel,
             kPidMenuCount);

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

void pid_edit_ui_show() {
  u8g2.drawBox(16, 16, 96, 31);
  u8g2.setDrawColor(2);
  u8g2.drawBox(17, 17, 94, 29);
  u8g2.setDrawColor(1);
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
        pidMenuState.boxWidth = pidMenuState.boxWidthTarget = u8g2.getStrWidth(kPidMenu[pidMenuState.selected].label) +
                               kMenuTextOffsetX * 2;
      }
      break;
    case InputAction::Back:
      ui_state = S_DISAPPEAR;
      ui_index = M_SELECT;
      pidMenuState.selected = 0;
      pidMenuState.indicatorY = pidMenuState.indicatorTarget = 1;
      pidMenuState.boxY = pidMenuState.boxYTarget = 0;
      pidMenuState.boxWidth = pidMenuState.boxWidthTarget =
          u8g2.getStrWidth(kPidMenu[pidMenuState.selected].label) + kMenuTextOffsetX * 2;
      break;
    default:
      break;
  }

  pidMenuState.boxWidthTarget = u8g2.getStrWidth(kPidMenu[pidMenuState.selected].label) + kMenuTextOffsetX * 2;
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

// ==== ICON PAGE ====

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

// ==== CHART PAGE ====

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

// ==== TEXT EDIT PAGE ====

void text_edit_ui_show() {
  u8g2.drawRFrame(4, 6, 120, 52, 8);
  u8g2.drawStr((128 - u8g2.getStrWidth("--Text Editor--")) / 2, 20, "--Text Editor--");
  u8g2.drawStr(10, 38, name);
  u8g2.drawStr(80, 50, "-Return");

  uint8_t box_x = 9;

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

  if (edit_flag) {
    if (blink_flag < BLINK_SPEED) {
      blink_flag += 1;
    } else {
      blink_flag = 0;
    }
  } else {
    blink_flag = 0;
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

// ==== ABOUT PAGE ====

void about_ui_show() {
  u8g2.drawStr(2, 12, "CocoFactory Sleep Porj");
  u8g2.drawStr(2, 28, "    Rortable Radar Kit");
  u8g2.drawStr(2, 44, "FreeRAM : 517KB");
  u8g2.drawStr(2, 60, "Sensor : R60A");
}

void about_proc() {
  if (g_inputEvent.action != InputAction::None) {
    g_inputEvent.action = InputAction::None;
    ui_state = S_DISAPPEAR;
    ui_index = M_SELECT;
  }
  about_ui_show();
}

// ==== UI DISPATCH ====

void ui_proc() {
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
