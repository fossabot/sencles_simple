// Copyright 2021-2022 Espressif Systems (Shanghai) PTE LTD
// Copyright 2022 JeongYeham
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Create LCD panel for model GC9A01
     *
     * @param[in] io LCD panel IO handle
     * @param[in] panel_dev_config general panel device configuration
     * @param[out] ret_panel Returned LCD panel handle
     * @return
     *          - ESP_ERR_INVALID_ARG   if parameter is invalid
     *          - ESP_ERR_NO_MEM        if out of memory
     *          - ESP_OK                on success
     */
    esp_err_t esp_lcd_new_panel_gc9a01(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

    void gui_task(void *arg);

#ifdef __cplusplus
}
#endif
