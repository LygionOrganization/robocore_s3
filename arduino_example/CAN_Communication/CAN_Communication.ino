// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// No external libraries required; all headers are included with the ESP32 board package.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <driver/twai.h>
bool ready = false;
void setup() {
  Serial0.begin(115200);
  // Set the physical selector to CAN. CAN and external RS485 share RX13/TX14.
  // GPIO38 drives the CAN transceiver slope-control input; LOW selects normal speed.
  pinMode(38, OUTPUT);
  digitalWrite(38, LOW);
  twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_14, GPIO_NUM_13, TWAI_MODE_NORMAL);
  twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  ready = twai_driver_install(&general, &timing, &filter) == ESP_OK;
  if (ready) ready = twai_start() == ESP_OK;
  Serial0.println("Selector: CAN. Use CAN 2+2, another 500 kbps node and common ground.");
  Serial0.println("This board has 120 ohm termination. Check termination across the full bus.");
  Serial0.println("Send t to transmit ID 0x100 with two bytes.");
}
void loop() {
  if (!ready) { delay(100); return; }
  if (Serial0.read() == 't') {
    twai_message_t tx = {};
    tx.identifier = 0x100; tx.data_length_code = 2; tx.data[0] = 0x12; tx.data[1] = 0x34;
    Serial0.printf("Transmit result=%d\n", twai_transmit(&tx, pdMS_TO_TICKS(100)));
  }
  twai_message_t rx = {};
  if (twai_receive(&rx, 0) == ESP_OK) {
    Serial0.printf("CAN ID=%lx DLC=%u", rx.identifier, rx.data_length_code);
    for (int i = 0; i < rx.data_length_code && i < 8; ++i) Serial0.printf(" %02x", rx.data[i]);
    Serial0.println();
  }
  delay(1);
}
