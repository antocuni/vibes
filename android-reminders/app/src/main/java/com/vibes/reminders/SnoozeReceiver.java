package com.vibes.reminders;

import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class SnoozeReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        long reminderId = intent.getLongExtra("reminder_id", -1);
        int notificationId = intent.getIntExtra("notification_id", -1);

        // Dismiss the notification
        if (notificationId != -1) {
            NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            nm.cancel(notificationId);
        }

        // Schedule snooze (10 minutes)
        if (reminderId != -1) {
            AlarmScheduler.scheduleSnooze(context, reminderId);
        }
    }
}
