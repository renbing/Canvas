package com.woyouquan;

import java.io.IOException;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnCompletionListener;

public class JavaAudioPlayer implements OnPreparedListener, OnCompletionListener{
	
	private MediaPlayer mPlayer;
	private boolean mPlaying;
	private boolean mPrepared;
	
	public boolean mAutoplay;
	
	
	public JavaAudioPlayer() {
		mPlayer = new MediaPlayer();
    	mPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
    	
    	mPrepared = false;
    	mPlaying = false;
    	mAutoplay = false;
	}

    public void setSource(String path) {
    	try {
			mPlayer.setDataSource(path);
		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (IllegalStateException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
    	finally
    	{
    		mPlayer.setOnPreparedListener(this);
    		mPlayer.prepareAsync();
    	}
    }
    
    public void setLoop(boolean loop) {
    	mPlayer.setLooping(loop);
    }
    
    public void setAutoplay(boolean autoplay) {
    	mAutoplay = autoplay;
    }
    
	public void onPrepared(MediaPlayer mp) {
		mPrepared = true;
		
		if( mAutoplay || mPlaying )
		{
			mp.start();
			mPlaying = true;
		}
	}
	
	public void onCompletion(MediaPlayer mp) {
		// TODO Auto-generated method stub
	}
	
	public void play() {
		mPlaying = true;
		if( mPrepared )
		{
			mPlayer.start();
		}
	}
	
	public void pause() {
		mPlaying = false;
		if( mPrepared )
		{
			mPlayer.pause();
		}
	}
	
	public void release() {
		mPlayer.release();
	}
}
