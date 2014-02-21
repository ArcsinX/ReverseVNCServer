package org.arcsinx.reversevncserver;

import java.io.OutputStream;
import java.io.InputStreamReader;
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.CheckBox;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import java.io.File;

public class MainActivity extends Activity {
	static private String exe_name = "libreversevncserver.so";
	
	private boolean serverStarted = false;
	
	private SharedPreferences prefs;
	
	private boolean checkIfRunning()
	{
		try
		{
			Process p = Runtime.getRuntime().exec("ps");
			p.waitFor();
			StringBuffer sb = new StringBuffer();
			InputStreamReader isr = new InputStreamReader(p.getInputStream());
			int ch;
			char [] buf = new char[1024];
			while((ch = isr.read(buf)) != -1)
			{
				sb.append(buf, 0, ch);
			}
			if (sb.indexOf(exe_name) != -1)
			{
				return true;
			}
		}
		catch (Exception e)
		{
			return false;
		}
		return false;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		if (checkIfRunning())
		{
			Button startStopButton = (Button)findViewById(R.id.startstop_button);
			startStopButton.setText("Stop");
			serverStarted = true;
		}
		
		
		EditText port_edit = (EditText)findViewById(R.id.port_edit);
		EditText reverse_hostport_edit = (EditText)findViewById(R.id.reversehostport_edit);
		CheckBox reconnect_checkbox = (CheckBox)findViewById(R.id.reconnect_checkbox);
		
		prefs = PreferenceManager.getDefaultSharedPreferences(this);
		
		reconnect_checkbox.setChecked(prefs.getBoolean("reconnect", false));
		port_edit.setText(prefs.getString("port", "5901"));
		reverse_hostport_edit.setText(prefs.getString("reverse_hostport",""));
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	static void writeCommand(OutputStream os, String command) throws Exception {
		os.write((command + "\n").getBytes("ASCII"));
	}
	
	public void starStopServer(View view) {
		Button startStopButton = (Button)findViewById(R.id.startstop_button);
		EditText port_edit = (EditText)findViewById(R.id.port_edit);
		EditText reverse_hostport_edit = (EditText)findViewById(R.id.reversehostport_edit);
		CheckBox reconnect_checkbox = (CheckBox)findViewById(R.id.reconnect_checkbox);
		
		if (serverStarted)
		{
			String files_dir = getFilesDir().getAbsolutePath();
			try
			{
				Process sh = Runtime.getRuntime().exec("su",null,new File(files_dir));
				OutputStream os = sh.getOutputStream();
				writeCommand(os, "killall libreversevncserver.so");
				
				startStopButton.setText("Start");
				serverStarted = false;
			}
			catch (Exception e)
			{
				
			}
			
		}
		else
		{
			String files_dir = getFilesDir().getAbsolutePath();
			String reversevncserver_cmd = getFilesDir().getParent() + "/lib/" + exe_name;
			String reversevncserver_chmod = "chmod a+rwx " + reversevncserver_cmd;
			
			String port = port_edit.getText().toString();
			if (!port.isEmpty())
				reversevncserver_cmd += " -p " + port;
			
		    String reverse_hostport = reverse_hostport_edit.getText().toString();
		    if (!reverse_hostport.isEmpty())
		    	reversevncserver_cmd += " -c " + reverse_hostport;
		    
		    if (reconnect_checkbox.isChecked())
		    	reversevncserver_cmd += " -r";
			
			try
			{
				Process sh = Runtime.getRuntime().exec("su", null, new File(files_dir));
				OutputStream os = sh.getOutputStream();
				writeCommand(os, reversevncserver_chmod);
				writeCommand(os, reversevncserver_cmd);
				
				startStopButton.setText("Stop");
				serverStarted = true;
			}
			catch (Exception e)
			{
				
			}
			
			SharedPreferences.Editor e = prefs.edit();
			
			e.putBoolean("reconnect", reconnect_checkbox.isChecked());
			e.putString("port", port_edit.getText().toString());
			e.putString("reverse_hostport",reverse_hostport_edit.getText().toString());
			e.commit();
		}
	}

}
