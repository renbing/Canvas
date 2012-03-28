package com.woyouquan;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Window;
import android.view.WindowManager;
import android.view.Gravity;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.view.View;

public class Canvas extends Activity {
    /** Called when the activity is first created. */
	
	static final String packageName = "com.woyouquan";
	private GLSurfaceView mGLView;
	private TextView mFps;
	private TextView mConsole;
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        //android.os.Debug.waitForDebugger();
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        
        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                
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

    	setContentView(R.layout.main);
    	
        mGLView = new EAGLView(this, handler);
        //mGLView.setOnTouchListener(new TouchListener());
        
        RelativeLayout layout = (RelativeLayout)findViewById(R.id.mainLayout);
        layout.addView(mGLView, 0);
        
        mFps = (TextView)findViewById(R.id.fps);
        mFps.setOnClickListener(new View.OnClickListener() {
			
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if( mConsole.getVisibility() == View.INVISIBLE )
				{
					mConsole.setVisibility(View.VISIBLE);
				}
				else
				{
					mConsole.setVisibility(View.INVISIBLE);
				}
			}
		});
        
        mConsole = (TextView)findViewById(R.id.console);
        mConsole.setMovementMethod(new ScrollingMovementMethod());
        mConsole.setVisibility(View.INVISIBLE);
        
        //stringFromJNI();
    }
    
    @Override
    public void onDestroy()
    {
    	super.onDestroy();
    	mGLView.onPause();
    	
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
	
	public static final int CanvasAlert = 0x101;
	public static final int FpsUpdate = 0x102;
	public static final int ConsoleUpdate = 0x103;
	
	Handler handler = new Handler() {
		public void handleMessage(Message msg) {
			Bundle b = msg.getData();
			switch(msg.what) {
				case Canvas.CanvasAlert:
					String alertContent = b.getString("content");
					alert(alertContent);
					break;
				case Canvas.FpsUpdate:
					int fps = b.getInt("fps");
					mFps.setText("fps:" + String.valueOf(fps));
					break;
				case Canvas.ConsoleUpdate:
					String traceContent = b.getString("content");
					mConsole.append(traceContent + "\n");
					break;
				default:
					break;
			}
			super.handleMessage(msg);
		}
	};
	
	public void alert(String content) {
        AlertDialog dialog = new AlertDialog.Builder(this).setTitle("alert")
        		.setMessage(content)
        		.setPositiveButton("确认", null)
        		.create();
        dialog.show();
	}
	
    private native String stringFromJNI();
    private native void nativeInit(String apkPath);
    private native void nativeDone();
    
    static {
        System.loadLibrary("gljni");
    }
}

class EAGLView extends GLSurfaceView implements GLSurfaceView.Renderer {
	
	private Handler uiHandler;
	private long time = System.currentTimeMillis();
	private int fpCount = 0;
	private int fps = 0;
		
    private native void nativeInit();
    private native void nativeResize(int w, int h);
    private native void nativeRender();
    
	private static native void nativeDown(float x, float y);
	private static native void nativeMove(float x, float y);
	private static native void nativeUp(float x, float y);
	
    public EAGLView(Context context, Handler handler) {
        super(context);
        uiHandler = handler;
        setRenderer(this);
    }
    
    public boolean onTouchEvent(final MotionEvent event) {
    	final EAGLView glView = this;
    	queueEvent(new Runnable() {
    		public void run() {
    			glView.handleTouchEvent(event);
    		}
    	});

		return true;
    }

	public void onDrawFrame(GL10 gl) {
    	gl.glClear(GL10.GL_COLOR_BUFFER_BIT);
    	nativeRender();
    	
    	fpCount++;
    	if( (System.currentTimeMillis() - time) > 500 )
    	{
    		fps = (int) (fpCount * 1000 / (System.currentTimeMillis() - time));
        	//android.util.Log.v("fps", "fps:" + String.valueOf(fps));
    		time = System.currentTimeMillis();
    		fpCount = 0;
    		
            Message msg = new Message();
            msg.what = Canvas.FpsUpdate;
            Bundle data = new Bundle();
            data.putInt("fps", fps);
            msg.setData(data);
            uiHandler.sendMessage(msg);
    	}    	
	}

	public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeResize(width, height);		
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		nativeInit();
	}
	
    public void handleTouchEvent(MotionEvent event) {
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
    }	

    // NDK C++调用Java函数
    public void alert(String content) {
        Message msg = new Message();
        msg.what = Canvas.CanvasAlert;
        Bundle data = new Bundle();
        data.putString("content", content);
        msg.setData(data);
        uiHandler.sendMessage(msg);
    }
    
    public void trace(String content) {
        Message msg = new Message();
        msg.what = Canvas.ConsoleUpdate;
        Bundle data = new Bundle();
        data.putString("content", content);
        msg.setData(data);
        uiHandler.sendMessage(msg);
    }
    
    public JavaAudioPlayer newAudioPlayer() {
    	JavaAudioPlayer player = new JavaAudioPlayer(this.getContext());
    	return player;
    }

    public void createTextBitmap(String text, int w, int h, int pw, int ph, String textAlign, String fontName, 
    								float fontSize, String fontSizeUnit, int textColor, boolean bold, boolean italic, byte[] pixels) {
       	
    	Bitmap bitmap = Bitmap.createBitmap(pw, ph, Bitmap.Config.ARGB_8888);
    	android.graphics.Canvas canvas = new android.graphics.Canvas(bitmap);
    	bitmap.eraseColor(0);

    	TextView textView = new TextView(this.getContext()); 
    	textView.layout(0, 0, w, h);
    	
    	int hGravity = Gravity.LEFT;
    	if( textAlign.equalsIgnoreCase("end") || textAlign.equalsIgnoreCase("right") )
    	{
    		hGravity = Gravity.RIGHT;
    	}
    	else if( textAlign.equalsIgnoreCase("center") )
    	{
    		hGravity = Gravity.CENTER;
    	}
    	textView.setGravity(hGravity);
    	
    	int unit = TypedValue.COMPLEX_UNIT_PX;
    	if( fontSizeUnit.equalsIgnoreCase("pt") )
    	{
    		unit = TypedValue.COMPLEX_UNIT_PT;
    	}
    	else if( fontSizeUnit.equalsIgnoreCase("in") )
    	{
    		unit = TypedValue.COMPLEX_UNIT_IN;
    	}
    	else if( fontSizeUnit.equalsIgnoreCase("mm") )
    	{
    		unit = TypedValue.COMPLEX_UNIT_MM;
    	}
    	else if( fontSizeUnit.equalsIgnoreCase("cm") )
    	{
    		unit = TypedValue.COMPLEX_UNIT_MM;
    		fontSize = fontSize * 10;
    	}
    	
    	textView.setTextSize(unit, fontSize);
    	
    	int styleFace = Typeface.NORMAL;
    	if( bold )
    	{
    		styleFace = styleFace | Typeface.BOLD;
    	}
    	if( italic )
    	{
    		styleFace = styleFace | Typeface.ITALIC;
    	}
    	
    	Typeface fontFace = Typeface.defaultFromStyle(Typeface.NORMAL);
    	if( fontName.equalsIgnoreCase("sans-serif") )
    	{
    		fontFace = Typeface.SANS_SERIF;
    	}
    	else if( fontName.equalsIgnoreCase("serif") )
    	{
    		fontFace = Typeface.SERIF;
    	}
    	else if( fontName.equalsIgnoreCase("monospace") )
    	{
    		fontFace = Typeface.MONOSPACE;
    	}
    	
    	textView.setTypeface(fontFace, styleFace);
    	textView.setTextColor(textColor);
    	textView.setText(text);
    	textView.setMaxLines(h/textView.getLineHeight());
    	textView.setDrawingCacheEnabled(true); 
    	canvas.drawBitmap(textView.getDrawingCache(), 0, 0, null); 
    	
    	//byte[] pixels = new byte[bitmap.getWidth() * bitmap.getHeight() * 4];
    	ByteBuffer buf = ByteBuffer.wrap(pixels);
    	buf.order(ByteOrder.nativeOrder());
    	bitmap.copyPixelsToBuffer(buf);

    	bitmap.recycle();    	
    }
}