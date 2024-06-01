 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "hardware/adc.h"

//connect all pins to S0 to S3 or it will fail
int bits[4] = {0, 1, 2, 3};
int sig = 31;

int main() {
  stdio_init_all();

  for (int i = 0; i<4; i++) {
    gpio_init(bits[i]);
    gpio_set_dir(bits[i], GPIO_OUT);
    gpio_put(bits[i], 0);
  }
  adc_init();
  adc_gpio_init(sig);
  adc_select_input(0);
  
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, 1);
  
  
  const float conversion_factor = 3.3f / (1 << 12);
  while (true) {
    gpio_put(0, 0); //select channel 0
    uint16_t result0 = adc_read();
    gpio_put(0, 1); //select channel 1
    uint16_t result1 = adc_read();
    
    printf("%4d %4d\n", result0, result1);
    sleep_ms(5);
  }
}
