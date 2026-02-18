package com.vibes.reminders;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TimePicker;
import android.widget.ToggleButton;

@SuppressWarnings("deprecation")
public class EditReminderActivity extends Activity {
    private Reminder reminder;
    private ReminderStorage storage;
    private boolean isNew;

    private EditText editName;
    private TimePicker timePicker;
    private ToggleButton[] dayButtons;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_edit_reminder);

        storage = new ReminderStorage(this);

        editName = (EditText) findViewById(R.id.edit_name);
        timePicker = (TimePicker) findViewById(R.id.time_picker);
        timePicker.setIs24HourView(true);

        dayButtons = new ToggleButton[]{
                (ToggleButton) findViewById(R.id.day_mon),
                (ToggleButton) findViewById(R.id.day_tue),
                (ToggleButton) findViewById(R.id.day_wed),
                (ToggleButton) findViewById(R.id.day_thu),
                (ToggleButton) findViewById(R.id.day_fri),
                (ToggleButton) findViewById(R.id.day_sat),
                (ToggleButton) findViewById(R.id.day_sun)
        };

        Button btnSave = (Button) findViewById(R.id.btn_save);
        Button btnCancel = (Button) findViewById(R.id.btn_cancel);
        Button btnDelete = (Button) findViewById(R.id.btn_delete);

        long reminderId = getIntent().getLongExtra("reminder_id", -1);
        if (reminderId != -1) {
            reminder = storage.findById(reminderId);
            isNew = false;
            setTitle(R.string.edit_reminder);
            btnDelete.setVisibility(View.VISIBLE);
        }

        if (reminder == null) {
            reminder = new Reminder();
            isNew = true;
            setTitle(R.string.add_reminder);
        }

        editName.setText(reminder.name);
        timePicker.setCurrentHour(reminder.hour);
        timePicker.setCurrentMinute(reminder.minute);
        for (int i = 0; i < 7; i++) {
            dayButtons[i].setChecked(reminder.days[i]);
        }

        btnSave.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) { save(); }
        });
        btnCancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) { finish(); }
        });
        btnDelete.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) { delete(); }
        });
    }

    private void save() {
        reminder.name = editName.getText().toString().trim();
        if (reminder.name.isEmpty()) {
            editName.setError("Name required");
            return;
        }

        reminder.hour = timePicker.getCurrentHour();
        reminder.minute = timePicker.getCurrentMinute();
        for (int i = 0; i < 7; i++) {
            reminder.days[i] = dayButtons[i].isChecked();
        }
        reminder.enabled = true;

        storage.save(reminder);
        AlarmScheduler.schedule(this, reminder);
        finish();
    }

    private void delete() {
        AlarmScheduler.cancel(this, reminder);
        storage.delete(reminder.id);
        finish();
    }
}
