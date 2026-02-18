package com.minimal.reminders;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import java.util.Calendar;
import java.util.List;

public class AlarmScheduler {

    public static void schedule(Context ctx, Reminder r) {
        if (!r.enabled) {
            cancel(ctx, r);
            return;
        }

        AlarmManager am = (AlarmManager) ctx.getSystemService(Context.ALARM_SERVICE);
        if (am == null) return;

        // Schedule one alarm for each active day
        for (int day = 0; day < 7; day++) {
            if (!r.days[day]) {
                cancelDay(ctx, r.id, day);
                continue;
            }

            int requestCode = r.id * 10 + day;
            Intent intent = new Intent(ctx, AlarmReceiver.class);
            intent.putExtra("reminder_id", r.id);
            intent.putExtra("reminder_name", r.name);

            PendingIntent pi = PendingIntent.getBroadcast(ctx, requestCode, intent,
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

            Calendar cal = getNextAlarmTime(r.hour, r.minute, day);

            try {
                am.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, cal.getTimeInMillis(), pi);
            } catch (SecurityException e) {
                // Fallback if exact alarms not permitted
                am.set(AlarmManager.RTC_WAKEUP, cal.getTimeInMillis(), pi);
            }
        }
    }

    public static void cancel(Context ctx, Reminder r) {
        for (int day = 0; day < 7; day++) {
            cancelDay(ctx, r.id, day);
        }
    }

    private static void cancelDay(Context ctx, int reminderId, int day) {
        AlarmManager am = (AlarmManager) ctx.getSystemService(Context.ALARM_SERVICE);
        if (am == null) return;
        int requestCode = reminderId * 10 + day;
        Intent intent = new Intent(ctx, AlarmReceiver.class);
        PendingIntent pi = PendingIntent.getBroadcast(ctx, requestCode, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        am.cancel(pi);
    }

    // day: 0=Mon..6=Sun -> Calendar: Mon=2..Sun=1
    private static Calendar getNextAlarmTime(int hour, int minute, int dayIndex) {
        int[] calDays = {Calendar.MONDAY, Calendar.TUESDAY, Calendar.WEDNESDAY,
                Calendar.THURSDAY, Calendar.FRIDAY, Calendar.SATURDAY, Calendar.SUNDAY};

        Calendar now = Calendar.getInstance();
        Calendar cal = Calendar.getInstance();
        cal.set(Calendar.HOUR_OF_DAY, hour);
        cal.set(Calendar.MINUTE, minute);
        cal.set(Calendar.SECOND, 0);
        cal.set(Calendar.MILLISECOND, 0);
        cal.set(Calendar.DAY_OF_WEEK, calDays[dayIndex]);

        // If this time already passed this week, schedule for next week
        if (cal.before(now) || cal.equals(now)) {
            cal.add(Calendar.WEEK_OF_YEAR, 1);
        }

        return cal;
    }

    public static void scheduleSnooze(Context ctx, int reminderId, String name) {
        AlarmManager am = (AlarmManager) ctx.getSystemService(Context.ALARM_SERVICE);
        if (am == null) return;

        int requestCode = reminderId * 10 + 8; // 8 = snooze slot
        Intent intent = new Intent(ctx, AlarmReceiver.class);
        intent.putExtra("reminder_id", reminderId);
        intent.putExtra("reminder_name", name);

        PendingIntent pi = PendingIntent.getBroadcast(ctx, requestCode, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        long triggerAt = System.currentTimeMillis() + 10 * 60 * 1000; // 10 minutes

        try {
            am.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, triggerAt, pi);
        } catch (SecurityException e) {
            am.set(AlarmManager.RTC_WAKEUP, triggerAt, pi);
        }
    }

    public static void rescheduleAll(Context ctx) {
        ReminderStore store = new ReminderStore(ctx);
        List<Reminder> all = store.loadAll();
        for (Reminder r : all) {
            schedule(ctx, r);
        }
    }
}
