# qt5-kobo-platform-plugin
A Qt5 platform backend plugin for Kobo E-Ink devices. \
Supports most of the Kobo platform functons including E-Ink screen refresh control,
input management for buttons and touchscreen, frontlight control, WiFi management and more. \
At runtime an app using this plugin can be configurend with the environment variable QT_QPA_PLATFORM. \
The following parameters are supported: \
touchscreen_rotate=auto|0|90|180|270 -> 90 is portait mode for some devices but one others it might be different \
touchscreen_invert_x=auto|0|1 \
touchscreen_invert_y=auto|0|1 \
logicaldpitarget= -> enables high dpi scaling \ 

For example:
```
export QT_QPA_PLATFORM=kobo:touchscreen_rotate=auto:touchscreen_invert_x=auto:touchscreen_invert_y=auto:logicaldpitarget=108
./myapp
```


## Cross-compile for Kobo
See https://github.com/Rain92/UltimateMangaReader.
