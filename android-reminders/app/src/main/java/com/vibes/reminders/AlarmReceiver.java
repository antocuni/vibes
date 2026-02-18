package com.vibes.reminders;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Build;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

public class AlarmReceiver extends BroadcastReceiver {
    private static final String CHANNEL_ID = "reminders_channel";
    // PendingIntent.FLAG_IMMUTABLE added in API 31
    private static final int FLAG_IMMUTABLE = 0x04000000;

    @Override
    public void onReceive(Context context, Intent intent) {
        long reminderId = intent.getLongExtra("reminder_id", -1);
        boolean isSnooze = intent.getBooleanExtra("is_snooze", false);

        ReminderStorage storage = new ReminderStorage(context);
        Reminder reminder = storage.findById(reminderId);
        if (reminder == null) return;

        createNotificationChannel(context);
        showNotification(context, reminder);

        if (!isSnooze) {
            AlarmScheduler.schedule(context, reminder);
        }
    }

    private void createNotificationChannel(Context context) {
        if (Build.VERSION.SDK_INT < 26) return; // NotificationChannel requires API 26+

        try {
            NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

            Method getChannel = NotificationManager.class.getMethod("getNotificationChannel", String.class);
            if (getChannel.invoke(nm, CHANNEL_ID) != null) return;

            Class<?> channelClass = Class.forName("android.app.NotificationChannel");
            // IMPORTANCE_HIGH = 4
            Constructor<?> ctor = channelClass.getConstructor(String.class, CharSequence.class, int.class);
            Object channel = ctor.newInstance(CHANNEL_ID,
                    context.getString(R.string.reminder_channel_name), 4);

            Method setDesc = channelClass.getMethod("setDescription", String.class);
            setDesc.invoke(channel, context.getString(R.string.reminder_channel_desc));

            Uri alarmSound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM);
            AudioAttributes aa = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_ALARM)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();

            Method setSound = channelClass.getMethod("setSound", Uri.class, AudioAttributes.class);
            setSound.invoke(channel, alarmSound, aa);

            Method enableVibration = channelClass.getMethod("enableVibration", boolean.class);
            enableVibration.invoke(channel, true);

            Method createChannel = NotificationManager.class.getMethod("createNotificationChannel", channelClass);
            createChannel.invoke(nm, channel);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @SuppressWarnings("deprecation")
    private void showNotification(Context context, Reminder reminder) {
        int notifId = (int) (reminder.id % Integer.MAX_VALUE);
        int piFlags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= 31) piFlags |= FLAG_IMMUTABLE;

        Intent dismissIntent = new Intent(context, DismissReceiver.class);
        dismissIntent.putExtra("notification_id", notifId);
        PendingIntent dismissPi = PendingIntent.getBroadcast(context,
                notifId + 2000, dismissIntent, piFlags);

        Intent snoozeIntent = new Intent(context, SnoozeReceiver.class);
        snoozeIntent.putExtra("reminder_id", reminder.id);
        snoozeIntent.putExtra("notification_id", notifId);
        PendingIntent snoozePi = PendingIntent.getBroadcast(context,
                notifId + 3000, snoozeIntent, piFlags);

        Intent contentIntent = new Intent(context, MainActivity.class);
        PendingIntent contentPi = PendingIntent.getActivity(context, 0, contentIntent, piFlags);

        Uri alarmSound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_ALARM);

        Notification.Builder builder;
        if (Build.VERSION.SDK_INT >= 26) {
            try {
                Constructor<Notification.Builder> ctor = Notification.Builder.class
                        .getConstructor(Context.class, String.class);
                builder = ctor.newInstance(context, CHANNEL_ID);
            } catch (Exception e) {
                builder = new Notification.Builder(context);
            }
        } else {
            builder = new Notification.Builder(context);
            builder.setSound(alarmSound);
            builder.setVibrate(new long[]{0, 500, 200, 500});
            builder.setPriority(Notification.PRIORITY_HIGH);
        }

        builder.setSmallIcon(android.R.drawable.ic_lock_idle_alarm)
                .setContentTitle(reminder.name)
                .setContentText(reminder.getTimeText() + " - " + reminder.getDaysText())
                .setContentIntent(contentPi)
                .setAutoCancel(true)
                .addAction(0, context.getString(R.string.dismiss), dismissPi)
                .addAction(0, context.getString(R.string.snooze), snoozePi);

        NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.notify(notifId, builder.build());
    }
}
