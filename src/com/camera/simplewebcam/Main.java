package com.camera.simplewebcam;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.media.AudioManager;
import android.media.SoundPool;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.GregorianCalendar;
import java.util.Locale;

public class Main extends Activity implements CameraPreview.PreviewStartCallback{
    private static final boolean DEBUG = true;
    private static final String TAG="WebCam";
    public static final String KEY_CAPTURE_URI = "capture_uri";
    private static final SimpleDateFormat mDateTimeFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss", Locale.US);

    private LinearLayout loading_container;
    private CameraPreview cp;
    private ImageButton captureButton;
    private SoundPool mSoundPool;
    private int mSoundId;

    private Uri mSaveUri;
    private boolean mIsCaptureIntent;

	static {
		Log.d(TAG, "loadLibrary");
		System.loadLibrary("ImageProc");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
        loading_container= (LinearLayout) findViewById(R.id.loading_container);
        loading_container.setVisibility(View.VISIBLE);
		cp = (CameraPreview) findViewById(R.id.cp);
        cp.setCallBack(this);
        captureButton = (ImageButton) findViewById(R.id.capture_button);
        captureButton.setEnabled(false);

        loadShutterSound(getApplicationContext());
        captureButton.setOnClickListener(mOnClickListener);
	}

    @Override
    protected void onStart() {
        Log.d(TAG, "onStart");
        super.onStart();
        cp.openCamera();
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume");
        super.onResume();
    }

    @Override
    protected void onRestart() {
        Log.d(TAG, "onRestart");
        super.onRestart();
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "onStop");
        super.onStop();
        cp.quitCamera();
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        super.onDestroy();
    }

    @Override
    public void onPreviewStart() {
        Log.d(TAG, "onPreviewStart");
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                loading_container.setVisibility(View.GONE);
                captureButton.setEnabled(true);
            }
        });
    }


    private final View.OnClickListener mOnClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mSoundPool.play(mSoundId, 1.0f, 1.0f, 0, 0, 1.0f);
            mIsCaptureIntent = isCaptureIntent();
            if (mIsCaptureIntent && mSaveUri != null) {
                String path = mSaveUri.getPath();//Uri.parse(mSaveUri.toString()).getPath();
                if (DEBUG) Log.v(TAG, "filePath=" + path);
                boolean isSuccess = saveCaptureFile(path);
                exitActivity(isSuccess);
            } else {
                String fileName = getDateTimeString() + ".jpg";
                Log.d(TAG, "capture image:" + fileName);
                MediaStore.Images.Media.insertImage(getContentResolver(), cp.captureImage(), fileName, null);
            }
        }
    };

    /**
     * prepare and load shutter sound for still image capturing
     */
    @SuppressWarnings("deprecation")
    private void loadShutterSound(final Context context) {
        if (DEBUG) Log.d(TAG, "loadShutterSound:");
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

    private boolean saveCaptureFile(final String path) {
        final File file = new File(path);
        try {
            FileOutputStream fos = new FileOutputStream(file);
            final Bitmap bmp = reverseBitmap(cp.captureImage(), 0);
            bmp.compress(Bitmap.CompressFormat.JPEG, 100, fos);
            try {
                fos.flush();
                fos.close();
                return true;
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return false;
        }
    }

    /**
     * get current date and time as String
     * @return
     */
    private static final String getDateTimeString() {
        final GregorianCalendar now = new GregorianCalendar();
        return mDateTimeFormat.format(now.getTime());
    }

    private boolean isCaptureIntent(){
        if(MediaStore.ACTION_IMAGE_CAPTURE.equals(getIntent().getAction())
                || MediaStore.ACTION_IMAGE_CAPTURE_SECURE.equals(getIntent().getAction())){
            mSaveUri = getIntent().getParcelableExtra(MediaStore.EXTRA_OUTPUT);
            if (DEBUG) {
                if (mSaveUri != null) {
                    Log.v(TAG, "isCaptureIntent: mSaveUri=" + mSaveUri.toString());
                } else {
                    Log.v(TAG, "isCaptureIntent: mSaveUri=null");
                }
            }
            return true;
        }else {
            mSaveUri = null;
            return false;
        }
    }

    private void exitActivity(boolean isSuccess) {
        if (DEBUG) Log.v(TAG, "exitActivity isSuccess=" + isSuccess);
        captureButton.setEnabled(false);
        if (mIsCaptureIntent && mSaveUri != null) {
            Intent intent = new Intent();
            intent.putExtra(KEY_CAPTURE_URI, mSaveUri);
            if (isSuccess) {
                setResult(Activity.RESULT_OK, intent);
            }
            finish();
        }
    }

    /**
     * 图片反转
     *
     * @param bmp
     * @param flag 0为水平反转，1为垂直反转
     * @return
     */
    public Bitmap reverseBitmap(Bitmap bmp, int flag) {
        float[] floats = null;
        switch (flag) {
            case 0: // 水平反转
                floats = new float[]{-1f, 0f, 0f, 0f, 1f, 0f, 0f, 0f, 1f};
                break;
            case 1: // 垂直反转
                floats = new float[]{1f, 0f, 0f, 0f, -1f, 0f, 0f, 0f, 1f};
                break;
        }

        if (floats != null) {
            Matrix matrix = new Matrix();
            matrix.setValues(floats);
            return Bitmap.createBitmap(bmp, 0, 0, bmp.getWidth(), bmp.getHeight(), matrix, true);
        }

        return null;
    }
}
