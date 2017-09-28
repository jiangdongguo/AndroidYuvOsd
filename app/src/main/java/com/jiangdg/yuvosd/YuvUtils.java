package com.jiangdg.yuvosd;

/** 时间水印native方法
 *
 * Created by jiangdongguo on 2017/9/28.
 */

public class YuvUtils {
    /** 叠加时间水印
     * @param src NV21原始数据
     * @param width 分辨率的宽
     * @param height 分辨率的高
     * @param dest 叠加结果
     * @param date yyyy-MM-dd HH:mm:ss格式时间字符串
     * @param colorFormat 编码器支持的颜色格式，主要有YUV420SemiPlannar和YUV420Plannar
     * @param isVerticalTake 横屏采集true，竖直采集为false，设置不正确时间水印会错乱
     */
    public native static void AddYuvOsd(byte[] src,int width,int height ,byte[] dest,String date,int colorFormat,boolean isVerticalTake);

    /**颜色格式转换(摄像头支持的格式->编码器支持的格式)
     * @param src nv21原始数据
     * @param width 分辨率的宽
     * @param height 分辨率的高
     * @param dest yuv420sp或yuv420p
     * @param colorFormat 编码器支持的颜色格式，如果转换不正确会花屏
     */
    public native static void transferColorFormat(byte[] src,int width,int height ,byte[] dest,int colorFormat);

    /**旋转后置摄像头采集的YUV420sp图像
     * @param src nv21原始数据
     * @param dest 旋转后的nv21数据
     * @param width 分辨率的宽
     * @param height 分辨率的高
     * @param rotateDegree 旋转的角度，90、180、270
     */
    public native static void YUV420spRotateOfBack(byte[] src,byte[] dest,int width, int height,int rotateDegree);

    /**旋转前置摄像头采集的YUV420sp图像
     * @param src nv21原始数据
     * @param dest 旋转后的nv21数据
     * @param width 分辨率的宽
     * @param height 分辨率的高
     * @param rotateDegree 旋转的角度，270，180(特殊摄像头)
     */
    public  static native void Yuv420spRotateOfFront(byte[] src,byte[] dest,int width, int height,int rotateDegree);

    /** YV12转NV21(Camera采集的YUV数据主要为NV21或YV12)
     * @param src YV12原始数据
     * @param dest NV21数据
     * @param width 分辨率的宽
     * @param height 分辨率的高
     */
    public static native void swYV12ToNV21(byte[] src,byte[] dest,int width, int height);

    static{
        System.loadLibrary("YuvOsd");
    }
}
