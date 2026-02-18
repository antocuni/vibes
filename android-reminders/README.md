# Android Reminders

A minimal, no-framework Android reminders app. Pure Java, compiled with bare SDK tools (aapt2 + dx + apksigner) — no Gradle, no Android Studio, no Kotlin, no Jetpack. The resulting APK is ~29KB.

## What it does

- List, create, edit, and delete reminders from a simple main screen
- Each reminder has a name, a time, and selectable days of the week (e.g. "every day", "Mon, Wed, Fri", "weekdays")
- When a reminder triggers, it shows a notification with sound
- Notification has **Dismiss** and **Snooze** (10 minutes) buttons
- Reminders persist across reboots (re-scheduled on BOOT_COMPLETED)
- Per-reminder enable/disable toggle

## Implementation

- **Zero dependencies** — only the Android SDK (`android.jar` API 23)
- **7 Java source files**, ~600 lines total
- Uses `SharedPreferences` with JSON for storage (no SQLite, no Room)
- Uses `AlarmManager.setExactAndAllowWhileIdle()` for reliable alarm delivery
- Newer APIs (NotificationChannel, exact alarm permissions) accessed via **reflection** to compile against API 23 while supporting up to API 34+
- Material Light theme with green accent, day-of-week toggle buttons
- Build script uses `aapt2` → `javac` → `dx` → `zipalign` → `apksigner`

## Project structure

```
app/src/main/
  AndroidManifest.xml
  java/com/vibes/reminders/
    Reminder.java           - Data model with JSON serialization
    ReminderStorage.java    - SharedPreferences-based persistence
    AlarmScheduler.java     - AlarmManager scheduling + snooze
    AlarmReceiver.java      - BroadcastReceiver for alarm + notification
    DismissReceiver.java    - Dismisses notification
    SnoozeReceiver.java     - Snoozes for 10 minutes
    BootReceiver.java       - Re-schedules alarms after reboot
    MainActivity.java       - Reminder list with enable/disable toggle
    EditReminderActivity.java - Add/edit/delete reminder form
  res/
    layout/                 - XML layouts for activities and list items
    values/                 - Strings, styles
    drawable/               - Vector icon, toggle backgrounds
build.sh                    - Complete build script (no Gradle needed)
```

## Building

Prerequisites (Ubuntu/Debian):
```bash
apt install android-sdk android-sdk-build-tools android-sdk-platform-23 dalvik-exchange
```

Build:
```bash
./build.sh
```

Output: `build/reminders.apk` (~29KB, signed)

## Play Store distribution

The current build produces a signed APK suitable for sideloading. For Google Play Store upload, you would need:

1. **An AAB (Android App Bundle)** — requires Google's `bundletool` which converts the APK to AAB format
2. **A proper release keystore** — the build script auto-generates one, but for Play Store you should use a persistent keystore and enroll in Play App Signing
3. **targetSdkVersion 34+** — already set in the manifest

To convert to AAB once you have `bundletool`:
```bash
java -jar bundletool.jar build-bundle --modules=base.zip --output=reminders.aab
```

## Key findings

- A fully functional Android app can be built in ~29KB with zero frameworks
- The old `dx` dex compiler doesn't support Java 8 lambdas (`invokedynamic`), requiring anonymous inner classes
- API 23's `android.jar` lacks NotificationChannel and other modern classes, but reflection works perfectly for runtime usage
- The entire build pipeline (resource compile → java compile → dex → zip → align → sign) takes about 2 seconds
