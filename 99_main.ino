U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/D5, /* data=*/D4);

void setup() {
  Serial.begin(9600);
  pinMode(BTN0, INPUT_PULLUP);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  key_init();
  u8g2.setBusClock(2000000);
  u8g2.begin();
  u8g2.setFont(u8g2_font_wqy12_t_chinese1);

  buf_ptr = u8g2.getBufferPtr();
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
  ui_state = S_NONE;
  chart_x = 0;
  frame_is_drawed = false;
}

void loop() {
  g_inputEvent = pollInput();
  if (g_inputEvent.action == InputAction::Up || g_inputEvent.action == InputAction::Down) {
    g_lastMenuDirection = g_inputEvent.action;
  }
  ui_proc();
}
