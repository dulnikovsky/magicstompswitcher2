# magicstompswitcher2 - Switch patches on Yamaha Magicstomp using a MIDI controller!

## Preface

The purpose of this application is to be able to switch patches of the YAMAHA Magicstomp using MIDI Program Change messages. The Magicstomp does not support this, it only supports MIDI SysEx messages, mainly for editing. Thus an application mapping Program Change messages to SysEx is neccessary.  The magicstompswitcher2 supports multiple Magicstomps, you can control as meny of them as you want! (I wish I had written this app while Allan Holdsworth was still with us, as he used six Magicstomps!).

This application is for Linux. Means: Every Linux system, from a PC to small Single Board Computer like Raspberry Pi can be used as hardware in order for this application to run. As there are many different distributions the easest way is to compile it on the target computer directly from the sources. It's not that hard, you only need to be familiar with Linux terminal and command prompt like bash. The application is a pure console application, however it can optionally be used with a 128x64 SSD1306 display connected via i2c.

## How to build and install magicstompswitcher2

This manual is for Linux distributions using apt package manager.

1. Log in to your Linux computer on console and make sure has internet connection.

2. Install required packages using apt:

    ```sudo apt install build-essential libasound2-dev cmake git```

3. If you want to use this app with a SSD1306 display go to the section "How to build and install libssd1306" and follow it. Otherwise skip it.

4. Clone the source code of magicstompswitcher2:

    ```git clone https://github.com/dulnikovsky/magicstompswitcher2```

5. Change to the magicstompswitcher2 directory:

    ```cd magicstompswitcher2```

6. Create the build directory:

    ```mkdir build```

7. Change to the build directory:

    ```cd build```

8. Run cmake:

    ```cmake -DCMAKE_BUILD_TYPE:STRING=Release ..```

    If building with the SSD1306 display support run:

    ```cmake -DWITH_SSD1306_DISPLAY:BOOL=ON -DCMAKE_BUILD_TYPE:STRING=Release ..```

9. Install the built executable to the /usr/local/bin directory:

    ```sudo cp magicstompswitcher2 /usr/local/bin```


## How to build and install libssd1306 

This section is only relevant if you want to build the magicstompswitcher2 with the SSD1306 support. Note: If using the display make sure i2c is enabled and the user running the app has the rights to access it.

1. Install required packages:

    ```sudo apt install libfreetype6-dev fonts-freefont-ttf ttf-bitstream-vera autoconf automake libtool autotools-dev pkg-config i2c-tools```

2. Clone the source code of libssd1306:

    ```git clone https://github.com/stealthylabs/libssd1306.git```

3. Change to the libssd1306 directory:

    ```cd libssd1306```

4. Build the library using the commands:

    ```./autogen.sh```

    ```./configure```

    ```make```

5. Install the the library (to /usr/local/lib and /usr/local/include):

    ```sudo make install```

6. Refresh system lib cache:

    ```sudo ldconfig```

6. Go back to the previous directory:

    ```cd ..```

## How to use magicstompswitcher2 
After the installation the app can be run in the terminal, just type magicstompswitcher2. After the start it scans the connected Magicstomps and copies the user patches from them into the memory. It takes about one minute. After that its ready to use. Any incoming Program Change masseges from a connected hardware MIDI controller will result in loading according user patch into the connected Magicstomp(s).  
  
By default it's working in OMNI mode, means it takes PC messeges on every MIDI Channel. If you want to use a partucular MIDI channel only you need to run the app with the argument "-c" and the number of the MIDI Channel. For example to run it on MIDI channel 4:  
    ```magicstompswitcher2 -c 4```




