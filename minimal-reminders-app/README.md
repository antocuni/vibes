# Minimal Reminders

A tiny Android reminders app with no external dependencies. Pure Java, XML layouts, and the Android SDK — nothing else.

## What it does

- Create, edit, and delete recurring reminders
- Each reminder has a name, time, and selected days of the week
- Triggers a notification with sound when a reminder fires
- Notification actions: Dismiss or Snooze (10 minutes)
- Reminders survive device reboots
- Enable/disable individual reminders with a toggle switch

## Implementation

- **No frameworks**: No Kotlin, no Jetpack, no Room, no Compose. Just `android.app.Activity`, `SharedPreferences`, and `AlarmManager`.
- **Storage**: Reminders stored as JSON in SharedPreferences
- **Alarms**: Uses `AlarmManager.setExactAndAllowWhileIdle()` for reliable triggering
- **Notifications**: Standard `Notification.Builder` with action buttons
- **Size**: Compiles to a ~50KB APK thanks to ProGuard minification and zero dependencies

## Usage

```bash
# Build debug APK
cd minimal-reminders-app
./gradlew assembleDebug

# Build release AAB (for Google Play)
./gradlew bundleRelease

# Install debug on connected device
./gradlew installDebug
```

## Key findings

- An Android app with zero dependencies compiles to an extremely small artifact
- `SharedPreferences` with JSON is sufficient for simple data storage — no need for Room/SQLite
- The built-in `android.app.Notification.Builder` handles everything needed without AndroidX NotificationCompat
