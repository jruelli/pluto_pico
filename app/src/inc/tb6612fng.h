// Motor structure
typedef struct {
    struct gpio_dt_spec in1;
    struct gpio_dt_spec in2;
    struct gpio_dt_spec standby;
    drive_command_t current_drive_command;
} motor_t;

// Function declarations
//void motor_init(motor_t *motor);
void motor_a_init(motor_t *motor);
