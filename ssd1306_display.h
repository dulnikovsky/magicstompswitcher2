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
#ifndef SSD1306DISPLAY_H
#define SSD1306DISPLAY_H

#include <list>
#include <string>

bool SSD1306Display_Init(const char *i2c_dev);
bool SSD1306Display_Draw(unsigned char currentProgram, const std::list<std::string> &patchNameList);

#endif // SSD1306DISPLAY_H
