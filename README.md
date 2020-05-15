# qt5-kobo-platform-plugin
A Qt5 platform backend plugin for Kobo E-Ink devices. \
Supports most of the Kobo platform functons including E-Ink screen refresh control,
input management for buttons and touchscreen, frontlight control, WiFi management and more. \
At runtime an app using this plugin can be configurend with the environment variable QT_QPA_PLATFORM. \
Screen rotation can be controlled with the screen touchscreen_rotate parameter: 90 -> portrait mode. \
For high dpi scaling the logicaldpitarget can be used. 

For example:
```
export QT_QPA_PLATFORM=kobo:touchscreen_rotate=90:logicaldpitarget=108
./myapp
```


## Cross-compile for Kobo
See https://github.com/Rain92/UltimateMangaReader.
