package com.jiangdg.yuvosd.utils;

import java.util.Calendar;

import android.annotation.SuppressLint;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

/**加速器传感器，监听手机运动状态，比如静止、移动，用于自动对焦
 *
 * @author Created by jiangdongguo on 2017-3-1下午2:17:40
 */
public class SensorAccelerometer implements SensorEventListener {
	private int status = -1; 
	private static final int STATUS_MOVING = 1;
	private static final int STATUS_STOP = 2;
	private static final String TAG = "SensorAccelerometer";
	private SensorManager mSensorManager;
	private Sensor mAccelerometer;
	private OnSensorChangedResult reslistener;
	private static SensorAccelerometer sensorMeter;
	private static long STATIC_DELAY_TIME = 1000;
	private long laststamp; 
	private int lastX;
	private int lastY;
	private int lastZ;
	//自动对焦标志，防止连续对焦
	private boolean isFocused = false;
	
	//对外回调结果接口
	public interface OnSensorChangedResult{
		void onMoving(int x, int y, int z);
		void onStopped();
	}

	private SensorAccelerometer(){}
	
	public static SensorAccelerometer getSensorInstance(){	
		if(sensorMeter == null){
			sensorMeter = new SensorAccelerometer();
		}
		return sensorMeter;
	}
	
	public  void startSensorAccelerometer(Context mContext,OnSensorChangedResult reslistener){
		//注册运动事件结果监听器
		this.reslistener = reslistener;
		//初始化传感器
		mSensorManager = (SensorManager)mContext.getSystemService(Context.SENSOR_SERVICE);		
		//启动加速传感器
		mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_NORMAL);
		lastX = 0;
		lastY = 0;
		lastZ = 0;
		Log.i(TAG, "启动加速传感器");
	}
	
	public void stopSensorAccelerometer(){
		if(mSensorManager == null){
			return;
		}
		//停止加速传感器
		mSensorManager.unregisterListener(this, mAccelerometer);
		Log.i(TAG, "停止加速传感器");
	}
	
	@SuppressLint("NewApi") 
	@Override
	public void onSensorChanged(SensorEvent event) {
		if(reslistener == null || event.sensor == null){
			return;
		}
		//event.sensor.getStringType().equals(Sensor.STRING_TYPE_ACCELEROMETER)
		//部分机型报NoSuchMethod异常
		if(event.sensor.getType() == Sensor.TYPE_ACCELEROMETER){
			//获得当前运动坐标值，时间戳
			int x = (int)event.values[0];
			int y = (int)event.values[1];
			int z = (int)event.values[2];
			long stamp = Calendar.getInstance().getTimeInMillis();
			//根据坐标变化值，计算加速度大小
			int px = Math.abs(lastX-x); 
			int py = Math.abs(lastY-y);
			int pz = Math.abs(lastZ-z);
			double accelerometer = Math.sqrt(px*px+py*py+pz*pz);
//			Log.i(TAG, "px="+px+"；py="+py+"；pz="+pz+"；accelerometer="+accelerometer);
			//当手机倾斜20度左右或移动4cm时accelerometer值约为1.4
			if(accelerometer > 1.4){
				isFocused = false;
				reslistener.onMoving(x,y,z);
				status = STATUS_MOVING;
			}else{
				//记录静止起止时间，如果静止时间超过800ms，则回调onStopped实现对焦
				if(status == STATUS_MOVING){
					laststamp = stamp;
				}				
				if((stamp - laststamp> STATIC_DELAY_TIME) && !isFocused){
					isFocused  = true;
					reslistener.onStopped();
				}
				status = STATUS_STOP;
			}
			//缓存当前坐标，用于下次计算
			lastX = x;
			lastY = y;
			lastZ = z;
		}
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {

	}
}
