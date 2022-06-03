package com.example.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Debug;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import android.widget.TimePicker;
import android.app.TimePickerDialog;

import androidx.appcompat.app.AppCompatActivity;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

import yuku.ambilwarna.AmbilWarnaDialog;

public class MainActivity extends AppCompatActivity {

    static final UUID mUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private int mDefaultColor;
    private BluetoothSocket btSocket;
    byte[] buffer = new byte[256];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final Button setTime = (Button) findViewById(R.id.setTime);
        final Button setDate = (Button) findViewById(R.id.setDate);
        final Button setTemp = (Button) findViewById(R.id.setTemp);
        final Button setHumid = (Button) findViewById(R.id.setHumid);
        final Button setConnect = (Button) findViewById(R.id.setConnect);
        final Button setQuit = (Button) findViewById(R.id.setQuit);
        final Button setColor = (Button) findViewById(R.id.setColor);
        final Button setWake = (Button) findViewById(R.id.setWake);
        final Button setSleep = (Button) findViewById(R.id.setSleep);

        BluetoothDevice hc05 = getAdapter();
        btSocket = connectDevice(hc05);

        setTime.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                writeSingle('k');
            }
        });
        setDate.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                writeSingle('m');
            }
        });
        setTemp.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                writeSingle('t');
            }
        });
        setHumid.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                writeSingle('h');
            }
        });
        setConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                // Stops code from connecting when already connected
                if (btSocket.isConnected()){
                    notificationToast("Device is already connected");
                    return;
                }
                BluetoothDevice hc05 = getAdapter();
                BluetoothSocket btSocket = connectDevice(hc05);
            }
        });
        setQuit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                // Stops code from disconnecting when already disconnected
                if (!btSocket.isConnected()){
                    notificationToast("Device is not connected");
                    return;
                }
                try {
                    btSocket.close();
                    notificationToast("Device is now disconnected");
                    System.out.println(btSocket.isConnected());
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });
        setColor.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {

                String data = readData("?c");
                int r = getSubInt(data, 20, 3);
                int g = getSubInt(data, 25, 3);
                int b = getSubInt(data, 30, 3);
                mDefaultColor = Color.parseColor(String.format("#%02x%02x%02x", r, g, b));
                openColorPickerDialogue();
            }
        });
        setWake.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                getTimeInput(0);
            }
        });
        setSleep.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                getTimeInput(1);
            }
        });

    }

    private void writeString(String msg){
        for (int i = 0; i < msg.length(); i++)
            writeSingle(msg.charAt(i));
    }

    private void writeSingle(int i) {
        try {
            OutputStream outputStream = btSocket.getOutputStream();
            outputStream.write(i);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private String readData(String command){

        try{
            writeString(command);

            int bytes = 0;
            while (true){
                buffer[bytes] = (byte) btSocket.getInputStream().read();
                if (buffer[bytes] == '\n') break;
                else bytes++;
            }
            return new String(buffer, 0, bytes);

        } catch (IOException e){
            e.printStackTrace();
        }

        return "";

    }

    private void notificationToast(CharSequence text) {
        Context context = getApplicationContext();
        //int duration = Toast.LENGTH_LONG;
        Toast toast = Toast.makeText(context, text, 30);
        toast.show();
    }



    private BluetoothSocket connectDevice(BluetoothDevice hc05){
        int counter = 0;
        BluetoothSocket btSocket = null;
        do {
            try {
                btSocket = hc05.createRfcommSocketToServiceRecord(mUUID);
                btSocket.connect();
                notificationToast("Connected to: " + hc05.getName());

            } catch (IOException e) {
                notificationToast("Could not connect to device");
                e.printStackTrace();
            }
            counter++;
        } while (!btSocket.isConnected() && counter < 3);

        return btSocket;
    }

    private BluetoothDevice getAdapter(){
        BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
        System.out.println(btAdapter.getBondedDevices());

        BluetoothDevice hc05 = btAdapter.getRemoteDevice("FC:A8:9A:00:0A:3A");
        System.out.println(hc05.getName());

        return  hc05;
    }

    public void openColorPickerDialogue() {

        // the AmbilWarnaDialog callback needs 3 parameters
        // one is the context, second is default color,
        final AmbilWarnaDialog colorPickerDialogue = new AmbilWarnaDialog(this, mDefaultColor,
                new AmbilWarnaDialog.OnAmbilWarnaListener() {
                    @Override
                    public void onCancel(AmbilWarnaDialog dialog) {
                        // leave this function body as
                        // blank, as the dialog
                        // automatically closes when
                        // clicked on cancel button
                    }

                    @Override
                    public void onOk(AmbilWarnaDialog dialog, int color) {
                        mDefaultColor = color;

                        // Send color code to BT
                        String r = String.format("%03d", Color.red(color));
                        String g = String.format("%03d", Color.green(color));
                        String b = String.format("%03d", Color.blue(color));

                        String message = "c" + r + g + b;

                        writeString(message);

                    }
                });
        colorPickerDialogue.show();
    }

    public int getSubInt(String msg, int start, int len){
        String tmp = "";
        for (int i = start; i < start+len; i++)
            tmp += msg.charAt(i);
        return Integer.parseInt(tmp);

    }

    public void getTimeInput(int i) {

        String awakeTime = readData("?z");

        int wH, wM, sH, sM, h, m, wake, sleep;

        wH = getSubInt(awakeTime, 12, 2);
        wM = getSubInt(awakeTime, 15, 2);
        sH = getSubInt(awakeTime, 20, 2);
        sM = getSubInt(awakeTime, 23, 2);

        wake = wH*60+wM;
        sleep = sH*60+sM;

        if (i == 0){
            h = wH;
            m = wM;
        }else{
            h = sH;
            m = sM;
        }

        TimePickerDialog timePickerDialog = new TimePickerDialog(this,
                new TimePickerDialog.OnTimeSetListener() {

                    @Override
                    public void onTimeSet(TimePicker view, int hourOfDay, int minute) {

                        String msg = "z";
                        if (i == 0){
                            if (60*hourOfDay + minute >= sleep){
                                notificationToast("Cannot set wake up after sleep time");
                                return;
                            }

                            msg += String.format("%02d", hourOfDay);
                            msg += String.format("%02d", minute);
                            msg += String.format("%02d", sH);
                            msg += String.format("%02d", sM);
                        }else{
                            if (60*hourOfDay + minute <= wake){
                                notificationToast("Cannot set sleep after wake up time");
                                return;
                            }
                            msg += String.format("%02d", wH);
                            msg += String.format("%02d", wM);
                            msg += String.format("%02d", hourOfDay);
                            msg += String.format("%02d", minute);
                        }

                        writeString(msg);
                    }
                }, h, m, true);
        timePickerDialog.show();

    }


}