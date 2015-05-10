package com.erikawilli.mydoor;

import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import android.os.Bundle;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.view.Menu;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Toast;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelUuid;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.view.animation.RotateAnimation;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends Activity {

    private Button On,Off,Visible,list;
    private BluetoothAdapter BA;
    private Set<BluetoothDevice>pairedDevices;
    private ListView lv;
    private UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb"); //e73d86ae-aeb6-4577-81db-74b3c7c2a221
    protected static final int MESSAGE_READ = 1;
    protected ConnectThread connect = null;
    protected ConnectedThread conn;
    protected String lockCommand = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        On = (Button)findViewById(R.id.button1);
        Off = (Button)findViewById(R.id.button2);
        Visible = (Button)findViewById(R.id.button3);
        list = (Button)findViewById(R.id.button4);

        lv = (ListView)findViewById(R.id.listView1);

        BA = BluetoothAdapter.getDefaultAdapter();
    }

    public void on(View view){
        if (!BA.isEnabled()) {
            Intent turnOn = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(turnOn, 0);
            Toast.makeText(getApplicationContext(),"Turned on"
                    ,Toast.LENGTH_LONG).show();
        }
        else{
            Toast.makeText(getApplicationContext(),"Already on",
                    Toast.LENGTH_LONG).show();
        }
    }
    public void list(View view){
        pairedDevices = BA.getBondedDevices();

        ArrayList list = new ArrayList();
        for(BluetoothDevice bt : pairedDevices)
            list.add(bt.getName());

        Toast.makeText(getApplicationContext(),"Showing Paired Devices",
                Toast.LENGTH_SHORT).show();
        final ArrayAdapter adapter = new ArrayAdapter
                (this,android.R.layout.simple_list_item_1, list);
        lv.setAdapter(adapter);

    }
    public void off(View view){
        BA.disable();
        Toast.makeText(getApplicationContext(),"Turned off" ,
                Toast.LENGTH_LONG).show();
    }

    public void write(byte[] out) {
        ConnectedThread r;
        synchronized (this) {
            r = conn;
            r.write(out);
        }
    }

    public void lock(View view){
        lockCommand = "L\r\n";
        Toast.makeText(getApplicationContext(), "Locking Door" ,
                Toast.LENGTH_LONG).show();
        connectToDevice();
    }

    public void unlock(View view){
        lockCommand = "U\r\n";
        Toast.makeText(getApplicationContext(), "Unlocking Door" ,
                Toast.LENGTH_LONG).show();
        connectToDevice();
    }

    public void visible(View view){
        Intent getVisible = new Intent(BluetoothAdapter.
                ACTION_REQUEST_DISCOVERABLE);
        startActivityForResult(getVisible, 0);

    }

    public void connectToDevice(){
        //if (connect == null) {
            pairedDevices = BA.getBondedDevices();

            for (BluetoothDevice bt : pairedDevices) {
                if (bt.getName().equals("HC-05")) {
                    ParcelUuid[] uuids = bt.getUuids();
                    connect = new ConnectThread(bt);
                    connect.start();

                }
            }
        //} else {

        //}


    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    private void manageConnectedSocket(BluetoothSocket mBluetoothSocket) {
        conn = new ConnectedThread(mBluetoothSocket);

        conn.write(lockCommand.getBytes());
        conn.start();

    }


    private class ConnectThread extends Thread {
        private final BluetoothSocket mmSocket;
        private final BluetoothDevice mmDevice;

        public ConnectThread(BluetoothDevice device) {
            // Use a temporary object that is later assigned to mmSocket,
            // because mmSocket is final
            BluetoothSocket tmp = null;
            mmDevice = device;

            // Get a BluetoothSocket to connect with the given BluetoothDevice
            try {
                // MY_UUID is the app's UUID string, also used by the server code
                tmp = device.createRfcommSocketToServiceRecord(MY_UUID);
            } catch (IOException e) { }
            mmSocket = tmp;
        }

        public void run() {
            // Cancel discovery because it will slow down the connection
            BA.cancelDiscovery();

            try {
                // Connect the device through the socket. This will block
                // until it succeeds or throws an exception
                if(!mmSocket.isConnected()) {
                    mmSocket.connect();
                }
            } catch (IOException connectException) {
                // Unable to connect; close the socket and get out
                connectException.printStackTrace();
                try {
                    mmSocket.close();
                } catch (IOException closeException) {
                }
                return;
            }

            // Do work to manage the connection (in a separate thread)
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    manageConnectedSocket(mmSocket);
                    try {
                        mmSocket.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            });

        }

        /** Will cancel an in-progress connection, and close the socket */
        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) { }
        }

    }

    private class ConnectedThread extends Thread {
        private final BluetoothSocket mmSocket;
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            mmSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            // Get the input and output streams, using temp objects because
            // member streams are final
            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) { }

            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {
            byte[] buffer = new byte[1024];  // buffer store for the stream
            int bytes; // bytes returned from read()

            // Keep listening to the InputStream until an exception occurs
            while (true) {
                try {
                    // Read from the InputStream
                    bytes = mmInStream.read(buffer);
                    // Send the obtained bytes to the UI activity
                    mHandler.obtainMessage(MESSAGE_READ, bytes, -1, buffer)
                            .sendToTarget();
                } catch (IOException e) {
                    break;
                }
            }
        }

        /* Call this from the main activity to send data to the remote device */
        public void write(byte[] bytes) {
            try {
                mmOutStream.write(bytes);
            } catch (IOException e) { }
        }

        /* Call this from the main activity to shutdown the connection */
        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) { }
        }
    }

    Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MESSAGE_READ:
                    byte[] readBuf = (byte[])msg.obj;

                    String message = new String(readBuf);

                    break;
            }
        }
    };

}
