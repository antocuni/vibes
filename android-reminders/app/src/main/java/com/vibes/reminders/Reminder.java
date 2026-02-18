package com.vibes.reminders;

import org.json.JSONException;
import org.json.JSONObject;

public class Reminder {
    public long id;
    public String name;
    public int hour;
    public int minute;
    public boolean[] days; // 0=Mon, 1=Tue, ..., 6=Sun
    public boolean enabled;

    public Reminder() {
        id = System.currentTimeMillis();
        name = "";
        hour = 8;
        minute = 0;
        days = new boolean[]{true, true, true, true, true, true, true};
        enabled = true;
    }

    public String getDaysText() {
        String[] names = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        boolean allTrue = true;
        boolean allFalse = true;
        boolean weekdays = true;
        boolean weekends = true;

        for (int i = 0; i < 7; i++) {
            if (days[i]) allFalse = false;
            else allTrue = false;
        }
        for (int i = 0; i < 5; i++) {
            if (!days[i]) weekdays = false;
        }
        for (int i = 5; i < 7; i++) {
            if (!days[i]) weekends = false;
        }

        if (allTrue) return "Every day";
        if (allFalse) return "No days selected";
        if (weekdays && !days[5] && !days[6]) return "Weekdays";
        if (weekends && !days[0] && !days[1] && !days[2] && !days[3] && !days[4]) return "Weekends";

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 7; i++) {
            if (days[i]) {
                if (sb.length() > 0) sb.append(", ");
                sb.append(names[i]);
            }
        }
        return sb.toString();
    }

    public String getTimeText() {
        return String.format("%02d:%02d", hour, minute);
    }

    public JSONObject toJson() throws JSONException {
        JSONObject obj = new JSONObject();
        obj.put("id", id);
        obj.put("name", name);
        obj.put("hour", hour);
        obj.put("minute", minute);
        obj.put("enabled", enabled);
        StringBuilder dayStr = new StringBuilder();
        for (boolean d : days) dayStr.append(d ? "1" : "0");
        obj.put("days", dayStr.toString());
        return obj;
    }

    public static Reminder fromJson(JSONObject obj) throws JSONException {
        Reminder r = new Reminder();
        r.id = obj.getLong("id");
        r.name = obj.getString("name");
        r.hour = obj.getInt("hour");
        r.minute = obj.getInt("minute");
        r.enabled = obj.getBoolean("enabled");
        String dayStr = obj.getString("days");
        for (int i = 0; i < 7 && i < dayStr.length(); i++) {
            r.days[i] = dayStr.charAt(i) == '1';
        }
        return r;
    }
}
