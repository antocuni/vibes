package com.minimal.reminders;

import org.json.JSONException;
import org.json.JSONObject;

public class Reminder {
    public int id;
    public String name;
    public int hour;
    public int minute;
    public boolean[] days; // 0=Mon, 1=Tue, 2=Wed, 3=Thu, 4=Fri, 5=Sat, 6=Sun
    public boolean enabled;

    public Reminder() {
        days = new boolean[7];
        enabled = true;
    }

    public String getDaysText() {
        boolean allDays = true;
        boolean noDays = true;
        for (boolean d : days) {
            if (!d) allDays = false;
            if (d) noDays = false;
        }
        if (allDays) return "Every day";
        if (noDays) return "No days selected";

        String[] names = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
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
        StringBuilder daysStr = new StringBuilder();
        for (boolean d : days) daysStr.append(d ? "1" : "0");
        obj.put("days", daysStr.toString());
        return obj;
    }

    public static Reminder fromJson(JSONObject obj) throws JSONException {
        Reminder r = new Reminder();
        r.id = obj.getInt("id");
        r.name = obj.getString("name");
        r.hour = obj.getInt("hour");
        r.minute = obj.getInt("minute");
        r.enabled = obj.optBoolean("enabled", true);
        String daysStr = obj.getString("days");
        for (int i = 0; i < 7 && i < daysStr.length(); i++) {
            r.days[i] = daysStr.charAt(i) == '1';
        }
        return r;
    }
}
