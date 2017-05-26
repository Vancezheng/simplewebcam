package com.camera.simplewebcam;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class Main extends Activity {
	private static final String TAG="WebCam";
	CameraPreview cp;

	static {
		Log.d(TAG, "loadLibrary");
		System.loadLibrary("ImageProc");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		cp = (CameraPreview) findViewById(R.id.cp);
	}

	@Override
	protected void onStart() {
		Log.d(TAG, "onStart");
		super.onStart();
		cp.openCamera();
	}

	@Override
	protected void onRestart() {
		Log.d(TAG, "onRestart");
		super.onRestart();
	}

	@Override
	protected void onResume() {
		Log.d(TAG, "onResume");
		super.onResume();
	}

	@Override
	protected void onPause() {
		Log.d(TAG, "onPause");
		super.onPause();
	}

	@Override
	protected void onStop() {
		super.onStop();
		Log.d(TAG, "onStop");
		cp.quitCamera();
	}

	@Override
	protected void onDestroy() {
		Log.d(TAG, "onDestroy");
		super.onDestroy();
	}
}
