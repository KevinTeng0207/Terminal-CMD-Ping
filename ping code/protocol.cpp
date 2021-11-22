#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "protocol.h"

char ICMP[] = "ICMP";
static volatile int keepping = 1;
//捕獲終止信號函數,專門處理無限ping時的操作
void get_ctrl_stop(int signal)
{
	if (signal == SIGINT)
	{
		keepping = 0;
	}
}


uint16_t getCheckSum(void * protocol, char * type)
{
	uint32_t checkSum = 0;
	uint16_t* word = (uint16_t*)protocol;
	uint32_t size = 0;
	if (!strcmp(type, "ICMP")) {//計算有多少個字節
		size = (sizeof(ICMPrecvReq));
	}
	while (size > 1)//用32位變量來存是因為要存儲16位數相加可能發生的溢出情況，將溢出的最高位最後加到16位的最低位上
	{
		checkSum += *word++;
		size -= 2;
	}
	if (size == 1)
	{
		checkSum += *(uint8_t*)word;
	}
	//二進制反碼求和運算，先取反在相加和先相加在取反的結果是一樣的，所以先全部相加在取反
	//計算加上溢出後的結果
	while (checkSum >> 16) checkSum = (checkSum >> 16) + (checkSum & 0xffff);
	//取反
	return (~checkSum);
}

bool ping(const char * dstIPaddr, const char* param)
{
	SOCKET		rawSocket;//socket
	sockaddr_in srcAddr;//socket源地址
	sockaddr_in dstAddr;//socket目的地址
	int			Ret;//捕獲狀態值
	char		TTL = '0';//跳數

	//生成一個套接字
	//TCP/IP協議族,RAW模式，ICMP協議
	//RAW創建的是一個原始套接字，最低可以訪問到數據鏈路層的數據，也就是說在網絡層的IP頭的數據也可以拿到了。
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	//設置目標IP地址
	dstAddr.sin_addr.S_un.S_addr = inet_addr(dstIPaddr);
	//端口
	dstAddr.sin_port = htons(0);
	//協議族
	dstAddr.sin_family = AF_INET;

	//提示信息
	std::cout << "正在 Ping " << inet_ntoa(dstAddr.sin_addr) << " 具有 "
		<< sizeof(ICMPrecvReq::data) << " 字節的數據:" << std::endl;
	std::cout << param << std::endl;
	keepping = 1;
	if (!strcmp(param, "-t"))
	{
		uint32_t i = 0;

		while (keepping)
		{
			//無限執行ping操作
			doPing(rawSocket, srcAddr, dstAddr, i);
			//睡眠1ms
			Sleep(1000);
			i++;
		}
		keepping = 1;
	}
	else if (!strcmp(param, "no_param"))
	{	//執行4次ping
		for (int i = 0; i < 4; i++)
		{
			doPing(rawSocket, srcAddr, dstAddr, i);
			Sleep(1000);
		}
	}

	Ret = closesocket(rawSocket);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket關閉時發生錯誤:" << WSAGetLastError() << std::endl;
	}

	return true;
}

bool sendICMPReq(SOCKET &mysocket, sockaddr_in &dstAddr, unsigned int num)
{
	//創建ICMP請求回顯報文
	//設置回顯請求
	ICMPrecvReq myIcmp;//ICMP請求報文
	myIcmp.icmphead.type = 8;
	myIcmp.icmphead.code = 0;
	//設置初始檢驗和為0
	myIcmp.icmphead.checkSum = 0;
	//獲得一個進程標識，這樣就能夠根據校驗來識別接收到的是不是這個進程發出去的ICMP報文
	myIcmp.icmphead.ident = (uint16_t)GetCurrentProcessId();
	//設置當前序號為0
	myIcmp.icmphead.seqNum = ++num;
	//保存發送時間
	myIcmp.timeStamp = GetTickCount();
	//計算並且保存校驗和
	myIcmp.icmphead.checkSum = getCheckSum((void*)&myIcmp, ICMP);
	//發送報文
	int Ret = sendto(mysocket, (char*)&myIcmp, sizeof(ICMPrecvReq), 0, (sockaddr*)&dstAddr, sizeof(sockaddr_in));

	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket 發送錯誤:" << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

uint32_t readICMPanswer(SOCKET &mysocket, sockaddr_in &srcAddr, char &TTL)
{
	ICMPansReply icmpReply;//接收應答報文
	int addrLen = sizeof(sockaddr_in);
	//接收應答
	int Ret = recvfrom(mysocket, (char*)&icmpReply, sizeof(ICMPansReply), 0, (sockaddr*)&srcAddr, &addrLen);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket 接收錯誤:" << WSAGetLastError() << std::endl;
		return false;
	}
	//讀取校驗並重新計算對比
	uint16_t checkSum = icmpReply.icmpanswer.icmphead.checkSum;
	//因為發出去的時候計算的校驗和是0
	icmpReply.icmpanswer.icmphead.checkSum = 0;
	//重新計算
	if (checkSum == getCheckSum((void*)&icmpReply.icmpanswer, ICMP)) {
		//獲取TTL值
		TTL = icmpReply.iphead.timetoLive;
		return icmpReply.icmpanswer.timeStamp;
	}

	return -1;
}

int waitForSocket(SOCKET &mysocket)
{
	//5S 等待套接字是否由數據
	timeval timeOut;
	fd_set	readfd;
	readfd.fd_count = 1;
	readfd.fd_array[0] = mysocket;
	timeOut.tv_sec = 5;
	timeOut.tv_usec = 0;
	return (select(1, &readfd, NULL, NULL, &timeOut));
}

void doPing(SOCKET & mysocket, sockaddr_in & srcAddr, sockaddr_in & dstAddr, int num)
{
	uint32_t	timeSent;//發送時的時間
	uint32_t	timeElapsed;//延遲時間
	char		TTL;//跳數
	//發送ICMP回顯請求
	sendICMPReq(mysocket, dstAddr, num);
	//等待數據
	int Ret = waitForSocket(mysocket);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket發生錯誤:" << WSAGetLastError() << std::endl;
		return;
	}
	if (!Ret)
	{
		std::cout << "請求超時:" << std::endl;
		return;
	}
	timeSent = readICMPanswer(mysocket, srcAddr, TTL);
	if (timeSent != -1) {
		timeElapsed = GetTickCount() - timeSent;
		//輸出信息，注意TTL值是ASCII碼，要進行轉換
		std::cout << "來自 " << inet_ntoa(srcAddr.sin_addr) << " 的回复: 字節= " << sizeof(ICMPrecvReq::data) << " 時間= " << timeElapsed << "ms TTL= " << fabs((int)TTL) << std::endl;
	}
	else {
		std::cout << "請求超時" << std::endl;
	}

}

char *isParamEmpty(char *buffer, char *param)
{
	char *temp = NULL;
	temp = buffer;
	while (*temp != '\0')
	{
		if (*temp == ' ')
		{
			*temp = '\0';
			param = ++temp;
		}
		temp++;
	}
	return param;
}