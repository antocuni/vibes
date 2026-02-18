package com.minimal.reminders;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.RingtoneManager;
import android.net.Uri;

public class AlarmReceiver extends BroadcastReceiver {

    private static final String CHANNEL_ID = "reminders";

    @Override
    public void onReceive(Context ctx, Intent intent) {
        int id = intent.getIntExtra("reminder_id", -1);
        String name = intent.getStringExtra("reminder_name");
        if (name == null) name = "Reminder";

        createChannel(ctx);

        // Dismiss action
        Intent dismissIntent = new Intent(ctx, NotificationActionReceiver.class);
        dismissIntent.setAction("DISMISS");
        dismissIntent.putExtra("notification_id", id);
        PendingIntent dismissPi = PendingIntent.getBroadcast(ctx, id * 100 + 1, dismissIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        // Snooze action
        Intent snoozeIntent = new Intent(ctx, NotificationActionReceiver.class);
        snoozeIntent.setAction("SNOOZE");
        snoozeIntent.putExtra("notification_id", id);
        snoozeIntent.putExtra("reminder_id", id);
        snoozeIntent.putExtra("reminder_name", name);
        PendingIntent snoozePi = PendingIntent.getBroadcast(ctx, id * 100 + 2, snoozeIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        // Content intent - open the app
        Intent openIntent = new Intent(ctx, MainActivity.class);
        PendingIntent openPi = PendingIntent.getActivity(ctx, id * 100 + 3, openIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        Uri alarmSound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM);
        if (alarmSound == null) {
            alarmSound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
        }

        android.app.Notification notification = new android.app.Notification.Builder(ctx, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_lock_idle_alarm)
                .setContentTitle(name)
                .setContentText("Reminder triggered")
                .setContentIntent(openPi)
                .setAutoCancel(true)
                .setSound(alarmSound)
                .addAction(android.R.drawable.ic_menu_close_clear_cancel, "Dismiss", dismissPi)
                .addAction(android.R.drawable.ic_popup_reminder, "Snooze (10 min)", snoozePi)
                .setPriority(android.app.Notification.PRIORITY_HIGH)
                .setDefaults(android.app.Notification.DEFAULT_VIBRATE)
                .build();

        NotificationManager nm = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
        if (nm != null) {
            nm.notify(id, notification);
        }

        // Reschedule recurring alarm for next week
        ReminderStore store = new ReminderStore(ctx);
        Reminder r = store.findById(id);
        if (r != null) {
            AlarmScheduler.schedule(ctx, r);
        }
    }

    private void createChannel(Context ctx) {
        NotificationManager nm = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
        if (nm == null) return;

        Uri alarmSound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM);
        AudioAttributes audioAttr = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_ALARM)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();

        NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Reminders",
                NotificationManager.IMPORTANCE_HIGH);
        channel.setDescription("Reminder notifications");
        channel.setSound(alarmSound, audioAttr);
        channel.enableVibration(true);
        nm.createNotificationChannel(channel);
    }
}
