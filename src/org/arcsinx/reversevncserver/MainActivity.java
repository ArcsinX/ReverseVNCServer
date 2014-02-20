package org.arcsinx.reversevncserver;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.content.Intent;
import android.widget.Button;

public class MainActivity extends Activity {

	private boolean started = false;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	public void starStopServer(View view) {
		Button startStopButton = (Button)findViewById(R.id.startstop_button);
		if (started)
		{
			startStopButton.setText("Start");
			started = false;
		}
		else
		{
			String reversevncserver_exec=getFilesDir().getParent() + "/lib/reversevncserver";
			
			startStopButton.setText("Stop");
			started = true;
		}
	}

}
