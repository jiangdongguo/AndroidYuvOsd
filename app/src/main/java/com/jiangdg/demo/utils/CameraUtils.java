package com.jiangdg.demo.utils;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.Iterator;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

/** Camera操作封装类
 * Created by jiangdongguo on 2017/5/6.
 */
public class CameraUtils {
	private static final String TAG = "CameraManager";
	public static int PREVIEW_WIDTH = 640;
	public static int PREVIEW_HEIGHT = 480;
	public static boolean isUsingYv12 = false;

	private Camera mCamera;
	private static Context mContext;
	private boolean isFrontCamera = false;
	private OnPreviewFrameResult mPreviewListener;
	private WeakReference<SurfaceHolder> mHolderRef;
	private static CameraUtils mCameraManager;
	private CameraUtils() {}
	
	public interface OnPreviewFrameResult{
		void onPreviewResult(byte[] data, Camera camera);
	}
	
	public interface OnCameraFocusResult{
		void onFocusResult(boolean result);
	}
	
	public static CameraUtils getCamManagerInstance(Context mContext){
		CameraUtils.mContext = mContext;
		if(mCameraManager == null){
			mCameraManager = new CameraUtils();
		}
		return mCameraManager;
	} 
	
	//将预览数据回传到onPreviewResult方法中
	private PreviewCallback previewCallback = new PreviewCallback() {	
		private boolean rotate = false;
		
		@Override
		public void onPreviewFrame(byte[] data, Camera camera) {
			mPreviewListener.onPreviewResult(data, camera);
		}
	};

	public void setOnPreviewResult(OnPreviewFrameResult mPreviewListener){
		this.mPreviewListener = mPreviewListener;
	}

	public void setSurfaceHolder(SurfaceHolder mSurfaceHolder){
		if(mHolderRef != null){
			mHolderRef.clear();
			mHolderRef = null;
		}
		mHolderRef = new WeakReference<SurfaceHolder>(mSurfaceHolder);
	}

	public void startPreview() {		
		if (mCamera == null) {
			return;
		}
		//设定预览控件
			try {
				Log.i(TAG, "CameraManager-->开始相机预览");
				mCamera.setPreviewDisplay(mHolderRef.get());
			} catch (IOException e) {
				e.printStackTrace();
			}
			//开始预览Camera
			try {
				mCamera.startPreview();
			} catch (RuntimeException e){
				Log.i(TAG, "相机预览失败，重新启动Camera.");
				stopPreivew();
				destoryCamera();
				createCamera();
				startPreview();
			}
		//自动对焦
		mCamera.autoFocus(null);
		//设置预览回调缓存
        int previewFormat = mCamera.getParameters().getPreviewFormat();
        Size previewSize = mCamera.getParameters().getPreviewSize();
        int size = previewSize.width * previewSize.height * ImageFormat.getBitsPerPixel(previewFormat) / 8;
        mCamera.addCallbackBuffer(new byte[size]);        
        mCamera.setPreviewCallbackWithBuffer(previewCallback);
	}

	public void stopPreivew(){
		if(mCamera==null){
			return;
		}
		try {
			mCamera.setPreviewDisplay(null);
			mCamera.setPreviewCallbackWithBuffer(null);
			mCamera.stopPreview();
			Log.i(TAG, "CameraManager-->停止相机预览");
		} catch (IOException e) {
			e.printStackTrace();
		}		
	}

	public void createCamera(){
		//创建Camera
		openCamera();
		setCamParameters();
	}

	private void openCamera() {
		if(mCamera != null){
			stopPreivew();
			destoryCamera();
		}
		//打开前置摄像头
		if(isFrontCamera ){
			CameraInfo cameraInfo = new CameraInfo();
			int camNums = Camera.getNumberOfCameras();
			for (int i = 0; i < camNums; i++) {
				Camera.getCameraInfo(i, cameraInfo);
				if(cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT){
					try {
						mCamera = Camera.open(i);
						Log.i(TAG, "CameraManager-->创建Camera对象，开启前置摄像头");
						break;
					} catch (Exception e) {
						Log.d(TAG, "打开前置摄像头失败："+e.getMessage());
					}				
				}
			}
		}else{
			try {
				mCamera = Camera.open();
				Log.i(TAG, "CameraManager-->创建Camera对象，开启后置摄像头");
			} catch (Exception e) {
				Log.d(TAG, "打开后置摄像头失败："+e.getMessage());
			}			
		}
	}

	public void destoryCamera() {
		if(mCamera==null){
			return;
		}		
		mCamera.release();
		mCamera = null;
		Log.i(TAG, "CameraManager-->释放相机资源");
	}
	
	private void setCamParameters() {
		if(mCamera == null)
			return;
		Camera.Parameters params = mCamera.getParameters();
		if(isUsingYv12){
			params.setPreviewFormat(ImageFormat.YV12);
		}else{
			params.setPreviewFormat(ImageFormat.NV21);
		}
		//开启自动对焦
		List<String> focusModes = params.getSupportedFocusModes();
		if(isSupportFocusAuto(focusModes)){
			params.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
		}
		//设置预览分辨率，问题出在这里
		List<Size> previewSizes = params.getSupportedPreviewSizes();
		if(!isSupportPreviewSize(previewSizes)){			
			PREVIEW_WIDTH = previewSizes.get(0).width;
			PREVIEW_HEIGHT = previewSizes.get(0).height;
		}	
		params.setPreviewSize(PREVIEW_WIDTH,PREVIEW_HEIGHT);
		//设置预览的最大、最小像素
		int[] max = determineMaximumSupportedFramerate(params);
		params.setPreviewFpsRange(max[0], max[1]);		
		//使参数配置生效
		mCamera.setParameters(params);
		//旋转预览方向
		int rotateDegree = getPreviewRotateDegree();
        mCamera.setDisplayOrientation(rotateDegree);
	}

	public void cameraFocus(final OnCameraFocusResult listener){
		if(mCamera != null){
			mCamera.autoFocus(new AutoFocusCallback() {			
				@Override
				public void onAutoFocus(boolean success, Camera camera) {
					if(listener != null){
						listener.onFocusResult(success);
					}				
				}
			});
		}	
	}
	
	private int getPreviewRotateDegree(){
		int phoneDegree = 0;
		int result = 0;
		//获得手机方向
		int phoneRotate =((Activity)mContext).getWindowManager().getDefaultDisplay().getOrientation();
		//得到手机的角度
		switch (phoneRotate) {
		case Surface.ROTATION_0: phoneDegree = 0; break;   	 //旋转90度
		case Surface.ROTATION_90: phoneDegree = 90; break;	//旋转0度
		case Surface.ROTATION_180: phoneDegree = 180; break;//旋转270
		case Surface.ROTATION_270: phoneDegree = 270; break;//旋转180
		}
		//分别计算前后置摄像头需要旋转的角度
		CameraInfo cameraInfo = new CameraInfo();
		if(isFrontCamera){			
			Camera.getCameraInfo(CameraInfo.CAMERA_FACING_FRONT, cameraInfo);
			result = (cameraInfo.orientation + phoneDegree) % 360;
			result = (360 - result) % 360;
		}else{
			Camera.getCameraInfo(CameraInfo.CAMERA_FACING_BACK, cameraInfo);
			result = (cameraInfo.orientation - phoneDegree +360) % 360;
		}
		return result;
	}
	
	private boolean isSupportFocusAuto(List<String> focusModes){
		boolean isSupport = false;
		for (String mode:focusModes) {
			if(mode.equals(Camera.Parameters.FLASH_MODE_AUTO)){
				isSupport = true;
				break;
			}
		}
		return isSupport;
	}

	private boolean isSupportPreviewSize(List<Size> previewSizes) {
		boolean isSupport = false;
		for (Size size : previewSizes) {
			if ((size.width == PREVIEW_WIDTH && size.height == PREVIEW_HEIGHT)
					|| (size.width == PREVIEW_HEIGHT && size.height == PREVIEW_WIDTH)) {
				isSupport = true;
				break;
			}
		}
		return isSupport;
	}

	public void switchCamera(){
		isFrontCamera = !isFrontCamera;
		createCamera();
		startPreview();
	}

    public void setPreviewSize(int width, int height) {
        PREVIEW_WIDTH = width;
        PREVIEW_HEIGHT = height;
    }

    public int getPreviewFormat(){
    	if(mCamera == null){
    		return -1;
    	}
    	return mCamera.getParameters().getPreviewFormat();
    }

	public Camera getCameraIntance() {
		return mCamera;
	}

	public SurfaceHolder getSurfaceHolder() {
		if(mHolderRef == null){
			return null;
		}
		return mHolderRef.get();
	}

	public boolean getCameraDirection() {
		return isFrontCamera;
	}
	
	public static int[] determineMaximumSupportedFramerate(Camera.Parameters parameters) {
		int[] maxFps = new int[] { 0, 0 };
		List<int[]> supportedFpsRanges = parameters.getSupportedPreviewFpsRange();
		for (Iterator<int[]> it = supportedFpsRanges.iterator(); it.hasNext();) {
			int[] interval = it.next();
			if (interval[1] > maxFps[1]|| (interval[0] > maxFps[0] && interval[1] == maxFps[1])) {
				maxFps = interval;
			}
		}
		return maxFps;
	}
}
