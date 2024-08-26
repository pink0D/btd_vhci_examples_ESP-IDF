#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "PS4BT.h"
#include "btd_vhci.h"

PS4BT PS4;

bool printAngle, printTouch;
uint8_t oldL2Value, oldR2Value;

static const char *LOG_TAG = "main";

// print controller status
void ps4_print() {
    if (PS4.connected()) {
        if (PS4.getAnalogHat(LeftHatX) > 137 || PS4.getAnalogHat(LeftHatX) < 117 || PS4.getAnalogHat(LeftHatY) > 137 || PS4.getAnalogHat(LeftHatY) < 117 || PS4.getAnalogHat(RightHatX) > 137 || PS4.getAnalogHat(RightHatX) < 117 || PS4.getAnalogHat(RightHatY) > 137 || PS4.getAnalogHat(RightHatY) < 117) {
        ESP_LOGI(LOG_TAG, "L_x = %d, L_y = %d, R_x = %d, R_y = %d",
                    PS4.getAnalogHat(LeftHatX),PS4.getAnalogHat(LeftHatY),
                    PS4.getAnalogHat(RightHatX),PS4.getAnalogHat(RightHatY));
        }

        if (PS4.getAnalogButton(L2) || PS4.getAnalogButton(R2)) { // These are the only analog buttons on the PS4 controller
        ESP_LOGI(LOG_TAG, "L2 = %d, R2 = %d",PS4.getAnalogButton(L2),PS4.getAnalogButton(R2));
        }
        if (PS4.getAnalogButton(L2) != oldL2Value || PS4.getAnalogButton(R2) != oldR2Value) // Only write value if it's different
        PS4.setRumbleOn(PS4.getAnalogButton(L2), PS4.getAnalogButton(R2));
        oldL2Value = PS4.getAnalogButton(L2);
        oldR2Value = PS4.getAnalogButton(R2);

        if (PS4.getButtonClick(PS))
        ESP_LOGI(LOG_TAG, "PS");
        if (PS4.getButtonClick(TRIANGLE)) {
        ESP_LOGI(LOG_TAG, "Triangle");
        PS4.setRumbleOn(RumbleLow);
        }
        if (PS4.getButtonClick(CIRCLE)) {
        ESP_LOGI(LOG_TAG, "Circle");
        PS4.setRumbleOn(RumbleHigh);
        }
        if (PS4.getButtonClick(CROSS)) {
        ESP_LOGI(LOG_TAG, "Cross");
        PS4.setLedFlash(10, 10); // Set it to blink rapidly
        }
        if (PS4.getButtonClick(SQUARE)) {
        ESP_LOGI(LOG_TAG, "Square");
        PS4.setLedFlash(0, 0); // Turn off blinking
        }

        if (PS4.getButtonClick(UP)) {
        ESP_LOGI(LOG_TAG, "UP");
        PS4.setLed(Red);
        } if (PS4.getButtonClick(RIGHT)) {
        ESP_LOGI(LOG_TAG, "RIGHT");
        PS4.setLed(Blue);
        } if (PS4.getButtonClick(DOWN)) {
        ESP_LOGI(LOG_TAG, "DOWN");
        PS4.setLed(Yellow);
        } if (PS4.getButtonClick(LEFT)) {
        ESP_LOGI(LOG_TAG, "LEFT");
        PS4.setLed(Green);
        }

        if (PS4.getButtonClick(L1))
        ESP_LOGI(LOG_TAG, "L1");
        if (PS4.getButtonClick(L3))
        ESP_LOGI(LOG_TAG, "L3");
        if (PS4.getButtonClick(R1))
        ESP_LOGI(LOG_TAG, "R1");
        if (PS4.getButtonClick(R3))
        ESP_LOGI(LOG_TAG, "R3");

        if (PS4.getButtonClick(SHARE))
        ESP_LOGI(LOG_TAG, "SHARE");
        if (PS4.getButtonClick(OPTIONS)) {
        ESP_LOGI(LOG_TAG, "OPTIONS");
        printAngle = !printAngle;
        }
        if (PS4.getButtonClick(TOUCHPAD)) {
        ESP_LOGI(LOG_TAG, "TOUCHPAD");
        printTouch = !printTouch;
        }

        if (printAngle) { // Print angle calculated using the accelerometer only
        ESP_LOGI(LOG_TAG,"Pitch: %lf Roll: %lf", PS4.getAngle(Pitch), PS4.getAngle(Roll));        
        }

        if (printTouch) { // Print the x, y coordinates of the touchpad
            if (PS4.isTouching(0) || PS4.isTouching(1)) // Print newline and carriage return if any of the fingers are touching the touchpad
                ESP_LOGI(LOG_TAG, "");
            for (uint8_t i = 0; i < 2; i++) { // The touchpad track two fingers
                if (PS4.isTouching(i)) { // Print the position of the finger if it is touching the touchpad
                ESP_LOGI(LOG_TAG, "X = %d, Y = %d",PS4.getX(i),PS4.getY(i));          
                }
            }
        }
    }
}

void ps4_loop_task(void *task_params) {
    while (1) { 
        btd_vhci_mutex_lock();      // lock mutex so controller's data is not updated meanwhile
        ps4_print();                // print PS4 status
        btd_vhci_mutex_unlock();    // unlock mutex
        vTaskDelay(1);
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret;

    // initialize flash
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // initilize the library
    ret = btd_vhci_init();
    if (ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "BTD init error!");
    }
    ESP_ERROR_CHECK( ret );

    // run example code
    xTaskCreatePinnedToCore(ps4_loop_task,"ps4_loop_task",10*1024,NULL,2,NULL,1);

    // run auto connect task
    btd_vhci_autoconnect(&PS4);

    while (1) {       
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // main task should not return
}
