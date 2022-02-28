### README

**519021910647 周昱宏**

**email: sonoso@sjtu.edu.cn**

####	 1、核心思路：基于ACK、无NAK的GBN协议，且发送方及接收方用状态机的思想设计。

#### 2、具体设计。

①发送方参数及含义：

```c++
#define WINDOW_SIZE 10   //滑动窗口的大小，我定为10
#define TIME_OUT 1		//发送方的计时器超时时间
int base,nextseqnum;    //base：基序号  nextseqnum:要发送的下一个序号
std::queue<packet> buffer;  //用于窗口满了的时候缓存上方传过来的message
packet* window = nullptr;	//滑动窗口
```

②发送方包的设计以及相关函数

- 发送方包的设计

  packet的结构为：

  ```c++
  int nextseqnum | bool isEnd | int length of data | char[] data | char checksum
  ```

  由于一个message需要拆分成多个packet，因此使用isEnd来表明一个包是否为message的最后一个包。

  checksum 采用8bit的checksum

- 相关函数

  ```c++
  packet make_pkt1(char* data, bool isEnd = true,int length = 118);
  void make_pkt2(int seqnum, packet& pac);
  ```
  
  函数make_pkt1的主要作用是把message转化成多个包存入到buffer中，由于在buffer中的包的seqnum需要发送的时候才能确定，因此增加了make_pkt2函数来为包添加seqnum项。



③	发送方对上层和下层信息的处理。

- 上层信息处理

  当收到上层的message时，并不是直接把message发送出去，而是调用make_pkt1函数，把message转成多个packet，之后把packet加入到buffer中。接着从buffer中取出尽可能多的packet发送出去，这个过程窗口会向前滑动，同时计时器会启动。

- 下层信息处理

  当收到下层的packet时，如果收到正常的ACK，则窗口会继续向前滑动，否者窗口卡死直到计时器到时调用相关函数。 注意Sender_FromLowerLayer函数里也加入了发包的逻辑，即窗口滑动后会发包。
  

④计时器超时

​	这时候会重发所有发送了但是还未确认的包。

⑤接收方参数及含义

```c++
int expectedseqnum;  //期望接收到的seqnum
packet sndpkt;		 //存放返回的包的变量
std::vector<packet> reciverBuffer;  //用于缓存收到的包来生成message
```

⑥接收方收到下层信息的处理

​	如果收到的包一切正常，是预期的seqnum，则会把包先加入到reciverBuffer中，并通过包中的isEnd参数来判断是否需要把reciverBuffer中的包拼接成message送到上层。同时返回ACK，生成新的snkpkt,expectedseqnum加1。

​	否则，会返回旧的snkpkt，造成发送方计时器超时。

⑦接收方发送的包的设计

```c++
int expectedseqnum | bool ACK|char checksum
```

#### 3、遇到的问题以及解决方法

- 多线程的使用以及受阻

  最开始我的实现是，发送方额外开启一个线程，只要buffer不为空，就从buffer中取出packet发送出去。按照这个思路写完的代码能通过最基本的测试，即没有丢包、没有乱序以及包受损的情况。进行分析后发现，这个项目的框架不能采用多线程实现，rdt_sim的实现，包括计时器，发送方，接收方都是在同个主线程中的。

- 结束时buffer中的信息还没有发完

  当调用发送方的Sender_Final函数时，发送方的buffer中的packet没有发完，这是由于上层发送message的速度大于发包的速度导致的。分析原因后，我在Sender_FromLowerLayer中也加入了发包的逻辑，即窗口一滑动就确保发包，防止buffer中包的堆积。

- checksum未检查出包的受损。

  我使用的checksum采用的是8bit的checksum，这种情况也容易理解，当包中的两个byte调换位置之后，包是受损的但是checksum检测不出来。这就导致，发送方解析出来的acknum很离谱。为了避免这一问题，在解析出acknum之后增加了一个判断，即acknum需要落在[base - WINDOW_SIZE, base + WINDOW_SIZE]之中，否则当作包corrupt处理。

