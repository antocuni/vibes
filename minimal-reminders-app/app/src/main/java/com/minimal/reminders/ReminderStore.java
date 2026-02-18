package com.minimal.reminders;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONArray;
import org.json.JSONException;

import java.util.ArrayList;
import java.util.List;

public class ReminderStore {
    private static final String PREFS = "reminders";
    private static final String KEY = "data";
    private static final String KEY_NEXT_ID = "next_id";

    private final SharedPreferences prefs;

    public ReminderStore(Context ctx) {
        prefs = ctx.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    public List<Reminder> loadAll() {
        List<Reminder> list = new ArrayList<>();
        String json = prefs.getString(KEY, "[]");
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
        prefs.edit().putString(KEY, arr.toString()).apply();
    }

    public int nextId() {
        int id = prefs.getInt(KEY_NEXT_ID, 1);
        prefs.edit().putInt(KEY_NEXT_ID, id + 1).apply();
        return id;
    }

    public Reminder findById(int id) {
        for (Reminder r : loadAll()) {
            if (r.id == id) return r;
        }
        return null;
    }

    public void save(Reminder reminder) {
        List<Reminder> all = loadAll();
        boolean found = false;
        for (int i = 0; i < all.size(); i++) {
            if (all.get(i).id == reminder.id) {
                all.set(i, reminder);
                found = true;
                break;
            }
        }
        if (!found) all.add(reminder);
        saveAll(all);
    }

    public void delete(int id) {
        List<Reminder> all = loadAll();
        for (int i = 0; i < all.size(); i++) {
            if (all.get(i).id == id) {
                all.remove(i);
                break;
            }
        }
        saveAll(all);
    }
}
