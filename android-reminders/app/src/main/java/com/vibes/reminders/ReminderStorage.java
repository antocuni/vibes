package com.vibes.reminders;

import android.content.Context;
import android.content.SharedPreferences;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.List;

public class ReminderStorage {
    private static final String PREFS_NAME = "reminders_prefs";
    private static final String KEY_REMINDERS = "reminders";

    private final SharedPreferences prefs;

    public ReminderStorage(Context context) {
        prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    }

    public List<Reminder> loadAll() {
        List<Reminder> list = new ArrayList<>();
        String json = prefs.getString(KEY_REMINDERS, "[]");
        try {
            JSONArray arr = new JSONArray(json);
            for (int i = 0; i < arr.length(); i++) {
                list.add(Reminder.fromJson(arr.getJSONObject(i)));
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return list;
    }

    public void saveAll(List<Reminder> list) {
        JSONArray arr = new JSONArray();
        try {
            for (Reminder r : list) {
                arr.put(r.toJson());
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        prefs.edit().putString(KEY_REMINDERS, arr.toString()).apply();
    }

    public Reminder findById(long id) {
        for (Reminder r : loadAll()) {
            if (r.id == id) return r;
        }
        return null;
    }

    public void save(Reminder reminder) {
        List<Reminder> list = loadAll();
        boolean found = false;
        for (int i = 0; i < list.size(); i++) {
            if (list.get(i).id == reminder.id) {
                list.set(i, reminder);
                found = true;
                break;
            }
        }
        if (!found) list.add(reminder);
        saveAll(list);
    }

    public void delete(long id) {
        List<Reminder> list = loadAll();
        for (int i = 0; i < list.size(); i++) {
            if (list.get(i).id == id) {
                list.remove(i);
                break;
            }
        }
        saveAll(list);
    }
}
