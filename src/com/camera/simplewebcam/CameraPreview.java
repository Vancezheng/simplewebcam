package com.camera.simplewebcam;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.ImageButton;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

class CameraPreview extends SurfaceView implements SurfaceHolder.Callback, Runnable {

	private static final boolean DEBUG = true;
	private static final String TAG="WebCam";
	protected Context context;
	private SurfaceHolder holder;
    Thread mainLoop = null;
	private Bitmap bmp=null;

	private boolean cameraExists=false;
	private boolean shouldStop=false;
    private boolean captureFinished;
    private Object mCaptureSync = new Object();
	
	// /dev/videox (x=cameraId+cameraBase) is used.
	// In some omap devices, system uses /dev/video[0-3],
	// so users must use /dev/video[4-].
	// In such a case, try cameraId=0 and cameraBase=4
	private int cameraId=0;
	private int cameraBase=0;
	
	// This definition also exists in ImageProc.h.
	// Webcam must support the resolution 640x480 with YUYV format.
	static final int IMG_WIDTH=1280;
	static final int IMG_HEIGHT=720;

	// The following variables are used to draw camera images.
    private int winWidth=0;
    private int winHeight=0;
    private Rect rect;
    private int dw, dh;
    private float rate;
    private PreviewStartCallback psCallback;
    private boolean isPreviewStart;
  
    // JNI functions
    public native int powerOnOffCamera(int power);
    public native int prepareCamera(int videoid);
    public native int prepareCameraWithBase(int videoid, int camerabase, int width, int height);
    public native byte[] processCamera();
    public native void stopCamera();
    public native void pixeltobmp(Bitmap bitmap);

    
	public CameraPreview(Context context) {
		super(context);
		this.context = context;
		if(DEBUG) Log.d(TAG,"CameraPreview constructed");
		setFocusable(true);
		
		holder = getHolder();
		holder.addCallback(this);
	}

	public CameraPreview(Context context, AttributeSet attrs) {
		super(context, attrs);
		this.context = context;
		if(DEBUG) Log.d(TAG,"CameraPreview constructed");
		setFocusable(true);
		
		holder = getHolder();
		holder.addCallback(this);
	}
	
    @Override
    public void run() {
        while (true && cameraExists) {
        	//obtaining display area to draw a large image
        	if(winWidth==0){
        		winWidth=this.getWidth();
        		winHeight=this.getHeight();

        		if(winWidth*IMG_HEIGHT/IMG_WIDTH<=winHeight){
        			dw = 0;
        			dh = (winHeight-winWidth*IMG_HEIGHT/IMG_WIDTH)/2;
        			rate = ((float)winWidth)/IMG_WIDTH;
        			rect = new Rect(dw,dh,dw+winWidth-1,dh+winWidth*IMG_HEIGHT/IMG_WIDTH-1);
        		}else{
        			dw = (winWidth-winHeight*IMG_WIDTH/IMG_HEIGHT)/2;
        			dh = 0;
        			rate = ((float)winHeight)/IMG_HEIGHT;
        			rect = new Rect(dw,dh,dw+winHeight*IMG_WIDTH/IMG_HEIGHT -1,dh+winHeight-1);
                }

        	}
            synchronized (mCaptureSync) {
                // obtaining a camera image (pixel data are stored in an array in JNI).
                byte[] framebuffer = processCamera();
                // camera image to bmp
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inSampleSize = 1;
                bmp = BitmapFactory.decodeByteArray(framebuffer, 0, framebuffer.length, options);
                captureFinished = true;
                mCaptureSync.notify();
            }

        	//pixeltobmp(bmp);

			if (bmp != null) {
				Canvas canvas = getHolder().lockCanvas();
				if (canvas != null) {
					// draw camera bmp on canvas
					canvas.drawBitmap(bmp, null, rect, null);

					getHolder().unlockCanvasAndPost(canvas);
				}
                if (!isPreviewStart) {
                    psCallback.onPreviewStart();
                    isPreviewStart = true;
                }
			}

            if(shouldStop){
            	shouldStop = false;  
            	break;
            }	        
        }
    }

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		if(DEBUG) Log.d(TAG, "surfaceCreated");
        openCamera();
	}

    public void openCamera() {
        if(!cameraExists) {
            if (DEBUG) Log.d(TAG, "openCamera");
            powerOnOffCamera(1);
            if (bmp == null) {
                bmp = Bitmap.createBitmap(IMG_WIDTH, IMG_HEIGHT, Bitmap.Config.ARGB_8888);
            }
            if (DEBUG) Log.d(TAG, "preview size:" + IMG_WIDTH + 'x' + IMG_HEIGHT);
            // /dev/videox (x=cameraId + cameraBase) is used
            int ret = prepareCameraWithBase(cameraId, cameraBase, IMG_WIDTH, IMG_HEIGHT);

            if (ret != -1) cameraExists = true;

            mainLoop = new Thread(this);
            mainLoop.start();
        }
    }

    @Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		if(DEBUG) Log.d(TAG, "surfaceChanged");
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		if(DEBUG) Log.d(TAG, "surfaceDestroyed");
        quitCamera();
	}

    public void quitCamera(){
        if(cameraExists){
            if(DEBUG) Log.d(TAG, "quitCamera");
            shouldStop = true;
            while(shouldStop){
                try{
                    Thread.sleep(100); // wait for thread stopping
                }catch(Exception e){}
            }
            cameraExists = false;
            stopCamera();
            powerOnOffCamera(0);
        }
    }

    public Bitmap captureImage(){
        synchronized (mCaptureSync) {
            try {
                while (!captureFinished) {
                    mCaptureSync.wait();
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            captureFinished = false;
            return bmp;
        }
    }

    public void setCallBack(PreviewStartCallback previewStartCallback){
        psCallback = previewStartCallback;
    }

    public interface PreviewStartCallback {
        void onPreviewStart();
    }
}
