# pi-ds4-dongle

ds4蓝牙逆向工程：

[DS4-BT](https://www.psdevwiki.com/ps4/index.php?title=DS4-BT&oldid=279252)

ds4drv关于蓝牙音频传输的讨论：

[chrippa/ds4drv](https://github.com/chrippa/ds4drv/issues/76)

## ds4映射NintendoSwitch键位

较新的linux驱动中已经集成了ds4驱动，源码位置：

> linux/drivers/hid/hid-sony.c

这里直接读取了linux准备好的`/dev/input/js*`设备，或者按照逆向工程读取`report_id 11`的内容。

## ds4传输蓝牙音频

1. bluez与SBC编码
    
    按照逆向工程的描述，ds4接收的是SBC编码的蓝牙音频，作为linux默认的蓝牙协议栈，bluez提供了[SBC编解码API](http://www.bluez.org/sbc-13/)。
    
    pi0的Debian系统可以直接apt安装
    
    '''
    # apt install libsbc-dev
    '''    
    
    请阅读bluez提供的例子，理解一下SBC编码原理与使用方法。
    
    对于ds4来说，SBC音频头如下：
    
    > 9c 75 19 24
    
    >      0x9c = 156 syncword (always set to 156)
    >      1 byte - sf bl cm a s (msb..lsb)
    >        * frequency:
    >            00-16000
    >            01-32000
    >            10-44100
    >            11-48000
    >        * blocks:
    >            00-4
    >            01-8
    >            10-12
    >            11-16
    >        * channels:
    >            00-MONO
    >            01-DUAL_CHANNEL 
    >            10-STEREO 
    >            11-JOINT_STEREO
    >        * allocation method:
    >            0-loudnes
    >            1-SNR
    >        * subbands:
    >            0-4
    >            1-8
    >      1 byte - bitpool
    >            ...
    
    在例子中选择的SBC编码是`32kHz, 16blocks, 2channels, loudness, 8subbands`,使用`sbc_encode(...)`时会生成对应音频头.
    
2. 定时发送

    用于使用的是hidraw传输数据，我们要自行规划每次数据的传输间隔，保证在上一包音频数据消耗完时有下一包音频数据补充上。
    
    对于`32kHz 8subbands`SBC编码后的数据来说，每一帧大小为：
    
    > frame_size = 4 + (4 * subbands * channels) / 8 +(blocks * channels * bitpool) / 8
    
    = `4 + 4*8*2/8 + 16*2*25/8 = 112bytes`
    
    每帧时间为：
    
    > frame_time = subbands * blocks / sample_rate 
    
    = `8 * 16 /32 = 4ms`
    
    report_14/15每包有2帧数据，耗时8ms；report_17/18/19每包有4帧数据，耗时16ms。理论上，我们只要每8ms或16ms发送一包数据即可。
    
3. ALSA与PCM

    在较新的linux内核中使用ALSA作为默认的音频框架，我们需要从ALSA读取PCM数据再交由SBC编码，最后通过蓝牙发送ds4。
    
    这里使用libasound2读写声卡设备：
    
    ```
    # apt install libasound2-dev
    ```
    
    需要注意的是，通过`snd_pcm_hw_params_get_period_size`获取的PCM的帧数量，需要换算成字节数：
    
    >   frame_size = sample_bits * channels / 8  
    >   period_size = period_frames * frame_size  
    