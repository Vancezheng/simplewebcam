package com.camera.simplewebcam;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.media.SoundPool;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;

public class Main extends Activity {
	private static final String TAG="WebCam";
	CameraPreview cp;
    private ImageButton captureButton;
    private SoundPool mSoundPool;
    private int mSoundId;

	static {
		Log.d(TAG, "loadLibrary");
		System.loadLibrary("ImageProc");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		cp = (CameraPreview) findViewById(R.id.cp);
        captureButton = (ImageButton) findViewById(R.id.capture_button);
        loadShutterSound(getApplicationContext());
        captureButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mSoundPool.play(mSoundId, 1.0f, 1.0f, 0, 0, 1.0f);
                String fileName = System.currentTimeMillis() + ".jpg";
                Log.d(TAG, "capture image:" + fileName);
                MediaStore.Images.Media.insertImage(getContentResolver(), cp.captureImage(), fileName, null);
            }
        });
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

    /**
     * prepare and load shutter sound for still image capturing
     */
    @SuppressWarnings("deprecation")
    private void loadShutterSound(final Context context) {
        Log.d(TAG, "loadShutterSound:");
        if (mSoundPool != null) {
            try {
                mSoundPool.release();
            } catch (final Exception e) {
            }
            mSoundPool = null;
        }
        // load sutter sound from resource
        //把类型设置为AudioManager.STREAM_ALARM  就能保证在静音条件下也能播放声音
        mSoundPool = new SoundPool(2, AudioManager.STREAM_ALARM, 0);
        mSoundId = mSoundPool.load(context, R.raw.camera_click, 1);
    }
}
