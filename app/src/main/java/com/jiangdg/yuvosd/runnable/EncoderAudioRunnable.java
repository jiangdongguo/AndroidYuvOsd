package com.jiangdg.yuvosd.runnable;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

import com.jiangdg.yuvosd.utils.MediaMuxerUtils;

import android.annotation.SuppressLint;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import android.os.Build;
import android.os.Process;
import android.util.Log;

/** 对ACC音频进行编码
 * Created by jiangdongguo on 2017/5/6.
 */
public class EncoderAudioRunnable implements Runnable {
    private static final String TAG = "EncoderAudioRunnable";
    private static final String MIME_TYPE = "audio/mp4a-latm";
    private static final int TIMES_OUT = 1000;
    private static final int BIT_RATE = 16000;
    private static final int CHANNEL_COUNT = 1;
    private static final int SMAPLE_RATE = 8000;
    private static final int ACC_PROFILE = MediaCodecInfo.CodecProfileLevel.AACObjectLC;
    private static final int BUFFER_SIZE = 1600;
    private static final int AUDIO_BUFFER_SIZE = 1024;
    // 录音
    private static final int channelConfig = AudioFormat.CHANNEL_IN_MONO;
    private static final int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    private static final int audioSouce = MediaRecorder.AudioSource.MIC;
    private AudioRecord mAudioRecord;
    // 编码器
    private boolean isExit = false;
    private boolean isEncoderStarted = false;
    private WeakReference<MediaMuxerUtils> muxerRunnableRf;
    private MediaCodec mAudioEncoder;
	private long prevPresentationTimes;
	private MediaFormat mediaFormat;

    public EncoderAudioRunnable(WeakReference<MediaMuxerUtils> muxerRunnableRf){
        this.muxerRunnableRf = muxerRunnableRf;
        initMediaCodec();
    }

    private void initMediaCodec() {
        MediaCodecInfo mCodecInfo = selectSupportCodec(MIME_TYPE);
        if(mCodecInfo == null){
            Log.e(TAG,"编码器不支持"+MIME_TYPE+"类型");
            return;
        }
        try{
            mAudioEncoder = MediaCodec.createByCodecName(mCodecInfo.getName());
        }catch(IOException e){
            Log.e(TAG,"创建编码器失败"+e.getMessage());
            e.printStackTrace();
        }
        // 告诉编码器输出数据的格式,如MIME类型、码率、采样率、通道数量等
        mediaFormat = new MediaFormat();
        mediaFormat.setString(MediaFormat.KEY_MIME,MIME_TYPE);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE,BIT_RATE);
        mediaFormat.setInteger(MediaFormat.KEY_SAMPLE_RATE,SMAPLE_RATE);
        mediaFormat.setInteger(MediaFormat.KEY_AAC_PROFILE,ACC_PROFILE);
        mediaFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT,CHANNEL_COUNT);
        mediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE,BUFFER_SIZE);
	}

	@Override
	public void run() {
		if (!isEncoderStarted) {
			startAudioRecord();
			startCodec();
		}
		while (!isExit) {
			if (mAudioRecord != null) {
				byte[] audioBuf = new byte[AUDIO_BUFFER_SIZE];
				int readBytes = mAudioRecord.read(audioBuf, 0,AUDIO_BUFFER_SIZE);
				if (readBytes > 0) {
					try {
						encoderBytes(audioBuf, readBytes);
					} catch (IllegalStateException e) {
						// 捕获因中断线程并停止混合dequeueOutputBuffer报的状态异常
						e.printStackTrace();
					} catch (NullPointerException e) {
						// 捕获因中断线程并停止混合MediaCodec为NULL异常
						e.printStackTrace();
					}
				}
			}
		}
		stopCodec();
		stopAudioRecord();
	}

    @SuppressLint("NewApi")
    private void encoderBytes(byte[] audioBuf,int readBytes){
        ByteBuffer[] inputBuffers = mAudioEncoder.getInputBuffers();
        ByteBuffer[] outputBuffers = mAudioEncoder.getOutputBuffers();
        //返回编码器的一个输入缓存区句柄，-1表示当前没有可用的输入缓存区
        int inputBufferIndex = mAudioEncoder.dequeueInputBuffer(TIMES_OUT);
        if(inputBufferIndex >= 0){
            // 绑定一个被空的、可写的输入缓存区inputBuffer到客户端
            ByteBuffer inputBuffer  = null;
            if(!isLollipop()){
                inputBuffer = inputBuffers[inputBufferIndex];
            }else{
                inputBuffer = mAudioEncoder.getInputBuffer(inputBufferIndex);
            }
            // 向输入缓存区写入有效原始数据，并提交到编码器中进行编码处理
            if(audioBuf==null || readBytes<=0){
            	mAudioEncoder.queueInputBuffer(inputBufferIndex,0,0,getPTSUs(),MediaCodec.BUFFER_FLAG_END_OF_STREAM);
            }else{
                inputBuffer.clear();
                inputBuffer.put(audioBuf);
                mAudioEncoder.queueInputBuffer(inputBufferIndex,0,readBytes,getPTSUs(),0);
            }
        }

        // 返回一个输出缓存区句柄，当为-1时表示当前没有可用的输出缓存区
        // mBufferInfo参数包含被编码好的数据，timesOut参数为超时等待的时间
        MediaCodec.BufferInfo  mBufferInfo = new MediaCodec.BufferInfo();
        int outputBufferIndex = -1;
        do{
        	outputBufferIndex = mAudioEncoder.dequeueOutputBuffer(mBufferInfo,TIMES_OUT);
        	if(outputBufferIndex == MediaCodec. INFO_TRY_AGAIN_LATER){
                Log.i(TAG,"获得编码器输出缓存区超时");
            }else if(outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED){
                // 如果API小于21，APP需要重新绑定编码器的输入缓存区；
                // 如果API大于21，则无需处理INFO_OUTPUT_BUFFERS_CHANGED
                if(!isLollipop()){
                    outputBuffers = mAudioEncoder.getOutputBuffers();
                }
            }else if(outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){
                // 编码器输出缓存区格式改变，通常在存储数据之前且只会改变一次
                // 这里设置混合器视频轨道，如果音频已经添加则启动混合器（保证音视频同步）
                MediaFormat newFormat = mAudioEncoder.getOutputFormat();
                MediaMuxerUtils mMuxerUtils = muxerRunnableRf.get();
                if(mMuxerUtils != null){
                    mMuxerUtils.setMediaFormat(MediaMuxerUtils.TRACK_AUDIO,newFormat);
                }
                Log.i(TAG,"编码器输出缓存区格式改变，添加视频轨道到混合器");
            }else{
                // 当flag属性置为BUFFER_FLAG_CODEC_CONFIG后，说明输出缓存区的数据已经被消费了
                if((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0){
                    Log.i(TAG,"编码数据被消费，BufferInfo的size属性置0");
                    mBufferInfo.size = 0;
                }
                // 数据流结束标志，结束本次循环
                if((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0){
                    Log.i(TAG,"数据流结束，退出循环");
                    break;
                }
                // 获取一个只读的输出缓存区inputBuffer ，它包含被编码好的数据
                ByteBuffer outputBuffer = null;
                if(!isLollipop()){
                    outputBuffer  = outputBuffers[outputBufferIndex];
                }else{
                    outputBuffer  = mAudioEncoder.getOutputBuffer(outputBufferIndex);
                }
                if(mBufferInfo.size != 0){
                    // 获取输出缓存区失败，抛出异常
                    if(outputBuffer == null){
                        throw new RuntimeException("encodecOutputBuffer"+outputBufferIndex+"was null");
                    }
                    // 如果API<=19，需要根据BufferInfo的offset偏移量调整ByteBuffer的位置
                    //并且限定将要读取缓存区数据的长度，否则输出数据会混乱
                    if(isKITKAT()){
                        outputBuffer.position(mBufferInfo.offset);
                        outputBuffer.limit(mBufferInfo.offset+mBufferInfo.size);
                    }
                    // 对输出缓存区的H.264数据进行混合处理
                    MediaMuxerUtils mMuxerUtils = muxerRunnableRf.get();
                    mBufferInfo.presentationTimeUs = getPTSUs();
                    if(mMuxerUtils != null && mMuxerUtils.isMuxerStarted()){
                        Log.d(TAG,"------混合音频数据-------");
                        mMuxerUtils.addMuxerData(new MediaMuxerUtils.MuxerData(MediaMuxerUtils.TRACK_AUDIO,outputBuffer,mBufferInfo));
                        prevPresentationTimes = mBufferInfo.presentationTimeUs;
                    }
                }
                // 处理结束，释放输出缓存区资源
                mAudioEncoder.releaseOutputBuffer(outputBufferIndex,false);
            }
        }while (outputBufferIndex >= 0);
    }

    private void startCodec(){
    	isExit = false;
    	if(mAudioEncoder != null){
            mAudioEncoder.configure(mediaFormat,null,null,MediaCodec.CONFIGURE_FLAG_ENCODE);
            mAudioEncoder.start();    
        	isEncoderStarted = true;
    	}  
    }

    private void stopCodec(){
        if(mAudioEncoder != null){
            mAudioEncoder.stop();
            mAudioEncoder.release();
            mAudioEncoder = null;
        }
        isEncoderStarted = false;
    }
    
    private void startAudioRecord(){
        // 计算AudioRecord所需输入缓存空间大小
        int bufferSizeInBytes = AudioRecord.getMinBufferSize(SMAPLE_RATE,channelConfig,audioFormat);
        if(bufferSizeInBytes < 1600){
            bufferSizeInBytes = 1600;
        }
        Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
        mAudioRecord = new AudioRecord(audioSouce,SMAPLE_RATE,channelConfig,audioFormat,bufferSizeInBytes);
        // 开始录音
        mAudioRecord.startRecording();
    }

    public void stopAudioRecord(){
        if(mAudioRecord != null){
            mAudioRecord.stop();
            mAudioRecord.release();
            mAudioRecord = null;
        }
    }

    public void exit(){
        isExit = true;
    }
    
    /**
     * 遍历所有编解码器，返回第一个与指定MIME类型匹配的编码器
     *  判断是否有支持指定mime类型的编码器
     * */
    private MediaCodecInfo selectSupportCodec(String mimeType){
		int numCodecs = MediaCodecList.getCodecCount();
		for (int i = 0; i < numCodecs; i++) {
			MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
			// 判断是否为编码器，否则直接进入下一次循环
			if (!codecInfo.isEncoder()) {
				continue;
			}
			// 如果是编码器，判断是否支持Mime类型
			String[] types = codecInfo.getSupportedTypes();
			for (int j = 0; j < types.length; j++) {
				if (types[j].equalsIgnoreCase(mimeType)) {
					return codecInfo;
				}
			}
		}
		return null;
	}

    private boolean isLollipop(){
        // API>=21
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
    }

    private boolean isKITKAT(){
        // API<=19
        return Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT;
    }
    
    private long getPTSUs(){
    	long result = System.nanoTime()/1000;
    	if(result < prevPresentationTimes){
    		result = (prevPresentationTimes  - result ) + result;
    	}
    	return result;
    }
}
