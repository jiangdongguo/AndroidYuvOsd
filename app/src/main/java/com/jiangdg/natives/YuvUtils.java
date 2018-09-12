package com.jiangdg.natives;

/**
 * Created by jiangdongguo on 2018/8/18.
 */

public class YuvUtils {
    public static native int nativeNV21ToYUV420sp(byte[] data,int width, int height);
    public static native int nativeNV21ToYUV420p(byte[] data,int width, int height);
    public static native int nativeYV12ToNV21(byte[] data,int width, int height);

    // 后置旋转：90、180、270
    public native static void nativeRotateNV21(byte[] src,byte[] dest,int width, int height,int rotateDegree);

    // 前置旋转：270，180
    public  static native void nativeRotateNV21Flip(byte[] src,byte[] dest,int width, int height,int rotateDegree);

    static{
        System.loadLibrary("YuvOsd");
    }
}
