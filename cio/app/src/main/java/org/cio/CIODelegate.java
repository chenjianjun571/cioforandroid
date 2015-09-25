package org.cio;

import android.util.Log;

/**
 * Created by chenjianjun on 15/9/24.
 */
public interface CIODelegate {

    /**
     * 不要在此函数里面做业务,不要阻塞底层线程,需要通过handle的方式到其他线程去处理
     * @param fd
     * @param data
     * @param len
     * @return
     */

    public void RecvTCPDataWithFD(int fd, byte[] data, int len);

    /**
     * 不要在此函数里面做业务,不要阻塞底层线程,需要通过handle的方式到其他线程去处理
     * @param fd
     * @param status
     */
    public void StatusReportWithFD(int fd, int status);

}
