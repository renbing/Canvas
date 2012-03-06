package com.woyouquan;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;
import android.view.View;

public class Canvas extends Activity {
    /** Called when the activity is first created. */
	
	static final String packageName = "com.woyouquan";
	private GLSurfaceView mGLView;
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        //android.os.Debug.waitForDebugger();
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        
        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        
        //TextView tv = new TextView(this);
        //tv.setText(stringFromJNI());
        //setContentView(tv);
                
    	ApplicationInfo appInfo = null;
    	PackageManager packMgmr = this.getApplication().getPackageManager();
    	try
    	{
    		appInfo = packMgmr.getApplicationInfo(packageName, 0);
    	}
    	catch(NameNotFoundException e)
    	{
    		e.printStackTrace();
    		throw new RuntimeException("Unable to load asset");
    	}
    	
    	String apkPath = appInfo.sourceDir;
    	nativeInit(apkPath);
    	
        mGLView = new EAGLView(this);
        setContentView(mGLView);
        
        mGLView.setOnTouchListener(new TouchListener());
    }
    
    @Override
    public void onDestroy()
    {
    	super.onDestroy();
    	nativeDone();
    	
    	android.os.Process.killProcess(android.os.Process.myPid());
    }
    
	@Override
	protected void onPause() {
		super.onPause();
		mGLView.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
		mGLView.onResume();
	}
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if( keyCode == KeyEvent.KEYCODE_BACK )
		{
			onDestroy();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}
	
    //private native String stringFromJNI();
    private native void nativeInit(String apkPath);
    private native void nativeDone();
    
    static {
        System.loadLibrary("gljni");
    }
}

class EAGLView extends GLSurfaceView {
    private EAGLRenderer mRenderer;
    
    public EAGLView(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
        
        mRenderer = new EAGLRenderer();
        setRenderer(mRenderer);
    }
}

class EAGLRenderer implements GLSurfaceView.Renderer {

    private static native void nativeInit();
    private static native void nativeResize(int w, int h);
    private static native void nativeRender();
    
    public void onDrawFrame(GL10 gl) {
        // TODO Auto-generated method stub
        nativeRender();
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        // TODO Auto-generated method stub
        nativeResize(width, height);
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        // TODO Auto-generated method stub
        nativeInit();
    }
}

class TouchListener implements View.OnTouchListener{
	
	private static native void nativeDown(float x, float y);
	private static native void nativeMove(float x, float y);
	private static native void nativeUp(float x, float y);

	public boolean onTouch(View v, MotionEvent event) {
		// TODO Auto-generated method stub
		
		float x = event.getX();
		float y = event.getY();
		
		int action = event.getAction();
		switch(action)
		{
			case MotionEvent.ACTION_DOWN:
				nativeDown(x, y);
				break;
			case MotionEvent.ACTION_MOVE:
				nativeMove(x, y);
				break;
			case MotionEvent.ACTION_UP:
				nativeUp(x, y);
				break;
			default:
				break;
		}
		
		return true;
	}
	
}
