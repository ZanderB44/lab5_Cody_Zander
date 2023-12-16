#include <zephyr.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>


#define DEV_IN DT_NODELABEL(gpioa)
#define DEV_OUT DT_NODELABEL(gpioa)
#define PIN_OUT 0
#define PIN_IN 1

// def stacksize
#define STACKSIZE 500

// def thread and message queue structs
typedef struct k_thread k_thread_t;
typedef struct k_msgq k_msgq_t;

struct gpio_callback callback;
const struct device *dev_in, *dev_out;

//init thread to handle toggling
k_thread_t toggle_t;

//define toggle stack 
K_THREAD_STACK_DEFINE(toggle_s, STACKSIZE);

//define message queue: K_MSGQ_DEFINE(q_name, q_msg_size, q_max_msgs, q_align)
K_MSGQ_DEFINE(message, sizeof(int), 32, 4);


void pin_interrupt(const struct device *port,
                   struct gpio_callback *cb,
                   gpio_port_pins_t pins_)
{
    //gpio_pin_toggle(dev_out, PIN_OUT);

    //replace GPIO toggle with message containing toggle
    //k_msgq_put (struct k_msgq *msgq, const void *data, k_timeout_t timeout)
    k_msgq_put(&message, 1, K_FOREVER);
}


void message_handler(k_msgq_t *messages)
{
    while (1)
    {
        int i;
        //k_msgq_get (struct k_msgq *msgq, void *data, k_timeout_t timeout)
        //recieve message from queue, assign loop number to data 
        k_msgq_get(messages, i, K_FOREVER);

        //toggle GPIO
        gpio_pin_toggle(dev_out, PIN_OUT);
    }
}

void interrupt_main(void)
{
    //create toggle_t thread in toggle_s stack, run with message_handler(&message)
    k_thread_create(&toggle_t,
                    toggle_s,
                    STACKSIZE,
                    (k_thread_entry_t) message_handler,
                    &message,
                    NULL,
                    NULL,
                    K_PRIO_COOP(7),
                    0,
                    K_NO_WAIT);


    dev_in = device_get_binding(DT_LABEL(DEV_IN));
    dev_out = device_get_binding(DT_LABEL(DEV_OUT));

    gpio_pin_configure(dev_out, PIN_OUT, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(dev_in, PIN_IN, GPIO_INPUT);
    gpio_pin_interrupt_configure(dev_in, PIN_IN, GPIO_INT_EDGE_RISING);
    gpio_init_callback(&callback, pin_interrupt, 1 << PIN_IN);
    gpio_add_callback(dev_in, &callback);
    k_sleep(K_FOREVER);
}

