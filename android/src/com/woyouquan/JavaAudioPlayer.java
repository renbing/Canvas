package com.woyouquan;

import java.io.IOException;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.SoundPool;
import android.media.SoundPool.OnLoadCompleteListener;
import android.util.Log;

public class JavaAudioPlayer implements OnPreparedListener, OnCompletionListener, OnLoadCompleteListener{
	
	private Context mContext;
	private MediaPlayer mPlayer;
	private boolean mPlaying;
	private boolean mPlayerPrepared;
	private boolean mAutoplay;
	private boolean mLoop;
	private float mVolume;
	private String mSource;
	
	private SoundPool mSoundPool;
	private int mSoundID;
	private int mSoundStreamID;
	
	public JavaAudioPlayer(Context context) {
		mContext = context;
		mPlayer = new MediaPlayer();
    	mPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
    	
		mSoundPool = new SoundPool(16, AudioManager.STREAM_MUSIC, 0);
		mSoundPool.setOnLoadCompleteListener(this);
    	
    	mPlayerPrepared = false;
    	mPlaying = false;
    	mAutoplay = false;
    	mLoop = false; 
    	mVolume = 1.0f;
    	
    	mSoundID = 0;
    	mSoundStreamID = 0;
	}

    public void setSource(String path) {
    	
    	mSource = path;
    	mPlayer.reset();

		mSoundPool.stop(mSoundStreamID);
		mSoundStreamID = 0;
		mSoundPool.unload(mSoundID);
		mSoundID = 0;
		
		setLoop(mLoop);
		setVolume(mVolume);
    	
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
    	mLoop = loop;
    	mPlayer.setLooping(mLoop);
    	int loopMode = mLoop ? -1 : 0;
    	mSoundPool.setLoop(mSoundStreamID, loopMode);
    }
    
    public void setAutoplay(boolean autoplay) {
    	mAutoplay = autoplay;
    }
    
    public float getVolume(){
    	AudioManager mgr = (AudioManager)mContext.getSystemService(Context.AUDIO_SERVICE);
        
    	float streamVolumeCurrent = mgr.getStreamVolume(AudioManager.STREAM_MUSIC);   
        float streamVolumeMax = mgr.getStreamMaxVolume(AudioManager.STREAM_MUSIC);      
        float volume = streamVolumeCurrent/streamVolumeMax;
        
        return volume;
    }
    
	public void onPrepared(MediaPlayer mp) {
		
		if( mp.getDuration() < 3000 )
		{
			// 短音
			mSoundID = mSoundPool.load(mSource, 1);
			mp.reset();
			return;
		}
		else
		{
			mPlayerPrepared = true;
			if( mAutoplay || mPlaying )
			{
				mp.start();
				mPlaying = true;
			}
		}
	}
	
	public void onCompletion(MediaPlayer mp) {
	}
	
	public void play() {
		mPlaying = true;
		if( mSoundID > 0 )
		{
			int loopMode = mLoop ? -1 : 0;
			mSoundStreamID = mSoundPool.play(mSoundID, mVolume, mVolume, 1, loopMode, 1f);
		}
		else if( mPlayerPrepared )
		{
			mPlayer.start();
		}
	}
	
	public void pause() {
		mPlaying = false;
		if( mSoundStreamID > 0 )
		{
			mSoundPool.pause(mSoundStreamID);
		}
		else if( mPlayerPrepared )
		{
			mPlayer.pause();
		}
	}
	
	public void setVolume(float volume) {
		mVolume = volume;
		mPlayer.setVolume(volume, volume);
		mSoundPool.setVolume(mSoundStreamID, volume, volume);
	}
	
	public void release() {
		mPlayer.release();
		mSoundPool.stop(mSoundStreamID);
		mSoundPool.unload(mSoundID);
		mSoundPool.release();
	}

	public void onLoadComplete(SoundPool soundPool, int sampleId, int status) {
		if( mPlaying || mAutoplay )
		{
			play();
		}
	}
}
