package org.cio;

/**
 * Created by chenjianjun on 15/9/24.
 */
public class CIO {

    static {
        System.loadLibrary("cio");
    }

    private static CIO ourInstance = new CIO();

    public static CIO getInstance() {
        return ourInstance;
    }

    private CIO() {
    }

    /**
     *	@brief	初期化
     *
     *	@param 	ob 接口句柄，必须使用org.cio包里面提供的CIODelegate类或者派生类
     *
     *	@return	成功or失败
     */
    public native boolean InitWithDelegate(CIODelegate ob);

    /**
     *	@brief	退出
     *
     *	@return	成功or失败
     */
    public native void Terminat();

    /**
     *	@brief	打开一个通道
     *
     *	@return	返回一个句柄，后续的操作都是基于这个句柄，失败返回-1
     */
    public native int OpenIOChannel();

    /**
     *	@brief	通过句柄连接服务器
     *
     *	@param 	fd 	调用OpenIOChannel返回的句柄
     *	@param 	host_name 	服务器域名 www.jsbn.com
     *	@param 	host_port 	端口
     *
     *	@return	YES 成功 NO 失败
     */
    public native boolean ConnectServiceWhitFD(int fd, String host_name, int host_port);

    /**
     *	@brief	发送数据
     *
     *	@param 	fd 	调用OpenIOChannel返回的句柄
     *	@param 	data 	数据
     *  @param 	len 	数据长度
     *
     *	@return	0成功，-1失败
     */
    public native int SendDataWithFD(int fd, byte[] data, int len);

    /**
     *	@brief	关闭连接
     *
     *	@param 	fd 	调用OpenIOChannel返回的句柄
     *
     *	@return
     */
    public native void CloseIOChannelWithFD(int fd);

}
