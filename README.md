# qt5-kobo-platform-plugin

```diff
- This is my little sandbox trying to fix the Kobo Glo HD release events being ignored
```

A Qt5 platform backend plugin for Kobo E-Ink devices. \
Supports most of the Kobo platform functons including E-Ink screen refresh control,
input management for buttons and touchscreen, frontlight control, WiFi management and more. \
At runtime an app using this plugin can be configurend with the environment variable QT_QPA_PLATFORM. \
The following parameters are supported: 

debug -> enables additional debug output \
logicaldpitarget= -> enables high dpi scaling 

For example:
```
export QT_QPA_PLATFORM=kobo:debug:logicaldpitarget=108
./myapp
```


## Cross-compile for Kobo
See https://github.com/Rain92/kobo-qt-setup-script
