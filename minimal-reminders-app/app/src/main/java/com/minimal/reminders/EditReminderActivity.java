package com.minimal.reminders;

import android.app.Activity;
import android.app.TimePickerDialog;
import android.os.Bundle;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class EditReminderActivity extends Activity {
    private ReminderStore store;
    private Reminder reminder;
    private boolean isNew;

    private EditText nameEdit;
    private TextView timeDisplay;
    private CheckBox[] dayChecks = new CheckBox[7];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_edit);

        store = new ReminderStore(this);

        nameEdit = findViewById(R.id.edit_name);
        timeDisplay = findViewById(R.id.edit_time);
        dayChecks[0] = findViewById(R.id.day_mon);
        dayChecks[1] = findViewById(R.id.day_tue);
        dayChecks[2] = findViewById(R.id.day_wed);
        dayChecks[3] = findViewById(R.id.day_thu);
        dayChecks[4] = findViewById(R.id.day_fri);
        dayChecks[5] = findViewById(R.id.day_sat);
        dayChecks[6] = findViewById(R.id.day_sun);

        int reminderId = getIntent().getIntExtra("reminder_id", -1);
        if (reminderId != -1) {
            reminder = store.findById(reminderId);
            isNew = false;
        }

        if (reminder == null) {
            reminder = new Reminder();
            reminder.id = store.nextId();
            reminder.name = "";
            reminder.hour = 8;
            reminder.minute = 0;
            // Default to every day
            for (int i = 0; i < 7; i++) reminder.days[i] = true;
            isNew = true;
        }

        nameEdit.setText(reminder.name);
        updateTimeDisplay();

        for (int i = 0; i < 7; i++) {
            dayChecks[i].setChecked(reminder.days[i]);
        }

        timeDisplay.setOnClickListener(v -> {
            new TimePickerDialog(this, (view, hourOfDay, min) -> {
                reminder.hour = hourOfDay;
                reminder.minute = min;
                updateTimeDisplay();
            }, reminder.hour, reminder.minute, true).show();
        });

        findViewById(R.id.btn_save).setOnClickListener(v -> save());
        findViewById(R.id.btn_cancel).setOnClickListener(v -> finish());
    }

    private void updateTimeDisplay() {
        timeDisplay.setText(reminder.getTimeText());
    }

    private void save() {
        String name = nameEdit.getText().toString().trim();
        if (name.isEmpty()) {
            Toast.makeText(this, "Please enter a name", Toast.LENGTH_SHORT).show();
            return;
        }

        boolean anyDay = false;
        for (int i = 0; i < 7; i++) {
            reminder.days[i] = dayChecks[i].isChecked();
            if (reminder.days[i]) anyDay = true;
        }

        if (!anyDay) {
            Toast.makeText(this, "Select at least one day", Toast.LENGTH_SHORT).show();
            return;
        }

        reminder.name = name;
        reminder.enabled = true;
        store.save(reminder);
        AlarmScheduler.schedule(this, reminder);

        Toast.makeText(this, isNew ? "Reminder created" : "Reminder updated", Toast.LENGTH_SHORT).show();
        finish();
    }
}
