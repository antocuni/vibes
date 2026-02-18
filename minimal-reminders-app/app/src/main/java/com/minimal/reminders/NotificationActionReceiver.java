package com.minimal.reminders;

import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class NotificationActionReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context ctx, Intent intent) {
        int notifId = intent.getIntExtra("notification_id", -1);
        String action = intent.getAction();

        // Dismiss notification
        NotificationManager nm = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
        if (nm != null && notifId != -1) {
            nm.cancel(notifId);
        }

        if ("SNOOZE".equals(action)) {
            int reminderId = intent.getIntExtra("reminder_id", -1);
            String name = intent.getStringExtra("reminder_name");
            if (reminderId != -1) {
                AlarmScheduler.scheduleSnooze(ctx, reminderId, name);
            }
        }
    }
}
