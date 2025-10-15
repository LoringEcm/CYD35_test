# CYD35_test

Initial development/test of the Cheap Yellow Display (CYD) for the ESP-3248S035 (3.5" screen) board, which exercises the display graphics, Touch screen interface, and the Wireless interface (2.4G) This project was developed on Windows 11 using the following directory structure:
```
any-folder/
    /cyd35_test2
    /libraries
      /TFT_eSPI
      /AsyncTCP
      /ESPAsyncWebServer
      /gt911-arduino
 ```
The PlatformIO '.ini' file is configured to run in this structure.

Only the four libraries shown are required - no other libraries are used for this demo. Note that the library folder names have removed the version from the name (such as 'master'). 

The folder references are relative such that these folders can be located anywhere in a directory hirearchy so long as their relative locations are maintained.  Users with libraries installed in a different directory structure (ie, 'elsewhere') must edit the 'symlink' definitions in the 'lib_deps' section of this projects 'platformio.ini' file. As always, following any changes to the 'platformio.ini' file the user should run a project 'clean' (via pull-down in the upper-right corner of the IDE) and rebuild of the project.

All four libraries were the latest versions (ie, 'master') as of 10/15/25 downloaded from GitHub.

No other libraries are required.

Also, per the library instructions, the TFT_eSPI library requires the User_Setup.h file be customized to the users specific hardware (I recommend renaming the old version in case other areas of the file are changed in upcoming releases), and should be installed at (replacing the existing copy already present):

```
any-folder/
    /libraries
      /TFT_eSPI/User_Setup.h
```

The 'docs' project folder contains a working version of the User_Setup.h file tailored to the 3.5" CYD. Porting to one of the other CYDs (ie, the 2.8" CYD) should be straightforward.

The libraries are built external to the project, so that they will only be built once at installation.  However, if you perform a PlatformIO 'clean' ('clean' on the upper-right pull-down) they will (again, once) be re-built within the project.

A compressed (*.zip) copy of a 'clean' version of the project folder ('cyd35_test2') results in a (ZIP) file of 52K (since the library compiled object code isn't present following the 'clean').

The code will initialize the CYD screen showing two circles (one 'filled' and one not), as well as two buttons labeled 'Press #1' and 'WebServer'.  Pressing anywhere on the screen will output the x:y coordinates of the 'touch' on the PlatformIO serial monitor (when running with the PlatformIO serial monitor active). 

If the user button 'Press #1' is pressed the a 'btn1' message appears in the serial monitor showing the x:y coordinates of the press as well as the top/bottom/left/right boundaries of the 'Press #1' button.

If the user button 'WebServer' is pressed on the CYD screen the button metrics are shown on the PlatformIO serial monitor screen as above, and the user  screen changes to display the current status of the 3 webserver buttons and slider, and the (2.4G wireless) webserver is started propagating an SSID of 'ESP32_Webserver'. The default IP address is 192.168.4.1 (http only, https will not connect), and the default password is 'password'. The user must connect their computer to the webserver wireless via their (computer specific) Internet connection API. Once connected any browser may be used to attach to the webpage at 192.168.4.1.  This page shows three buttons and a slider. The buttons may be clicked on/off and the slider graphic may be slid over the 0-255 range.  Updates are once-per-second.  All actions performed on the web page are echoed on the CYD status screen (in this mode the user 'touch' is disabled on the CYD).  Pressing either of the top two 'buttonWsSamp' buttons simply updates their status on both the webpage and CYD. Updating the slider via the graphic likewise updates both screens.

Pressing the webpage 'WebServerExit' button causes the webpage processing to be disabled and restores the CYD GUI.

Toggling between the two screens may be repeated as often as desired. The wireless connection will sustain over multiple switches of the displays.

The ESP32-3248S035 screen resolution is 320 x 480

The screen is configured in landscape mode, as:
```
  TFT API - rotation 'inverted' (ie, '1')
  GT911 API - rotation is 'ROTATION_RIGHT'
```

These settings result in the display configured in Landscape mode with a point of origin 
at the upper-left corner:
```
    the X axis counts RIGHT horizontally from 0 -> 480
    the Y axis counts DOWN  vertically   from 0 -> 320
```

