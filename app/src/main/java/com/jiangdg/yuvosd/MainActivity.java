package com.jiangdg.yuvosd;

import android.app.Activity;
import android.hardware.Camera;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

import com.jiangdg.yuvosd.utils.CameraUtils;
import com.jiangdg.yuvosd.utils.MediaMuxerUtils;
import com.jiangdg.yuvosd.utils.SensorAccelerometer;
import com.jiangdg.yuvosd.utils.CameraUtils.OnCameraFocusResult;
import com.jiangdg.yuvosd.utils.SensorAccelerometer.OnSensorChangedResult;

public class MainActivity extends Activity implements SurfaceHolder.Callback{
    private Button mBtnRecord;
    private Button mBtnSwitchCam;
    private SurfaceView mSurfaceView;
    private CameraUtils mCamManager;
    private boolean isRecording;
	//加速传感器
	private static SensorAccelerometer mSensorAccelerometer;
	byte[] nv21 = new byte[CameraUtils.PREVIEW_WIDTH * CameraUtils.PREVIEW_HEIGHT * 3/2];

    private CameraUtils.OnPreviewFrameResult mPreviewListener = new CameraUtils.OnPreviewFrameResult() {
        @Override
        public void onPreviewResult(byte[] data, Camera camera) {
            mCamManager.getCameraIntance().addCallbackBuffer(data);
			if(CameraUtils.isUsingYv12 ){
//				YuvUtils.swapYV12ToNV21(data,CameraUtils.PREVIEW_WIDTH, CameraUtils.PREVIEW_HEIGHT);
				YuvUtils.swYV12ToNV21(data, nv21, CameraUtils.PREVIEW_WIDTH, CameraUtils.PREVIEW_HEIGHT);
				MediaMuxerUtils.getMuxerRunnableInstance().addVideoFrameData(nv21);
			}else{
				MediaMuxerUtils.getMuxerRunnableInstance().addVideoFrameData(data);
			}	
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mCamManager = CameraUtils.getCamManagerInstance(MainActivity.this);
		//实例化加速传感器
		mSensorAccelerometer = SensorAccelerometer.getSensorInstance();
		
        mSurfaceView = (SurfaceView)findViewById(R.id.main_record_surface);
        mSurfaceView.getHolder().addCallback(this);
        mSurfaceView.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				mCamManager.cameraFocus(new OnCameraFocusResult() {					
					@Override
					public void onFocusResult(boolean result) {
						if(result){
							Toast.makeText(MainActivity.this, "对焦成功", Toast.LENGTH_SHORT).show();
						}
					}
				});
			}
		});
        
        mBtnRecord = (Button)findViewById(R.id.main_record_btn);
        mBtnRecord.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                MediaMuxerUtils mMuxerUtils = MediaMuxerUtils.getMuxerRunnableInstance();
                if(!isRecording){
                    mMuxerUtils.startMuxerThread(mCamManager.getCameraDirection());
                    mBtnRecord.setText("停止录像");
                }else{
                    mMuxerUtils.stopMuxerThread();
                    mBtnRecord.setText("开始录像");
                }
                isRecording = !isRecording;
            }
        });
        
        mBtnSwitchCam = (Button)findViewById(R.id.main_switch_camera_btn);
        mBtnSwitchCam.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				if(isRecording){
					Toast.makeText(MainActivity.this, "正在录像，无法切换",
							Toast.LENGTH_SHORT).show();
					return;
				}
				if(mCamManager != null){
					mCamManager.switchCamera();
				}
			}
		});
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        mCamManager.setSurfaceHolder(surfaceHolder);
        mCamManager.setOnPreviewResult(mPreviewListener);
        mCamManager.createCamera();
        mCamManager.startPreview();
        startSensorAccelerometer();
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        mCamManager.stopPreivew();
        mCamManager.destoryCamera();
        stopSensorAccelerometer();
    }
    
	private void startSensorAccelerometer() {
		// 启动加速传感器，注册结果事件监听器
		if (mSensorAccelerometer != null) {
			mSensorAccelerometer.startSensorAccelerometer(MainActivity.this,
					new OnSensorChangedResult() {
						@Override
						public void onStopped() {					
							// 对焦成功，隐藏对焦图标
							mCamManager.cameraFocus(new OnCameraFocusResult() {
								@Override
								public void onFocusResult(boolean reslut) {
						
								}
							});
						}

						@Override
						public void onMoving(int x, int y, int z) {
//							Log.i(TAG, "手机移动中：x=" + x + "；y=" + y + "；z=" + z);
						}
					});
		}
	}

	private void stopSensorAccelerometer() {
		// 释放加速传感器资源
		if (mSensorAccelerometer == null) {
			return;
		}
		mSensorAccelerometer.stopSensorAccelerometer();
	}
}
