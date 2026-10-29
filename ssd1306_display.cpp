
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
