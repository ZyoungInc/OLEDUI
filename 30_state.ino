InputEvent g_inputEvent;
InputAction g_lastMenuDirection = InputAction::None;

uint8_t* buf_ptr;
uint16_t buf_len;

float Kpid[3] = { 9.97, 0.2, 0.01 };

uint8_t disappear_step = 1;

float angle;
float angle_last;
uint8_t chart_x;
bool frame_is_drawed = false;

MenuState mainMenuState;
MenuState pidMenuState;

int16_t icon_x;
int16_t icon_x_trg;
int16_t icon_label_offset;
int16_t icon_label_offset_trg;
int8_t icon_select;

uint8_t ui_index;
uint8_t ui_state;

char name[] = "Hello World ";
uint8_t edit_index = 0;
bool edit_flag = false;
uint8_t blink_flag;
