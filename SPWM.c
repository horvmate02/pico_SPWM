#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"


// define pins for the three phases
#define PIN_a 0
#define PIN_b 2
#define PIN_c 4


// PWM frequency [Hz]
const uint PWM_freq = 10000;

// TOP value
const uint TOP = 999;

// timer interupt interval [us]
const uint timer_int = 500;
float dt;
float drad;

// output frequency [Hz]
uint output_frequency = 1; // [Hz]

// amplitude modulation ratio
float ma = 1;

float phase_a;
float phase_b;
float phase_c;

uint slice_a;
uint slice_b;
uint slice_c;
uint chan_a;
uint chan_b;
uint chan_c;

uint ia;
uint ib;
uint ic;

float half;

#define LUT 512
const int LUT_ = LUT;
float sine[LUT];

const float phase_to_i = LUT_/(2.0f*M_PI);

bool timer_callback(__unused struct repeating_timer *t)
{
    phase_a += drad;
    phase_b += drad;
    phase_c += drad;
    if (phase_a >= 2*M_PI)phase_a=0;
    if (phase_b >= 2*M_PI)phase_b=0;
    if (phase_c >= 2*M_PI)phase_c=0;

    ia = (uint)(phase_a*phase_to_i);
    ib = (uint)(phase_b*phase_to_i);
    ic = (uint)(phase_c*phase_to_i);

    if(ia==LUT_)ia=0;
    if(ib==LUT_)ib=0;
    if(ic==LUT_)ic=0;
    
    pwm_set_chan_level(slice_a, chan_a, (uint16_t)(ma*sine[ia]));
    pwm_set_chan_level(slice_b, chan_b, (uint16_t)(ma*sine[ib]));
    pwm_set_chan_level(slice_c, chan_c, (uint16_t)(ma*sine[ic]));


    return true;
}

int main()
{
    stdio_init_all();
    gpio_set_function(PIN_a, GPIO_FUNC_PWM);
    gpio_set_function(PIN_b, GPIO_FUNC_PWM);
    gpio_set_function(PIN_c, GPIO_FUNC_PWM);


    slice_a = pwm_gpio_to_slice_num(PIN_a);
    slice_b = pwm_gpio_to_slice_num(PIN_b);
    slice_c = pwm_gpio_to_slice_num(PIN_c);
    chan_a = pwm_gpio_to_channel(PIN_a);
    chan_b = pwm_gpio_to_channel(PIN_b);
    chan_c = pwm_gpio_to_channel(PIN_c);


    pwm_set_wrap(slice_a, TOP);
    pwm_set_wrap(slice_b, TOP);
    pwm_set_wrap(slice_c, TOP);

    // phase corrected PWM -> triangle carrier signal
    pwm_set_phase_correct(slice_a, true);
    pwm_set_phase_correct(slice_b, true);
    pwm_set_phase_correct(slice_c, true);

    // div
    float div = 125000000.0f/((TOP+1)*2.0f*PWM_freq);
    uint8_t div_int = (uint8_t)div;
    uint8_t div_frac = (uint8_t)((div - div_int) * 16.0f);
    pwm_set_clkdiv_int_frac4(slice_a, div_int, div_frac);
    pwm_set_clkdiv_int_frac4(slice_b, div_int, div_frac);
    pwm_set_clkdiv_int_frac4(slice_c, div_int, div_frac);


    dt = timer_int * 1e-6f;
    drad = 2*M_PI*output_frequency * dt;
    half = (TOP + 1)/2.0f;

    // lookup table
    for (int i=0;i<LUT_;i++)sine[i]=half*sinf((float)i/(float)LUT_*2*M_PI)+half;

    phase_a = 0;
    phase_b = 2*M_PI/3;
    phase_c = 4*M_PI/3;

    // timer
    struct repeating_timer timer;
    add_repeating_timer_us(timer_int, timer_callback, NULL, &timer);

    pwm_set_chan_level(slice_a, chan_a, 0);
    pwm_set_chan_level(slice_b, chan_b, 0);
    pwm_set_chan_level(slice_c, chan_c, 0);


    pwm_set_enabled(slice_a, true);
    pwm_set_enabled(slice_b, true);
    pwm_set_enabled(slice_c, true);



    while (true) {
        printf("Output frequency: %i Hz\n", output_frequency);
        printf("PWM frequency: %i Hz\n", PWM_freq);
        printf("Update interval: %i us\n", timer_int);
        printf("===========================\n");
        sleep_ms(1000);
    }
}
