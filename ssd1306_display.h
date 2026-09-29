#ifndef SSD1306DISPLAY_H
#define SSD1306DISPLAY_H

#include <list>
#include <string>

bool SSD1306Display_Init(const char *i2c_dev);
bool SSD1306Display_Draw( unsigned char currentProgram, const std::list<std::string> &patchNameList);


#endif // SSD1306DISPLAY_H
