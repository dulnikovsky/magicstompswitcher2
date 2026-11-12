/****************************************************************************
**
** Copyright (C) 2026 Robert Vetter.
**
** This file is part of the MagicstompSwitcher2 - an utilitu for MIDI control
** of the YAMAHA Magicstomp
**
** THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
** ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
** IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
** PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version . The licenses are
** as published by the Free Software Foundation and appearing in the file LICENSE
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**/
#include "ssd1306_display.h"
#include <ssd1306_i2c.h>
#include <stdio.h>

#include <thread>
#include <mutex>
#include <condition_variable>

static ssd1306_i2c_t *handle{nullptr};
static ssd1306_framebuffer_t *fbp{nullptr};
static FILE *logFile{nullptr};
static std::mutex mtx;
static std::condition_variable cv;
static bool updateDisplay = false;
static bool shouldExit = false;
static std::thread backgroundWorker;

void workerThreadFunc() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        // Sleep until main thread signals conditionMet or shouldExit
        cv.wait(lock, []{ return updateDisplay || shouldExit; });

        if (shouldExit)
            break;

        ssd1306_i2c_display_update(handle, fbp);
        updateDisplay = false;
    }
}

bool SSD1306Display_Init(const char *i2c_dev)
{
    if (handle != nullptr)
        return false;

    logFile = fopen("/dev/null", "w");

    handle = ssd1306_i2c_open(i2c_dev, 0x3c, 128, 64, logFile);
    if (handle == nullptr)
        return false;

    if (ssd1306_i2c_display_initialize(handle) < 0) {
        ssd1306_i2c_close(handle);
        return false;
    }
    fbp = ssd1306_framebuffer_create(handle->width, handle->height, handle->err);
    ssd1306_i2c_display_clear(handle);

    ssd1306_framebuffer_draw_text(fbp, "Magicstomp", 10, 0, 24, SSD1306_FONT_DEFAULT, 4, NULL);
    ssd1306_framebuffer_draw_text(fbp, "Switcher 2.0", 8, 0, 48, SSD1306_FONT_DEFAULT, 4, NULL);

    ssd1306_i2c_display_update(handle, fbp);

    backgroundWorker = std::thread(workerThreadFunc);

    return true;
}

bool SSD1306Display_Draw( unsigned char currentProgram, const std::list<std::string> &patchNameList)
{
    ssd1306_framebuffer_clear(fbp);

    char prgArr[2];
    prgArr[0] = '0' + currentProgram/10;
    prgArr[1] = '0' + currentProgram%10;

    ssd1306_framebuffer_box_t bbox;
    ssd1306_framebuffer_draw_text(fbp, prgArr, 2, 0, 24, SSD1306_FONT_VERA_BOLD, 6, &bbox);

    uint8_t x = 0;
    uint8_t y = bbox.bottom+16;

    for ( const std::string& patchName : patchNameList) {
        ssd1306_framebuffer_draw_text(fbp, patchName.c_str(), patchName.size(), x, y, SSD1306_FONT_DEFAULT, 4, &bbox);
        y += 16;
    }

    updateDisplay = true;
    cv.notify_one();

    return true;
}
