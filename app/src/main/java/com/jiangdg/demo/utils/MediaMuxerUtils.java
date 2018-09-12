package com.jiangdg.demo.utils;

import java.io.File;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.Vector;

import com.jiangdg.demo.runnable.EncoderAudioRunnable;
import com.jiangdg.demo.runnable.EncoderVideoRunnable;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Environment;
import android.util.Log;
/**MP4混合器
 * Created by jiangdongguo on 2017/5/6.
 */

public class MediaMuxerUtils{
    private static final String ROOT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath();
    private static final String TAG = "MediaMuxerUtils";
    public static final int TRACK_VIDEO = 0;
    public static final int TRACK_AUDIO = 1;
    private boolean isVideoAdded;
    private boolean isAudioAdded;
    private boolean isMuxerStarted;
    private boolean isExit = false;
    private int videoTrack = -1;
    private int audioTrack = -1;

    private Object lock = new Object();
    private Vector<MuxerData> mMuxerDatas;
    private MediaMuxer mMuxer;
    private MediaFormat videoMediaFormat;
    private MediaFormat audioMediaFormat;
    private EncoderVideoRunnable videoRunnable;
    private EncoderAudioRunnable audioRunnable;
    private Thread mMuxerThread;
    private Thread mVideoThread;
    private Thread mAudioThread;
    private boolean isFrontCamera;
    private static MediaMuxerUtils muxerUtils;

    private MediaMuxerUtils(){}

    public static MediaMuxerUtils getMuxerRunnableInstance(){
        if(muxerUtils == null){
            muxerUtils = new MediaMuxerUtils();
        }
        return muxerUtils;
    }
    
    private void initMuxer(){
		try {
			mMuxer = new MediaMuxer(ROOT_PATH + File.separator
					+ System.currentTimeMillis() + ".mp4",
					MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
		} catch (Exception e) {
			e.printStackTrace();
		}
        mMuxerDatas = new Vector<>();
        videoRunnable = new EncoderVideoRunnable(new WeakReference<>(this));
        audioRunnable = new EncoderAudioRunnable(new WeakReference<>(this));
        mVideoThread = new Thread(videoRunnable);
        mAudioThread = new Thread(audioRunnable);
        videoRunnable.setFrontCamera(isFrontCamera);
        mAudioThread.start();
        mVideoThread.start();
         isExit = false;
    }
    
    class MediaMuxerRunnable implements  Runnable{
        @Override
        public void run() {
        	initMuxer();
            while (!isExit){
                // 混合器没有启动或数据缓存为空，则阻塞混合线程等待启动(数据输入)
            	if(isMuxerStarted){
                    // 从缓存读取数据写入混合器中       
                    if(mMuxerDatas.isEmpty()){
                    	Log.w(TAG, "run--->混合器没有数据，阻塞线程等待");
                        synchronized (lock){
                            try{
                                lock.wait();
                            }catch(Exception e){
                                e.printStackTrace();
                            }
                        }
                    }else{
                        MuxerData data = mMuxerDatas.remove(0);
                        if(data != null){
                            int track = 0;
                            try{
                                if(data.trackIndex == TRACK_VIDEO){
                                    track = videoTrack;
                                    Log.d(TAG,"---写入视频数据---");
                                }else if(data.trackIndex == TRACK_AUDIO){
                                    Log.d(TAG,"---写入音频数据---");
                                    track = audioTrack;
                                }
                                mMuxer.writeSampleData(track,data.byteBuf,data.bufferInfo);
                            }catch(Exception e){
                                Log.e(TAG,"写入数据到混合器失败，track="+track);
                                e.printStackTrace();
                            }
                        }
                    }
            	}else{
                 	Log.w(TAG, "run--->混合器没有启动，阻塞线程等待");
                    synchronized (lock){
                        try{
                            lock.wait();
                        }catch(Exception e){
                            e.printStackTrace();
                        }
                    }
                }
            }
            stopMuxer();
        }
    }

    private void startMuxer(){
        if(mMuxer == null){
            Log.e(TAG,"启动混合器失败，mMuxer=null");
            return;
        }
        if(isCanStartMuxer() && !isMuxerStarted){
            mMuxer.start();
            isMuxerStarted = true;
            synchronized (lock) {
                lock.notify();
			}
            Log.d(TAG,"---启动混合器---");
        }
    }

    private void stopMuxer(){
        if(mMuxer == null){
            Log.e(TAG,"停止混合器失败，mMuxer=null");
            return;
        }
        Log.d(TAG,"---停止混合器---");
        if(isMuxerStarted){
            mMuxer.stop();
            mMuxer.release();
            videoMediaFormat = null;
            audioMediaFormat = null;
            isVideoAdded = false;
            isAudioAdded = false;
            isMuxerStarted = false;
            mMuxer = null;
        }
    }

    // 添加音、视频轨道
    public void setMediaFormat(int index,MediaFormat meidaFormat){
        if(mMuxer == null && isMuxerStarted){
            Log.e(TAG,"添加轨道失败或混合器已经启动，index="+index);
            return;
        }
        if(index == TRACK_VIDEO){
            if(videoMediaFormat == null){
                videoMediaFormat = meidaFormat;
                videoTrack = mMuxer.addTrack(videoMediaFormat);
                isVideoAdded = true;
                Log.d(TAG,"---添加视频轨道到混合器---");
            }
        }else if(index == TRACK_AUDIO){
            if(audioMediaFormat == null){
                audioMediaFormat = meidaFormat;
                audioTrack = mMuxer.addTrack(audioMediaFormat);
                isAudioAdded = true;
                Log.d(TAG,"---添加音频轨道到混合器---");
            }
        }
        startMuxer();
    }

	// 向MediaMuxer添加数据
    public void addMuxerData(MuxerData data){
        if(mMuxerDatas == null){
            Log.e(TAG,"添加数据失败");
            return;
        }
        mMuxerDatas.add(data);
        // 解锁
        synchronized (lock){
            lock.notify();
        }
    }

    // 添加图像数据到视频编码器
    public void addVideoFrameData(byte[] frameData){
        if(videoRunnable != null){
            videoRunnable.addData(frameData);
        }
    }

    public void startMuxerThread(boolean isFrontCamera){
        Log.d(TAG,"---启动混合器线程---");
        this.isFrontCamera = isFrontCamera;
    	if(mMuxerThread == null){
    		synchronized (MediaMuxerUtils.this) {
    	        mMuxerThread =  new Thread(new MediaMuxerRunnable());
    	        mMuxerThread.start();
			}
    	}
    }

    public void stopMuxerThread(){
    	exit();
    	if(mMuxerThread != null){
    		try {
				mMuxerThread.join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
    	}
    	mMuxerThread = null;
    }
    
    private void exit(){
        Log.d(TAG,"---停止混合器(录音、录像)线程---");
        // 清理视频录制线程资源
        if(videoRunnable != null){
            videoRunnable.exit();
        }
        if(mVideoThread != null){
        	try {
				mVideoThread.join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
        	mVideoThread = null;
        }
        //  清理录音线程资源
        if(audioRunnable != null){
            audioRunnable.exit();
        }
        if(mAudioThread != null){
        	try {
				mAudioThread.join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
        	mAudioThread = null;
        }
        isExit = true;
    	synchronized (lock) {
			lock.notify();
		}
    }

    public boolean  isMuxerStarted(){
    	return isMuxerStarted;
    }
    
    public boolean isVideoAdded() {
        return isVideoAdded;
    }

    public boolean isAudioAdded() {
        return isAudioAdded;
    }

    private boolean isCanStartMuxer(){
        return isVideoAdded & isAudioAdded;
    }

    /**
     * 封装要混合器数据实体
     */
    public static class MuxerData {
        int trackIndex;
        ByteBuffer byteBuf;
        MediaCodec.BufferInfo bufferInfo;

        public MuxerData(int trackIndex, ByteBuffer byteBuf, MediaCodec.BufferInfo bufferInfo) {
            this.trackIndex = trackIndex;
            this.byteBuf = byteBuf;
            this.bufferInfo = bufferInfo;
        }
    }
}
