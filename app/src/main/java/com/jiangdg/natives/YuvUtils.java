package com.jiangdg.natives;

/** 时间水印native方法
 *
 * Created by jiangdongguo on 2017/9/28.
 */

public class YuvUtils {

    public static native void nativeNV21ToYV12(byte[] src,int width, int height,byte[] dst);
    public static native void nativeYV12ToNV21(byte[] src,int width, int height,byte[] dst);

    static{
        System.loadLibrary("YuvOsd");
    }
}
