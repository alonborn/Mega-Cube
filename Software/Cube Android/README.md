# Cube Android

Native Android controller written in Kotlin and Jetpack Compose. It scans for
the Mega Cube BLE service, connects to the ESP32, and sends animation,
playlist, and brightness commands.

## Build from VS Code

After installing the Android command-line SDK and Gradle wrapper:

```bash
./gradlew assembleDebug
./gradlew installDebug
```

Enable Developer options and USB debugging on the phone before `installDebug`.

