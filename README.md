# qt5-kobo-platform-plugin

```diff
- This fork no longer has any reasons to be. 
- This used to be my sandbox trying to fix some issues it had in the past.
- Issues has been fixed by now. So yeah ... That's it, go look at the original one instead.
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
