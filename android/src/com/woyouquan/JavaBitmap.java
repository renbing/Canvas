package com.woyouquan;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.Paint.Cap;
import android.graphics.Paint.Join;
import android.hardware.Camera.Size;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.TextView;

public class JavaBitmap {

	private Path mPath;
	private android.graphics.Canvas mCanvas;
	private Paint mPaint;
	
	private Bitmap mBitmap;
	private float mBrushX;
	private float mBrushY;
	
	private Context mContext;
	
	public JavaBitmap(Context context, int width, int height) {
		mContext = context;
		mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    	mCanvas = new android.graphics.Canvas(mBitmap);
    	mBitmap.eraseColor(0);
    	
    	mPath = new Path();
    	mPaint = new Paint();
    	mPaint.setAntiAlias(true);
	}
	
	public void beginPath() {
		mPath.reset();
	}
	
	public void closePath() {
		if( !mPath.isEmpty() ) {
			mPath.close();
		}
	}
	
	public void moveTo(float x, float y) {
		mPath.moveTo(x, y);
		
		mBrushX = x;
		mBrushY = y;
	}
	
	public void lineTo(float x, float y) {
		mPath.lineTo(x, y);
		
		mBrushX = x;
		mBrushY = y;
	}
	
	public Rect stroke() {
		RectF boundsf = new RectF();		
		mPath.computeBounds(boundsf, false);
		
		float lineWidth = mPaint.getStrokeWidth();
		boundsf.left -= lineWidth;
		boundsf.top -= lineWidth;
		boundsf.bottom += lineWidth;
		boundsf.right += lineWidth;
		
		mCanvas.drawPath(mPath, mPaint);
		mPath.reset();
		mPath.moveTo(mBrushX, mBrushY);
		
		Rect bounds = new Rect();
		boundsf.roundOut(bounds);
		
		if( bounds.left <= 0 )
		{
			bounds.left = 0;
		}
		if( bounds.top <= 0 )
		{
			bounds.top = 0;
		}
		
		if( bounds.width() > mBitmap.getWidth() )
		{
			bounds.right = mBitmap.getWidth() + bounds.left;
		}
		if( bounds.height() > mBitmap.getHeight() )
		{
			bounds.bottom = mBitmap.getHeight() + bounds.top;
		}
		
		return bounds;
	}
	
	public Rect clearRect(float x, float y, float w, float h) {
		Paint paint = new Paint();
		paint.setColor(Color.TRANSPARENT);
		paint.setStyle(Paint.Style.FILL);
		paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
		mCanvas.drawRect(x, y, x+w, y+h, paint);
		
		RectF rectf = new RectF(x, y, x+w, y+h);
		Rect bounds = new Rect();
		rectf.roundOut(bounds);
		
		return bounds;
	}
	
	public void setContextState(String lineCap, String lineJoin, float lineWidth, float miterLimit, 
			int fillColor, int strokeColor, boolean styleStroke) {

		if( styleStroke ) {
			mPaint.setStyle(Paint.Style.STROKE);
			mPaint.setColor(strokeColor);
			mPaint.setStrokeCap(Cap.valueOf(lineCap.toUpperCase()));
			mPaint.setStrokeJoin(Join.valueOf(lineJoin.toUpperCase()));
			mPaint.setStrokeWidth(lineWidth);
			mPaint.setStrokeMiter(miterLimit);
		} else {
			mPaint.setStyle(Paint.Style.FILL);
			mPaint.setColor(fillColor);
		}
	}
	
	public Rect fillText(String text, int x, int y, int w, int h, String textAlign,String fontName,
				float fontSize, String fontSizeUnit, int textColor, boolean bold, boolean italic) {
    	TextView textView = new TextView(mContext); 
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
    	int maxLine = h/textView.getLineHeight();
    	textView.setMaxLines(maxLine);
    	textView.setDrawingCacheEnabled(true); 
    	mCanvas.drawBitmap(textView.getDrawingCache(), x, y, null);
    	
    	return new Rect(x, y, x + w, y + (int)Math.ceil(maxLine * textView.getLineHeight()));
	}
	
	public void getPixels(byte[] pixels) {		
    	ByteBuffer buf = ByteBuffer.wrap(pixels);
    	//byte 跟字节序无关, RGBA
    	//buf.order(ByteOrder.nativeOrder());
    	mBitmap.copyPixelsToBuffer(buf);
	}
	
	public byte[] getRectPixels(int x, int y, int w, int h) {

		int[] pixels = new int[w * h];

    	mBitmap.getPixels(pixels, 0, w, x, y, w, h);

    	ByteBuffer buf = ByteBuffer.allocate(pixels.length * 4);
    	// 采用ARGB
    	buf.order(ByteOrder.BIG_ENDIAN);
    	buf.asIntBuffer().put(pixels);
    	
    	return buf.array();
	}
}