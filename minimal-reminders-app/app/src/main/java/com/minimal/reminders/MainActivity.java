package com.minimal.reminders;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.Switch;
import android.widget.TextView;

import java.util.List;

public class MainActivity extends Activity {
    private ReminderStore store;
    private List<Reminder> reminders;
    private ReminderAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        store = new ReminderStore(this);

        ListView listView = findViewById(R.id.list);
        View emptyView = findViewById(R.id.empty);
        listView.setEmptyView(emptyView);

        reminders = store.loadAll();
        adapter = new ReminderAdapter();
        listView.setAdapter(adapter);

        findViewById(R.id.fab).setOnClickListener(v -> {
            Intent intent = new Intent(this, EditReminderActivity.class);
            startActivity(intent);
        });

        requestNotificationPermission();
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshList();
    }

    private void refreshList() {
        reminders = store.loadAll();
        adapter.notifyDataSetChanged();
    }

    private void requestNotificationPermission() {
        if (Build.VERSION.SDK_INT >= 33) {
            if (checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, 1);
            }
        }
    }

    private class ReminderAdapter extends BaseAdapter {
        @Override
        public int getCount() { return reminders.size(); }

        @Override
        public Object getItem(int pos) { return reminders.get(pos); }

        @Override
        public long getItemId(int pos) { return reminders.get(pos).id; }

        @Override
        public View getView(int pos, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = getLayoutInflater().inflate(R.layout.item_reminder, parent, false);
            }
            Reminder r = reminders.get(pos);
            TextView nameView = convertView.findViewById(R.id.item_name);
            TextView timeView = convertView.findViewById(R.id.item_time);
            TextView daysView = convertView.findViewById(R.id.item_days);
            Switch toggle = convertView.findViewById(R.id.item_toggle);
            ImageButton editBtn = convertView.findViewById(R.id.btn_edit);
            ImageButton deleteBtn = convertView.findViewById(R.id.btn_delete);

            nameView.setText(r.name);
            timeView.setText(r.getTimeText());
            daysView.setText(r.getDaysText());

            toggle.setOnCheckedChangeListener(null);
            toggle.setChecked(r.enabled);
            toggle.setOnCheckedChangeListener((CompoundButton buttonView, boolean isChecked) -> {
                r.enabled = isChecked;
                store.save(r);
                if (isChecked) {
                    AlarmScheduler.schedule(MainActivity.this, r);
                } else {
                    AlarmScheduler.cancel(MainActivity.this, r);
                }
            });

            editBtn.setOnClickListener(v -> {
                Intent intent = new Intent(MainActivity.this, EditReminderActivity.class);
                intent.putExtra("reminder_id", r.id);
                startActivity(intent);
            });

            deleteBtn.setOnClickListener(v -> {
                new AlertDialog.Builder(MainActivity.this)
                        .setTitle("Delete reminder?")
                        .setMessage("Delete \"" + r.name + "\"?")
                        .setPositiveButton("Delete", (d, w) -> {
                            AlarmScheduler.cancel(MainActivity.this, r);
                            store.delete(r.id);
                            refreshList();
                        })
                        .setNegativeButton("Cancel", null)
                        .show();
            });

            return convertView;
        }
    }
}
