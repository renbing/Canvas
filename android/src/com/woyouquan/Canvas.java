package com.woyouquan;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Paint;
import android.media.MediaPlayer;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.method.ScrollingMovementMethod;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Window;
import android.view.WindowManager;
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

class EAGLView extends GLSurfaceView {
    private EAGLRenderer mRenderer;
    
    public EAGLView(Context context, Handler handler) {
        super(context);
        // TODO Auto-generated constructor stub
        
        mRenderer = new EAGLRenderer(handler);
        setRenderer(mRenderer);
    }
    
    public void onDestroy() {
    	mRenderer.onDestroy();
    }
    
    public boolean onTouchEvent(final MotionEvent event) {
    	queueEvent(new Runnable() {
    		public void run() {
    			mRenderer.handleTouchEvent(event);
    		}
    	});

		return true;
    }
}

class EAGLRenderer implements GLSurfaceView.Renderer {

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
    
    public EAGLRenderer(Handler handler) {
    	uiHandler = handler;
    }
    
    public void onDrawFrame(GL10 gl) {
        // TODO Auto-generated method stub
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
		//drawText(gl, "fps:" + String.valueOf(fps), 0, 24, 64, 32);
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        // TODO Auto-generated method stub
        nativeResize(width, height);
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        // TODO Auto-generated method stub
    	nativeInit();
    }
    
    public void onDestroy() {
    	
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
    
    public void drawText(GL10 gl, String text, int x, int y, int w, int h) {
    	// Create an empty, mutable bitmap
    	Bitmap bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
    	// get a canvas to paint over the bitmap
    	android.graphics.Canvas canvas = new android.graphics.Canvas(bitmap);
    	bitmap.eraseColor(Color.WHITE);

    	// Draw the text
    	Paint textPaint = new Paint();
    	textPaint.setTextSize(24);
    	textPaint.setAntiAlias(true);
    	textPaint.setARGB(0xff, 0xff, 0x00, 0x00);
    	// draw the text centered
    	canvas.drawText(text, x, y, textPaint);

    	int[] textures = new int[1];
		//Generate one texture pointer...
    	gl.glGenTextures(1, textures, 0);
    	//...and bind it to our array
    	gl.glBindTexture(GL10.GL_TEXTURE_2D, textures[0]);

    	//Create Nearest Filtered Texture
    	gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
    	gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);

    	//Different possible texture parameters, e.g. GL10.GL_CLAMP_TO_EDGE
    	gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_REPEAT);
    	gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_REPEAT);

    	GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bitmap, 0);

    	bitmap.recycle();
    	
    	float textureCoords[] = {
    		0.0f, 1.0f,
    		1.0f, 1.0f,
    		0.0f, 0.0f,
    		1.0f, 0.0f,
    	};
    	
    	float vertices[] = {
    		0, -h,
    		w, -h,
    		0, 0,
    		w, 0
    	};
    	
    	ByteBuffer vbb = ByteBuffer.allocateDirect(vertices.length*4);
    	vbb.order(ByteOrder.nativeOrder());
    	FloatBuffer vertexBuffer = vbb.asFloatBuffer();
    	vertexBuffer.put(vertices);
    	vertexBuffer.position(0);
    	
    	ByteBuffer tbb = ByteBuffer.allocateDirect(textureCoords.length*4);
    	tbb.order(ByteOrder.nativeOrder());
    	FloatBuffer textureBuffer = tbb.asFloatBuffer();
    	textureBuffer.put(textureCoords);
    	textureBuffer.position(0);
    	
    	gl.glEnable(GL10.GL_TEXTURE_2D);
    	gl.glBindTexture(GL10.GL_TEXTURE_2D, textures[0]);
    	gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
    	
    	gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, textureBuffer);
    	gl.glVertexPointer(2, GL10.GL_FLOAT, 0, vertexBuffer);
    	//gl.glColor4f(1, 0, 0, 1);
    	gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, 4);
    	
    	gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
    	gl.glDisable(GL10.GL_TEXTURE_2D);
    	gl.glDeleteTextures(1, textures, 0);
    	gl.glColor4f(1, 1, 1, 1);
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
    	JavaAudioPlayer player = new JavaAudioPlayer();
    	return player;
    }
}