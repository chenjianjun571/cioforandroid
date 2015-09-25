package com.example.chenjianjun.example;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.jsbn.protoc.protocol;

import org.cio.CIO;
import org.cio.CIODelegate;

import java.util.logging.LogRecord;


public class MainActivity extends Activity {

    private int fd = -1;
    private int connect_flg = -1;
    private MyDelegate myDelegate = new MyDelegate();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        final Button btnConnect = (Button)findViewById(R.id.btnConnect);
        btnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (fd > 0) {
                    Log.d("chenjianjun", "不要重复点击");
                    return;
                }

                CIO.getInstance().InitWithDelegate(myDelegate);
                fd = CIO.getInstance().OpenIOChannel();
                if (fd < 0) {
                    Log.e("chenjianjun", "channle error");
                    return;
                }

                // 测试
                CIO.getInstance().ConnectServiceWhitFD(fd, "192.168.1.4", 5858);
            }
        });

        final Button btnSend = (Button)findViewById(R.id.btnSend);
        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (fd < 0) {
                    return;
                }

                protocol.BSSNetProtocol.Builder builder=protocol.BSSNetProtocol.newBuilder();
                builder.setType(protocol.MSG.Heart_Beat);

                protocol.BSSNetProtocol info=builder.build();
                byte[] result=info.toByteArray();
                int len=info.getSerializedSize();

                CIO.getInstance().SendDataWithFD(fd, result, len);
            }
        });

        handler.postDelayed(runnable, TIME); //每隔1s执行
    }

    @Override
    protected void onDestroy() {
        CIO.getInstance().CloseIOChannelWithFD(fd);
        fd = -1;
        connect_flg = -1;
        CIO.getInstance().Terminat();
        super.onDestroy();
    }


    public class MyDelegate implements CIODelegate {

        @Override
        public void RecvTCPDataWithFD(int fd, byte[] data, int len) {

            // 1.不要在此函数里面调用CIO的接口，否则会造成底层死锁
            // 2.不要在此函数里面做一些阻塞性的操作，否则会阻塞底层通信线程
            // 3.建议通过sendMessage的方式把数据传递到其他线程去处理
            Log.e("chenjianjun", "收到数据");

        }

        @Override
        public void StatusReportWithFD(int fd, int status) {
            // 1.不要在此函数里面调用CIO的接口，否则会造成底层死锁
            // 2.不要在此函数里面做一些阻塞性的操作，否则会阻塞底层通信线程
            // 3.建议通过sendMessage的方式把数据传递到其他线程去处理
            Message message = new Message();
            message.what = status;
            myHandler.sendMessage(message);
        }

    }

    protected Handler myHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case 0:
                {
                    Log.e("chenjianjun", "连接建立");
                    connect_flg = 0;

                    break;
                }
                case 4:
                {
                    connect_flg = -1;
                    Log.e("chenjianjun", "连接关闭");
                    CIO.getInstance().CloseIOChannelWithFD(fd);
                    fd = -1;
                    break;
                }
            }
            super.handleMessage(msg);
        }
    };


    private int TIME = 1000;
    Handler handler = new Handler();
    Runnable runnable = new Runnable() {

        @Override
        public void run() {
            // handler自带方法实现定时器
            try {
                handler.postDelayed(this, TIME);

                if (fd < 0) return;
                if (connect_flg != 0) return;

                // 测试代码
                protocol.BSSNetProtocol.Builder builder=protocol.BSSNetProtocol.newBuilder();
                builder.setType(protocol.MSG.Heart_Beat);

                protocol.BSSNetProtocol info=builder.build();
                byte[] result=info.toByteArray();
                int len=info.getSerializedSize();

                for (int i = 0; i < 12; ++i) {
                    CIO.getInstance().SendDataWithFD(fd, result, len);
                }

            } catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                System.out.println("exception...");
            }
        }
    };
}

