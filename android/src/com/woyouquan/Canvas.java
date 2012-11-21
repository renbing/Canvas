package com.woyouquan;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Paint.Cap;
import android.graphics.Paint.Join;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Typeface;
import android.graphics.Xfermode;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.InputType;
import android.text.StaticLayout;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View.OnFocusChangeListener;
import android.view.View.OnKeyListener;
import android.view.View.OnTouchListener;
import android.view.Window;
import android.view.WindowManager;
import android.view.Gravity;
import android.widget.AbsoluteLayout;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;

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
    	
    	//setContentView(new MyView(this));

    	
    	setContentView(R.layout.main);
    	
        mGLView = new EAGLView(this, handler);
        //mGLView.setOnTouchListener(new TouchListener());
        
        AbsoluteLayout layout = (AbsoluteLayout)findViewById(R.id.mainLayout);
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
        
        EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setHint("hello");
        input.setHintTextColor(Color.RED);
        //input.setBackgroundColor(Color.TRANSPARENT);
        //input.setBackgroundDrawable(null);
        input.setTextColor(Color.RED);
        input.setGravity(Gravity.CENTER_VERTICAL | Gravity.CENTER_HORIZONTAL);
        input.setImeOptions(EditorInfo.IME_ACTION_DONE);
        input.setOnTouchListener(new OnTouchListener() {
        	public boolean onTouch(View v, MotionEvent event) {
        		((EditText)v).setText(String.valueOf(v.hasFocus()));
    			InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
    			imm.showSoftInput(v, 0);
        		return false;
        	}
        });
        input.setOnEditorActionListener(new OnEditorActionListener() {
        	public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        		if( actionId == EditorInfo.IME_ACTION_DONE
        				|| event.getAction() == KeyEvent.ACTION_DOWN 
        				|| event.getKeyCode() == KeyEvent.KEYCODE_ENTER){
        			v.setText("end");
        			v.setInputType(InputType.TYPE_NULL);
        			InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
        			imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
        			return true;
        		}
        		return false;
        	}
        });
        
        //AbsoluteLayout.LayoutParams inputLP = new AbsoluteLayout.LayoutParams(400, 62, 0, 200);
        //layout.addView(input, inputLP);
        
        //stringFromJNI();
    }
    
    @Override
    public void onDestroy()
    {
    	super.onDestroy();
    	//mGLView.onPause();
    	
    	//nativeDone();
    	android.os.Process.killProcess(android.os.Process.myPid());
    }
    
	@Override
	protected void onPause() {
		super.onPause();
		//mGLView.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
		//mGLView.onResume();
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
    	return new JavaAudioPlayer(this.getContext());
    }
    
    public JavaBitmap newBitmap(int w, int h) {
    	return new JavaBitmap(this.getContext(), w, h);
    }
    
    public void createTextBitmap(String text, int w, int h, int pw, int ph, String textAlign, String fontName, 
    								float fontSize, String fontSizeUnit, int textColor, boolean bold, boolean italic, byte[] pixels) {
       	
    	JavaBitmap jb = new JavaBitmap(this.getContext(), pw, ph);
    	jb.fillText(text, 0, 0, w, h, textAlign, fontName, fontSize, fontSizeUnit, textColor, bold, italic);
    	jb.getPixels(pixels);
    }
}

class MyView extends View {

	public MyView(Context context) {
		super(context);
	}
	protected void onDraw(android.graphics.Canvas canvas) {
		super.onDraw(canvas);
		canvas.drawColor(Color.WHITE);
		/*
		JavaBitmap bitmap = new JavaBitmap(this.getContext(), canvas, 200, 200);
		bitmap.beginPath();
		bitmap.moveTo(0, 0);
		bitmap.lineTo(100, 0);
		bitmap.lineTo(100, 100);
		bitmap.closePath();
		bitmap.setContextState("round", "round", 3, 10, Color.RED, Color.RED, false);
		bitmap.stroke();
		bitmap.clearRect(20, 20, 50, 50);
		bitmap.fillText("hello world", 100, 100, 50, 100, "center", "sans-serif", 14, "px", Color.RED, true, true, null);

		
		canvas.drawColor(Color.WHITE);
		
		Paint paint = new Paint();
		paint.setAntiAlias(true);
		paint.setColor(Color.RED);
		paint.setStyle(Paint.Style.STROKE);
		paint.setStrokeWidth(3);
		//paint.setStrokeJoin(Join.valueOf("ROUND"));
		
		Path path = new Path();
		path.moveTo(0, 0);
		path.lineTo(100, 100);
		path.lineTo(100, 0);
		path.close();
		canvas.drawPath(path, paint);
		
		paint.setColor(Color.TRANSPARENT);
		paint.setStyle(Paint.Style.FILL);
		Xfermode mode = paint.getXfermode();
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
		canvas.drawRect(100, 100, 200, 200, paint);
		paint.setXfermode(mode);
		canvas.drawRect(20, 20, 100, 100, paint);
		*/
	}
}