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

    /** 添加水印
     *  src 图像数据，YUV420采样格式均可(只用到了Y分量)
     *  width 图像宽度
     *  height 图像高度
     *  osdStr 水印内容
     *  isHorizontalTake 图像方向，水平或竖直
     * */
    public static native void addYuvOsd(byte[] src,int width,int height,boolean isHorizontalTake,String osdStr,int startX,int startY);

    static{
        System.loadLibrary("YuvOsd");
    }
}
