package com.vibes.reminders;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import java.lang.reflect.Method;
import java.util.Calendar;

public class AlarmScheduler {
    // PendingIntent.FLAG_IMMUTABLE added in API 31
    private static final int FLAG_IMMUTABLE = 0x04000000;

    public static void schedule(Context context, Reminder reminder) {
        if (!reminder.enabled) {
            cancel(context, reminder);
            return;
        }

        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        if (am == null) return;

        for (int i = 0; i < 7; i++) {
            if (reminder.days[i]) {
                int requestCode = getRequestCode(reminder.id, i);
                PendingIntent pi = createPendingIntent(context, reminder, requestCode);

                Calendar cal = getNextAlarmTime(reminder.hour, reminder.minute, i);

                scheduleExact(am, cal.getTimeInMillis(), pi);
            } else {
                cancelDay(context, reminder.id, i);
            }
        }
    }

    public static void scheduleSnooze(Context context, long reminderId) {
        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        if (am == null) return;

        int requestCode = (int) (reminderId % Integer.MAX_VALUE) + 1000;
        Intent intent = new Intent(context, AlarmReceiver.class);
        intent.putExtra("reminder_id", reminderId);
        intent.putExtra("is_snooze", true);

        int piFlags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= 31) piFlags |= FLAG_IMMUTABLE;
        PendingIntent pi = PendingIntent.getBroadcast(context, requestCode, intent, piFlags);

        long snoozeTime = System.currentTimeMillis() + 10 * 60 * 1000; // 10 minutes
        scheduleExact(am, snoozeTime, pi);
    }

    private static void scheduleExact(AlarmManager am, long triggerAtMillis, PendingIntent pi) {
        if (Build.VERSION.SDK_INT >= 31) {
            // API 31+: check canScheduleExactAlarms via reflection
            try {
                Method canSchedule = AlarmManager.class.getMethod("canScheduleExactAlarms");
                Boolean can = (Boolean) canSchedule.invoke(am);
                if (can != null && can) {
                    am.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, triggerAtMillis, pi);
                } else {
                    am.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, triggerAtMillis, pi);
                }
            } catch (Exception e) {
                am.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, triggerAtMillis, pi);
            }
        } else if (Build.VERSION.SDK_INT >= 23) {
            am.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, triggerAtMillis, pi);
        } else {
            am.setExact(AlarmManager.RTC_WAKEUP, triggerAtMillis, pi);
        }
    }

    public static void cancel(Context context, Reminder reminder) {
        for (int i = 0; i < 7; i++) {
            cancelDay(context, reminder.id, i);
        }
    }

    private static void cancelDay(Context context, long reminderId, int dayIndex) {
        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        if (am == null) return;
        int requestCode = getRequestCode(reminderId, dayIndex);
        Intent intent = new Intent(context, AlarmReceiver.class);

        int piFlags = PendingIntent.FLAG_NO_CREATE;
        if (Build.VERSION.SDK_INT >= 31) piFlags |= FLAG_IMMUTABLE;
        PendingIntent pi = PendingIntent.getBroadcast(context, requestCode, intent, piFlags);
        if (pi != null) {
            am.cancel(pi);
            pi.cancel();
        }
    }

    private static PendingIntent createPendingIntent(Context context, Reminder reminder, int requestCode) {
        Intent intent = new Intent(context, AlarmReceiver.class);
        intent.putExtra("reminder_id", reminder.id);
        intent.putExtra("is_snooze", false);

        int piFlags = PendingIntent.FLAG_UPDATE_CURRENT;
        if (Build.VERSION.SDK_INT >= 31) piFlags |= FLAG_IMMUTABLE;
        return PendingIntent.getBroadcast(context, requestCode, intent, piFlags);
    }

    static Calendar getNextAlarmTime(int hour, int minute, int dayIndex) {
        // dayIndex: 0=Mon..6=Sun -> Calendar: Mon=2..Sun=1
        int calDay;
        if (dayIndex == 6) calDay = Calendar.SUNDAY;
        else calDay = dayIndex + 2;

        Calendar now = Calendar.getInstance();
        Calendar cal = Calendar.getInstance();
        cal.set(Calendar.HOUR_OF_DAY, hour);
        cal.set(Calendar.MINUTE, minute);
        cal.set(Calendar.SECOND, 0);
        cal.set(Calendar.MILLISECOND, 0);
        cal.set(Calendar.DAY_OF_WEEK, calDay);

        if (cal.before(now)) {
            cal.add(Calendar.WEEK_OF_YEAR, 1);
        }

        return cal;
    }

    private static int getRequestCode(long reminderId, int dayIndex) {
        return (int) ((reminderId % 100000) * 10 + dayIndex);
    }

    public static void rescheduleAll(Context context) {
        ReminderStorage storage = new ReminderStorage(context);
        for (Reminder r : storage.loadAll()) {
            if (r.enabled) schedule(context, r);
        }
    }
}
