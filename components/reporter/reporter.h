#include "esp_err.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "esp_hid_common.h"

void init_reporter();

#define LEFT true

typedef union
{
    uint8_t Value;
    struct {
        bool LCTRL: 1;
        bool LSHIFT: 1;
        bool LALT: 1;
        bool LGUI: 1;
        bool RCTRL: 1;
        bool RSHIFT: 1;
        bool RALT: 1;
        bool RGUI: 1;
    };
} KeyboardModifier;

/* USB HID Keyboard/Keypad Usage(0x07) */
enum hid_keyboard_keypad_usage {
    KC_NO               = 0x00,
    KC_ROLL_OVER,
    KC_POST_FAIL,
    KC_UNDEFINED,
    KC_A,
    KC_B,
    KC_C,
    KC_D,
    KC_E,
    KC_F,
    KC_G,
    KC_H,
    KC_I,
    KC_J,
    KC_K,
    KC_L,
    KC_M,               /* 0x10 */
    KC_N,
    KC_O,
    KC_P,
    KC_Q,
    KC_R,
    KC_S,
    KC_T,
    KC_U,
    KC_V,
    KC_W,
    KC_X,
    KC_Y,
    KC_Z,
    KC_1,
    KC_2,
    KC_3,               /* 0x20 */
    KC_4,
    KC_5,
    KC_6,
    KC_7,
    KC_8,
    KC_9,
    KC_0,
    KC_ENTER,
    KC_ESCAPE,
    KC_BSPACE,
    KC_TAB,
    KC_SPACE,
    KC_MINUS,
    KC_EQUAL,
    KC_LBRACKET,
    KC_RBRACKET,        /* 0x30 */
    KC_BSLASH,          /* \ (and |) */
    KC_NONUS_HASH,      /* Non-US # and ~ (Typically near the Enter key) */
    KC_SCOLON,          /* ; (and :) */
    KC_QUOTE,           /* ' and " */
    KC_GRAVE,           /* Grave accent and tilde */
    KC_COMMA,           /* , and < */
    KC_DOT,             /* . and > */
    KC_SLASH,           /* / and ? */
    KC_CAPSLOCK,
    KC_F1,
    KC_F2,
    KC_F3,
    KC_F4,
    KC_F5,
    KC_F6,
    KC_F7,              /* 0x40 */
    KC_F8,
    KC_F9,
    KC_F10,
    KC_F11,
    KC_F12,
    KC_PSCREEN,
    KC_SCROLLLOCK,
    KC_PAUSE,
    KC_INSERT,
    KC_HOME,
    KC_PGUP,
    KC_DELETE,
    KC_END,
    KC_PGDOWN,
    KC_RIGHT,
    KC_LEFT,            /* 0x50 */
    KC_DOWN,
    KC_UP,
    KC_NUMLOCK,
    KC_KP_SLASH,
    KC_KP_ASTERISK,
    KC_KP_MINUS,
    KC_KP_PLUS,
    KC_KP_ENTER,
    KC_KP_1,
    KC_KP_2,
    KC_KP_3,
    KC_KP_4,
    KC_KP_5,
    KC_KP_6,
    KC_KP_7,
    KC_KP_8,            /* 0x60 */
    KC_KP_9,
    KC_KP_0,
    KC_KP_DOT,
    KC_NONUS_BSLASH,    /* Non-US \ and | (Typically near the Left-Shift key) */
    KC_APPLICATION,
    KC_POWER,
    KC_KP_EQUAL,
    KC_F13,
    KC_F14,
    KC_F15,
    KC_F16,
    KC_F17,
    KC_F18,
    KC_F19,
    KC_F20,
    KC_F21,             /* 0x70 */
    KC_F22,
    KC_F23,
    KC_F24,
    KC_EXECUTE,
    KC_HELP,
    KC_MENU,
    KC_SELECT,
    KC_STOP,
    KC_AGAIN,
    KC_UNDO,
    KC_CUT,
    KC_COPY,
    KC_PASTE,
    KC_FIND,
    KC__MUTE,
    KC__VOLUP,          /* 0x80 */
    KC__VOLDOWN,
    KC_LOCKING_CAPS,    /* locking Caps Lock */
    KC_LOCKING_NUM,     /* locking Num Lock */
    KC_LOCKING_SCROLL,  /* locking Scroll Lock */
    KC_KP_COMMA,
    KC_KP_EQUAL_AS400,  /* equal sign on AS/400 */
    KC_INT1,
    KC_INT2,
    KC_INT3,
    KC_INT4,
    KC_INT5,
    KC_INT6,
    KC_INT7,
    KC_INT8,
    KC_INT9,
    KC_LANG1,           /* 0x90 */
    KC_LANG2,
    KC_LANG3,
    KC_LANG4,
    KC_LANG5,
    KC_LANG6,
    KC_LANG7,
    KC_LANG8,
    KC_LANG9,
    KC_ALT_ERASE,
    KC_SYSREQ,
    KC_CANCEL,
    KC_CLEAR,
    KC_PRIOR,
    KC_RETURN,
    KC_SEPARATOR,
    KC_OUT,             /* 0xA0 */
    KC_OPER,
    KC_CLEAR_AGAIN,
    KC_CRSEL,
    KC_EXSEL,           /* 0xA4 */

    /* NOTE: 0xA5-DF are used for internal special purpose */

    /* Modifiers */
    KC_LCTRL            = 0xE0,
    KC_LSHIFT,
    KC_LALT,
    KC_LGUI,
    KC_RCTRL,
    KC_RSHIFT,
    KC_RALT,
    KC_RGUI,

    /* NOTE: 0xE8-FF are used for internal special purpose */
};