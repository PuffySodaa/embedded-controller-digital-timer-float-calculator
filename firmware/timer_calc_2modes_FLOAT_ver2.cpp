/*
 * Du an: Dong ho hen gio & May tinh bo tui (Phien ban FLOAT - 7 SO - Dao chieu PD0-PD7)
 * VDK: ATmega16
 * Hardware Update: PC1 (Timer) va PC2 (Calc) su dung PULL-DOWN ngoai.
 * Logic: Input HIGH (1) = Active.
 */

#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h> 

// ========================================================================
// DINH NGHIA PIN VA MODE
// ========================================================================
#define RELAY_PIN 0      // PC0
#define TIMER_MODE_PIN 1 // PC1
#define CALC_MODE_PIN 2  // PC2

#define MODE_COUNTDOWN 0
#define MODE_CALCULATOR 1

#define KEY_DIVIDE 10
#define KEY_MULTIPLY 11
#define KEY_SUBTRACT 12
#define KEY_ADD 13
#define KEY_EQUALS -3 // Phim '#'
#define KEY_AC -2     // Phim '*'

#define CALC_ERROR_VAL NAN 

// ========================================================================
// LED 7 DOAN ANODE CHUNG
// ========================================================================
#define SEG_E 0b10000110      
#define SEG_r 0b10101111      
#define SEG_NEGATIVE ~(1<<6) 

uint8_t seg_code[] = {
    0b11000000, // 0
    0b11111001, // 1
    0b10100100, // 2
    0b10110000, // 3
    0b10011001, // 4
    0b10010010, // 5
    0b10000010, // 6
    0b11111000, // 7
    0b10000000, // 8
    0b10010000, // 9
    SEG_E,      // 10
    SEG_r       // 11
};

// ========================================================================
// BIEN TOAN CUC
// ========================================================================

volatile uint8_t digits[8] = {0};
volatile uint8_t pos = 0;         
volatile uint8_t count = 0; 
volatile bool is_negative = false;  
volatile int8_t decimal_pos = -1; 
volatile uint8_t current_mode = MODE_COUNTDOWN;

// Bien Hen gio
volatile uint8_t countdown_active = 0; 
volatile uint8_t paused = 0;           
volatile uint8_t hh = 0, mm = 0, ss = 0, cs = 0;
volatile uint8_t countdown_display_count = 0;
volatile bool new_countdown_input = false;

// Bien May tinh
float current_value = 0.0f; 
float input_value = 0.0f;   
uint8_t current_operator = 0; 
bool new_input = true;      

// ========================================================================
// KHAI BAO TRUOC CAC HAM
// ========================================================================
void io_init(void);
int8_t keypad_scan(void);
void timer0_init(void);
void timer1_init(void);
void display_error(void);
void display_float(float number); 
void reset_countdown(void);
void reset_calculator(void);
void calculate(void);
void process_calculator_key(int8_t key);

// ========================================================================
// CAC HAM HO TRO HIEN THI
// ========================================================================

void display_error(void) {
    digits[0] = 11; // r
    digits[1] = 11; // r
    digits[2] = 10; // E
    for(uint8_t i=3; i<8; i++) digits[i] = 0;
    count = 3;             
    is_negative = false;   
    decimal_pos = -1;      
}

void display_float(float number) {
    decimal_pos = -1; 
    count = 0;        
    is_negative = false;

    if (isnan(number)) {
        display_error();
        return;
    }
    if (number < 0.0f) {
        is_negative = true;
        number = -number; 
    }
    if (isinf(number)) {
        display_error();
        current_value = NAN; 
        return;
    }
    // Gioi han 7 chu so
    if (number > 9999999.0f) {
        display_error(); 
        current_value = NAN;
        return;
    }
    
    int32_t int_part = (int32_t)number;
    int32_t temp_num = int_part;
    uint8_t int_digits_count = 0;
    
    if(temp_num == 0) {
        int_digits_count = 1; 
    } else {
        while (temp_num > 0) {
            int_digits_count++;
            temp_num /= 10;
        }
    }

    if (int_digits_count > 7) {
        display_error();
        current_value = NAN; 
        return;
    }

    uint8_t frac_digits_available = 7 - int_digits_count; 
    float frac_part = number - (float)int_part; 
    uint32_t frac_as_int = 0;

    if (frac_digits_available > 0 && frac_part > 0.0000001f) {
        float multiplier = 1.0f;
        for (uint8_t i = 0; i < frac_digits_available; i++) {
            multiplier *= 10.0f;
        }
        frac_as_int = (uint32_t)roundf(frac_part * multiplier);
        if (frac_as_int >= (uint32_t)multiplier) {
            frac_as_int = 0; 
            int_part++;      
            if(int_part > 9999999) {
                 display_error();
                 current_value = NAN;
                 return;
            }
        }
    }
    
    count = 0;
    decimal_pos = -1;
    bool has_significant_frac = false;
    
    while (frac_as_int > 0 && (frac_as_int % 10 == 0)) {
        frac_as_int /= 10;
    }
    uint32_t temp_frac = frac_as_int;
    if (temp_frac > 0) {
        has_significant_frac = true;
        while (temp_frac > 0) {
            if (count >= 7) break; 
            digits[count] = temp_frac % 10;
            temp_frac /= 10;
            count++;
        }
    }
    if (has_significant_frac) {
        decimal_pos = count; 
    }
    temp_num = int_part;
    do {
        if (count >= 7) break; 
        digits[count] = temp_num % 10;
        temp_num /= 10;
        count++;
    } while (temp_num > 0);

    for (uint8_t i = count; i < 8; i++) {
        digits[i] = 0;
    }
}

// ========================================================================
// CAC HAM KHOI TAO VA RESET
// ========================================================================

void reset_countdown(void) {
    countdown_active = 0; paused = 0; count = 0;
    countdown_display_count = 0;
    hh = 0; mm = 0; ss = 0; cs = 0;
    for (uint8_t i=0; i<8; i++) digits[i] = 0;
    PORTC &= ~(1 << RELAY_PIN); 
    new_countdown_input = false;
}

void reset_calculator(void) {
    current_value = 0.0f;
    input_value = 0.0f;
    current_operator = 0;
    new_input = true;
    is_negative = false;
    display_float(0.0f); 
}

void io_init(void) {
    DDRB = 0xFF; PORTB = 0xFF; 
    DDRD = 0xFF; PORTD = 0x00; 
    DDRA = 0x0F; PORTA = 0xF0;
    
    // PC0 Output (Relay), PC1 & PC2 Input
    DDRC = (1 << RELAY_PIN); 
    
    // PORTC = 0x00: Tat pull-up noi bo (vi da dung pull-down ngoai)
    PORTC = 0x00;           
    sei(); 
}

int8_t keypad_scan(void) {
    const int8_t keymap[4][4] = {
        {7, 8, 9, KEY_DIVIDE},
        {4, 5, 6, KEY_MULTIPLY},
        {1, 2, 3, KEY_SUBTRACT},
        {KEY_AC, 0, KEY_EQUALS, KEY_ADD}
    };
    for (uint8_t row = 0; row < 4; row++) {
        PORTA |= 0x0F;       
        PORTA &= ~(1 << row); 
        _delay_us(5);         
        uint8_t cols = (PINA >> 4) & 0x0F; 
        if (cols != 0x0F) {
            _delay_ms(10); 
            cols = (PINA >> 4) & 0x0F; 
            for (uint8_t col = 0; col < 4; col++) {
                if (!(cols & (1 << col))) { 
                    while (!((PINA >> 4) & (1 << col))); 
                    return keymap[row][col]; 
                }
            }
        }
    }
    return -1;
}

// ========================================================================
// KHOI TAO TIMER
// ========================================================================

void timer0_init(void) {
    TCCR0 = (1<<WGM01) | (1<<CS01) | (1<<CS00);
    OCR0 = 124;
    TIMSK |= (1<<OCIE0); 
}

void timer1_init(void) {
    TCCR1B = (1<<WGM12) | (1<<CS11) | (1<<CS10);
    OCR1A = 1249;
    TIMSK |= (1<<OCIE1A); 
}

// ========================================================================
// CAC HAM NGAT (ISR) - QUAN TRONG NHAT
// ========================================================================

ISR(TIMER0_COMP_vect) {
    PORTD = 0x00; // Tat het LED de chong lem

    if (current_mode == MODE_COUNTDOWN) {
        // === Che do Hen gio ===
        if (count == 0) { 
            // Hien thi so 0 o hang don vi (pos 0) -> PD7
            if (pos == 0) { 
                PORTB = seg_code[0]; 
                PORTD = (1<<(7-0)); // PD7
            } 
            else { PORTB = 0xFF; PORTD = 0x00; }
        } else { 
            if (pos < count) {
                uint8_t data = seg_code[ digits[pos] ];
                // Bat dau cham o pos 2,4,6 (tuong ung voi 1s, 1m, 1h)
                if (pos == 2 || pos == 4 || pos == 6) { data &= ~(1<<7); }
                PORTB = data;
                // Dao chieu bit
                PORTD = (1<<(7-pos)); 
            } else {
                PORTB = 0xFF; PORTD = 0x00;
            }
        }
    }
    else {
        // === Che do May tinh ===
        // 1. Hien thi chu so
        if (pos < count) { 
            uint8_t data = seg_code[ digits[pos] ];
            if (pos == decimal_pos) {
                 data &= ~(1<<7); // Bat bit DP
            }              
            PORTB = data;     
            // Dao chieu bit
            PORTD = (1<<(7-pos));   
        } 
        // 2. Hien thi dau am (o vi tri pos == count)
        else { 
            if (is_negative && pos == count && count < 8) {
                PORTB = SEG_NEGATIVE; 
                // Dao chieu bit (hien thi ngay ben trai so)
                PORTD = (1<<(7-pos));     
            } else {
                PORTB = 0xFF; 
                PORTD = 0x00; 
            }
        }
    }

    pos++; 
    if (pos > 7) pos = 0;
}

ISR(TIMER1_COMPA_vect) {
    if (current_mode == MODE_COUNTDOWN && countdown_active && !paused) {
        if (hh==0 && mm==0 && ss==0 && cs==0) {
            PORTC ^= (1 << RELAY_PIN); 
            countdown_active = 0;     
            digits[0] = 0;
            for(uint8_t i=1; i<8; i++) digits[i] = 0;
            count = 1;
            countdown_display_count = 0;
            new_countdown_input = true; 
            return; 
        }
        if (cs > 0) cs--;
        else { cs = 99; 
            if (ss > 0) ss--;
            else { ss = 59;
                if (mm > 0) mm--;
                else { mm = 59;
                    if (hh > 0) hh--;
                }
            }
        }
        uint8_t temp, d1; 
        temp = cs; d1 = 0; while (temp >= 10) { temp -= 10; d1++; }
        digits[0] = temp; digits[1] = d1;
        temp = ss; d1 = 0; while (temp >= 10) { temp -= 10; d1++; }
        digits[2] = temp; digits[3] = d1;
        temp = mm; d1 = 0; while (temp >= 10) { temp -= 10; d1++; }
        digits[4] = temp; digits[5] = d1;
        temp = hh; d1 = 0; while (temp >= 10) { temp -= 10; d1++; }
        digits[6] = temp; digits[7] = d1;
        count = countdown_display_count;
    }
}

// ========================================================================
// LOGIC CHE DO MAY TINH
// ========================================================================

void calculate(void) {
    if (isnan(current_value)) {
        new_input = true;
        return;
    }
    if (current_operator == 0) {
        current_value = input_value;
    } else {
        float a = current_value; 
        float b = input_value;   
        float result = 0.0f;
        bool error = false; 
        switch (current_operator) {
            case KEY_ADD: result = a + b; break;
            case KEY_SUBTRACT: result = a - b; break;
            case KEY_MULTIPLY: result = a * b; break;
            case KEY_DIVIDE: 
                if (b == 0.0f) { error = true; }
                else { result = a / b; }
                break;
        } 
        if (isinf(result) || isnan(result)) { error = true; }
        if (error) { current_value = NAN; }
        else { current_value = result; }
    }
    display_float(current_value);
    input_value = current_value;
    new_input = true; 
}

void process_calculator_key(int8_t key) {
    if (key >= 0 && key <= 9) {
        if (isnan(current_value)) {
            reset_calculator();
        }
        if (new_input) {
            input_value = 0.0f;
            new_input = false; 
        }
        if (input_value == 0.0f) {
            input_value = (float)key;
        }
        else {
            if (input_value >= 1000000.0f) { 
                return; 
            }
            input_value = input_value * 10.0f + (float)key;
        }
        display_float(input_value);
    }
    else if (key >= KEY_DIVIDE && key <= KEY_ADD) {
        if (!new_input) {
            calculate();
        }
        if (!isnan(current_value)) {
            current_operator = key;
        }
        new_input = true; 
    }
    else if (key == KEY_EQUALS) {
        if (!new_input) {
            calculate();
        }
        current_operator = 0; 
        new_input = true;
    }
    else if (key == KEY_AC) {
        reset_calculator();
    }
}

// ========================================================================
// VONG LAP CHINH
// ========================================================================

int main(void) {
    io_init();
    timer0_init(); 
    timer1_init(); 
    reset_countdown();

    while (1) {
        int8_t key = keypad_scan();

        // === THAY DOI LOGIC CHECK MODE THEO PHAN CUNG PULL-DOWN ===
        // 1. Uu tien kiem tra mode May tinh (PC2 High)
        if (PINC & (1 << CALC_MODE_PIN)) { 
            if (current_mode != MODE_CALCULATOR) {
                current_mode = MODE_CALCULATOR;
                reset_calculator();
                _delay_ms(200); 
            }
        } 
        // 2. Neu khong, kiem tra mode Hen gio (PC1 High)
        else if (PINC & (1 << TIMER_MODE_PIN)) {  
            if (current_mode != MODE_COUNTDOWN) {
                current_mode = MODE_COUNTDOWN;
                reset_countdown();
                _delay_ms(200); 
            }
        }
        // 3. Neu ca 2 cung Low (khong gat cong tac), chuong trinh se
        // giu nguyen mode hien tai, khong bi reset lien tuc.
        
        if (key != -1) {
            if (current_mode == MODE_COUNTDOWN) {
                if (key >= 0 && key <= 9 && !countdown_active) {
                    if (new_countdown_input) {
                        for (uint8_t i = 0; i < 8; i++) digits[i] = 0;
                        count = 0;
                        new_countdown_input = false;
                    }
                    if (count < 8U) { 
                        for (uint8_t i = count; i > 0; i--) {
                            digits[i] = digits[i-1];
                        }
                        digits[0] = key;
                        count++;
                    }
                }
                else if (key == KEY_AC) {
                    reset_countdown();
                }
                else if (key == KEY_EQUALS) {
                    if (!countdown_active && count > 0) {
                        cs = digits[0] + digits[1]*10;
                        ss = digits[2] + digits[3]*10;
                        mm = digits[4] + digits[5]*10;
                        hh = digits[6] + digits[7]*10;
                        if (mm >= 60) mm = 59;
                        if (ss >= 60) ss = 59;
                        if (cs >= 100) cs = 99;
                        countdown_display_count = count;
                        countdown_active = 1; 
                        paused = 0;
                        new_countdown_input = false;
                    }
                    else if (countdown_active) {
                        paused = !paused; 
                    }
                }
            }
            else { 
                process_calculator_key(key);
            }
        }
        _delay_ms(10);
    }
}