package com.vibes.reminders;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.ListView;
import android.widget.Switch;
import android.widget.TextView;
import java.util.List;

public class MainActivity extends Activity {
    private ReminderStorage storage;
    private List<Reminder> reminders;
    private ReminderAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        storage = new ReminderStorage(this);

        ListView listView = (ListView) findViewById(R.id.reminder_list);
        TextView emptyText = (TextView) findViewById(R.id.empty_text);

        adapter = new ReminderAdapter();
        listView.setAdapter(adapter);
        listView.setEmptyView(emptyText);

        listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                Intent intent = new Intent(MainActivity.this, EditReminderActivity.class);
                intent.putExtra("reminder_id", reminders.get(position).id);
                startActivity(intent);
            }
        });

        findViewById(R.id.fab_add).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, EditReminderActivity.class));
            }
        });

        // Request notification permission on Android 13+ (API 33)
        if (Build.VERSION.SDK_INT >= 33) {
            if (checkSelfPermission("android.permission.POST_NOTIFICATIONS") != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[]{"android.permission.POST_NOTIFICATIONS"}, 1);
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        reminders = storage.loadAll();
        adapter.notifyDataSetChanged();
    }

    private class ReminderAdapter extends BaseAdapter {
        @Override
        public int getCount() { return reminders == null ? 0 : reminders.size(); }

        @Override
        public Object getItem(int position) { return reminders.get(position); }

        @Override
        public long getItemId(int position) { return reminders.get(position).id; }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = getLayoutInflater().inflate(R.layout.item_reminder, parent, false);
            }
            final Reminder r = reminders.get(position);

            TextView nameView = (TextView) convertView.findViewById(R.id.reminder_name);
            TextView timeView = (TextView) convertView.findViewById(R.id.reminder_time);
            TextView daysView = (TextView) convertView.findViewById(R.id.reminder_days);
            Switch enabledSwitch = (Switch) convertView.findViewById(R.id.reminder_enabled);

            nameView.setText(r.name);
            timeView.setText(r.getTimeText());
            daysView.setText(r.getDaysText());

            enabledSwitch.setOnCheckedChangeListener(null);
            enabledSwitch.setChecked(r.enabled);
            enabledSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    r.enabled = isChecked;
                    storage.save(r);
                    AlarmScheduler.schedule(MainActivity.this, r);
                }
            });

            return convertView;
        }
    }
}
