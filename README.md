# ThunderCube

Screensaver is a Thunder plugin that renders a simpel EGL cube using the [compositorclient](https://github.com/rdkcentral/ThunderClientLibraries/tree/master/Source/compositorclient).

## Build

The plugin is a CMake-based project built like all other existing Thunder plugins 

Options:

1. ```PLUGIN_CUBE_AUTOSTART```: Automatically start the plugin when Thunder starts; default: ```true```

## JSONRPC API
### Pause Rendering
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Screensaver' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Screensaver.1.pause",
    }'
```

### Resume Rendering
``` shell
curl --location --request POST 'http://<Thunder IP>/jsonrpc/Screensaver' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "jsonrpc": "2.0",
        "id": 42,
        "method": "Screensaver.1.resume",
    }'
```

## REST API
### Pause Rendering
``` shell
curl --request PUT 'http://<Thunder IP>/Screensaver/Resume'
```

### Resume Rendering
``` shell
curl --request PUT 'http://<Thunder IP>/Screensaver/Pause'
```
