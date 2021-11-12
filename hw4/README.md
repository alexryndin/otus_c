# HW4

## How to build

```
meson build && ninja -C build
```
## How to play test file
```
GST_PLUGIN_PATH=build gst-launch-1.0 otushw4 filepath=test.wav ! "audio/x-raw,format=S16LE,rate=48000" ! autoaudiosink
```

