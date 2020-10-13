#include "driver/gpio.h"

#define LEFT true

#define COL1 GPIO_NUM_2
#define COL2 GPIO_NUM_14
#define COL3 GPIO_NUM_15
#define COL4 GPIO_NUM_13
#define COL5 GPIO_NUM_26
#define COL6 GPIO_NUM_25
#define ROW1 GPIO_NUM_16
#define ROW2 GPIO_NUM_5
#define ROW3 GPIO_NUM_4
#define ROW4 GPIO_NUM_0
#define ROW5 GPIO_NUM_3
#define ROW6 GPIO_NUM_12

#if LEFT
#define MATRIX_COLS { COL6, COL5, COL4, COL3, COL2, COL1 }
#else
#define MATRIX_COLS { COL1, COL2, COL3, COL4, COL5, COL6 }
#endif
#define MATRIX_ROWS { ROW1, ROW2, ROW3, ROW4, ROW5, ROW6 }
#define NCOL 6
#define NROW 6
#define NBUTTON (NCOL * NROW)
extern bool input_buttons[NBUTTON];

void scan_input();
void setup_input();