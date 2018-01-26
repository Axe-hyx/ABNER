package com.example.hnc.implement;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class MainActivity extends Activity {

    public static void rootwithLog() {
        String result = "";
        DataOutputStream dos = null;
        DataInputStream dis = null;

        try {
            Process p = Runtime.getRuntime().exec("su");
            dos = new DataOutputStream(p.getOutputStream());
            dis = new DataInputStream(p.getInputStream());

            String line = null;

            dos.writeBytes("./sdcard/rawsocket \n");
            dos.flush();
            while((line = dis.readLine())!=null){
                Log.d("**** result",line);
                result += line;
            }

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (dos != null) {
                try{
                    dos.close();
                }catch (IOException e){
                    e.printStackTrace();
                }
            }
            if (dis != null) {
                try{
                    dis.close();
                }catch (IOException e)
                {
                    e.printStackTrace();
                }
            }
        }
    }

    public static boolean root(String cmd) {
        Process process = null;
        DataOutputStream os = null;
        try {
            process = Runtime.getRuntime().exec("su");
            os = new DataOutputStream(process.getOutputStream());
            os.writeBytes(cmd + "\n");
            os.writeBytes("exit\n");
            os.flush();
            process.waitFor();
        } catch (Exception e) {
            Log.d("*** DEBUG ***", "ROOT REE" + e.getMessage());
            return false;
        } finally {
            try {
                if (os != null) {
                    os.close();
                }
                process.destroy();
            } catch (Exception e) {

            }
        }
        Log.d("*** DEBUG ***", "Root SUC");
        return true;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /*String cmd = "chmod 777" + getPackageCodePath();
        root(cmd);*/
        rootwithLog();

        //Intent intent = new Intent(this, android.app.NativeActivity.class);

    }


}
