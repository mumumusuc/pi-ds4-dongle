# pi-ds4-dongle

ds4蓝牙逆向工程：[DS4-BT](https://www.psdevwiki.com/ps4/index.php?title=DS4-BT&oldid=279252)

ds4drv关于蓝牙音频传输的讨论：[chrippa/ds4drv](https://github.com/chrippa/ds4drv/issues/76)

## ds4映射NintendoSwitch键位

较新的linux驱动中已经集成了ds4驱动，源码位置：

> linux/drivers/hid/hid-sony.c

这里直接读取了linux准备好的`/dev/input/js*`设备，或者按照逆向工程读取`report_id 11`的内容。

## ds4的震动、LED与其他功能

> TBD

## ds4传输蓝牙音频

1. bluez与SBC编码
    
    按照逆向工程的描述，ds4接收的是SBC编码的蓝牙音频，作为linux默认的蓝牙协议栈，bluez提供了[SBC编解码API](http://www.bluez.org/sbc-13/)。
    
    pi0的Debian系统可以直接apt安装
    
    ```
    # apt install libsbc-dev
    ```    
    
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
    
    对于"32kHz 8subbands"SBC编码后的数据来说，每一帧大小为：
    
    > frame_size = 4 + (4 * subbands * channels) / 8 +(blocks * channels * bitpool) / 8
    
    = `4 + 4*8*2/8 + 16*2*25/8 = 112bytes`
    
    每帧时间为：
    
    > frame_time = subbands * blocks / sample_rate 
    
    = `8*16/32 = 4ms`
    
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
    
    对于SND_PCM_FORMAT_S16_BE双声道来说，每一帧大小为4bytes，SBC每帧大小为`sbc_get_codesize(...)=512bytes`,编码后大小为`sbc_encode(...)=112bytes`,与ds4逆向工程提供的数据一致。
    
## Sample

0. 设置pi0的UAC
    
    pi0的Debian系统内核使用的是UAC2.0，如果需要连接Nintendo Switch作为USB音箱的话，需要编译成UAC1.0，请参考我的另一个项目[pi-joystick](https://github.com/mumumusuc/pi-joystick)。

1. 设置pi0的audio source
    
    ```
    # pacmd list-sources
    
    > index: 1
      	name: <alsa_input.platform-20980000.usb.analog-stereo>
      	driver: <module-alsa-card.c>
      	flags: HARDWARE DECIBEL_VOLUME LATENCY DYNAMIC_LATENCY
      	state: IDLE
      	suspend cause: 
      	priority: 9009
      	volume: front-left: 65536 / 100% / 0.00 dB,   front-right: 65536 / 100% / 0.00 dB
      	        balance 0.00
      	base volume: 65536 / 100% / 0.00 dB
      	volume steps: 65537
      	muted: no
      	current latency: 3945.43 ms
      	max rewind: 0 KiB
      	sample spec: s16le 2ch 48000Hz
      	channel map: front-left,front-right
      	             Stereo
      	used by: 0
      	linked by: 0
      	configured latency: 341.33 ms; range is 0.50 .. 341.33 ms
      	card: 0 <alsa_card.platform-20980000.usb>
      	module: 6
      	properties:
      		alsa.resolution_bits = "16"
      		device.api = "alsa"
      		device.class = "sound"
      		alsa.class = "generic"
      		alsa.subclass = "generic-mix"
      		alsa.name = "UAC2 PCM"
      		alsa.id = "UAC2 PCM"
      		alsa.subdevice = "0"
      		alsa.subdevice_name = "subdevice #0"
      		alsa.device = "0"
      		alsa.card = "1"
      		alsa.card_name = "UAC2_Gadget"
      		alsa.long_card_name = "UAC2_Gadget 0"
      		device.bus_path = "platform-20980000.usb"
      		sysfs.path = "/devices/platform/soc/20980000.usb/gadget/sound/card1"
      		device.string = "hw:1"
      		device.buffering.buffer_size = "65536"
      		device.buffering.fragment_size = "4096"
      		device.access_mode = "mmap+timer"
      		device.profile.name = "analog-stereo"
      		device.profile.description = "Analog Stereo"
      		device.description = "UAC2_Gadget Analog Stereo"
      		module-udev-detect.discovered = "1"
      		device.icon_name = "audio-card"
      	ports:
      		analog-input: Analog Input (priority 10000, latency offset 0 usec, available: unknown)
      			properties:
      				
      	active port: <analog-input>
    ```
    
    看到pi0 gadget模拟的音频输入设备为`index: 1 , name: <alsa_input.platform-20980000.usb.analog-stereo>`，接下来把它设置为默认信源。
    
    ```
    # pacmd set-default-source 1
    ```

3. pi0蓝牙连接ds4

    *推荐编译安装最新的[bluez](http://www.bluez.org/download/)，较老的版本可能会出现一些莫名的错误。*
    
    ```
    # bluetoothctl -v
    > 5.9
    
    # bluetoothctl 
    [bluetooth]# power on
    [bluetooth]# agent on
    [bluetooth]# default-agent
    [bluetooth]# scan on
    Discovery started
    [CHG] Controller B8:27:**:**:**:** Discovering: yes
    [NEW] Device DC:0C:**:**:**:** Wireless Controller
    [bluetooth]# info DC:0C:**:**:**:** 
    Device DC:0C:**:**:**:**
    	Name: Wireless Controller
    	Alias: Wireless Controller
    	Class: 0x002508
    	Icon: input-gaming
    	Paired: yes
    	Trusted: yes
    	Blocked: no
    	Connected: no
    	LegacyPairing: no
    	UUID: Human Interface Device... (00001124-0000-1000-8000-00805f9b34fb)
    	UUID: PnP Information           (00001200-0000-1000-8000-00805f9b34fb)
    	Modalias: usb:v054Cp09CCd0100
    [bluetooth]# pair DC:0C:**:**:**:**
    Attempting to pair with DC:0C:**:**:**:**
    [CHG] Device DC:0C:**:**:**:** Connected: yes
    [CHG] Device DC:0C:**:**:**:** Modalias: usb:v054Cp09CCd0100
    [CHG] Device DC:0C:**:**:**:** Modalias: usb:v054Cp09CCd0100
    [CHG] Device DC:0C:**:**:**:** UUIDs has unsupported type
    [CHG] Device DC:0C:**:**:**:** Paired: yes
    Pairing successful
    [CHG] Device DC:0C:**:**:**:** Connected: no
    [CHG] Device DC:0C:**:**:**:** RSSI: -43
    [bluetooth]# yes
    [bluetooth]# connet DC:0C:**:**:**:**
    Attempting to connect to DC:0C:**:**:**:**
    [CHG] Device DC:0C:**:**:**:** Connected: yes
    Connection successful
    [CHG] Device DC:0C:**:**:**:** Modalias: usb:v054Cp09CCd0100
    [CHG] Device DC:0C:**:**:**:** Modalias: usb:v054Cp09CCd0100

    ```
    成功连接ds4后，ds4手柄会亮起蓝色氛围灯，pi0中新增hidraw设备。
    ```
    # ls -al /dev/hidraw*
    > crw------- 1 root root 247, 0 4月   6 12:54 /dev/hidraw0
    # chmod 666 /dev/hidraw0
    ```
    
    如果出现了什么问题，请考虑升级bluez。
    
4. ?

    > TBD