#include <sys/cdefs.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <devicetree_generated.h>


// GPIO and PWM specifications from device tree
#define MOTOR_A_IN_1	DT_ALIAS(motorain1)
#define MOTOR_A_IN_2	DT_ALIAS(motorain2)
#define MOTOR_B_IN_1	DT_ALIAS(motorbin1)
#define MOTOR_B_IN_2	DT_ALIAS(motorbin2)
#define MOTOR_STANDBY	DT_ALIAS(stdby)

static const struct gpio_dt_spec motor_a_in1   = GPIO_DT_SPEC_GET_OR(MOTOR_A_IN_1, gpios,{0});
static const struct gpio_dt_spec motor_a_in2   = GPIO_DT_SPEC_GET_OR(MOTOR_A_IN_2, gpios,{0});
static const struct gpio_dt_spec motor_b_in1   = GPIO_DT_SPEC_GET_OR(MOTOR_B_IN_1, gpios,{0});
static const struct gpio_dt_spec motor_b_in2   = GPIO_DT_SPEC_GET_OR(MOTOR_B_IN_2, gpios,{0});
static const struct gpio_dt_spec motor_standby = GPIO_DT_SPEC_GET_OR(MOTOR_STANDBY, gpios,{0});
void motor_a_init(motor_t *motor) {
    motor->in1 = motor_a_in1;
    motor->in2 = motor_a_in2;
    motor->standby = motor_standby;
    motor->current_drive_command = STOP;

    // Initialize GPIO pins as outputs
    gpio_pin_configure_dt(&motor_a_in1, GPIO_OUTPUT);
    gpio_pin_configure_dt(&motor_a_in2, GPIO_OUTPUT);
    //test if this actually works
    while(1)
    {
        gpio_pin_set(motor_a_in1.port, motor_a_in1.pin, 1);
        gpio_pin_set(motor_a_in1.port, motor_a_in2.pin, 1);
        k_sleep(K_MSEC(1000u));
        gpio_pin_set(motor_a_in1.port, motor_a_in1.pin, 0);
        gpio_pin_set(motor_a_in1.port, motor_a_in2.pin, 0);
        k_sleep(K_MSEC(1000u));
    }
}
