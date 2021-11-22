#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <cstdint>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <Windows.h>
#include <iostream>
#include <csignal>

//本文件定義了符合ICMP協議還有IP協議的一些數據結構類型
//以及一些Ping程序處理的函數
/*                       IP報文格式
0            8           16    19                   31
+------------+------------+-------------------------+
| ver + hlen |  服務類型  |          總長度         |
+------------+------------+----+--------------------+
|           標識位        |flag|    分片偏移(13位)  |
+------------+------------+----+--------------------+
|   生存時間 | 高層協議號 |        首部校驗和       |
+------------+------------+-------------------------+
|                   源 IP 地址                      |
+---------------------------------------------------+
|                  目的 IP 地址                     |
+---------------------------------------------------+
*/
//IP頭
struct IPhead
{
	//這裡使用了C語言的位域，也就是說像version變量它的大小在內存中是佔4bit，而不是8bit
	uint8_t		version : 4; //IP協議版本
	uint8_t		headLength : 4;//首部長度
	uint8_t		serverce;//區分服務
	uint16_t	totalLength;//總長度
	uint16_t	flagbit;//標識
	uint16_t	flag : 3;//標誌
	uint16_t	fragmentOffset : 13;//片偏移
	char		timetoLive;//生存時間（跳數）
	uint8_t		protocol;//使用協議
	uint16_t	headcheckSum;//首部校驗和
	uint32_t	srcIPadd;//源IP
	uint32_t	dstIPadd;//目的IP
	//可選項和填充我就不定義了
};
/*
ICMP 請求報文
+--------+--------+---------------+
|  類型  |  代碼   |      描述    |
+--------+--------+---------------+
|    8   |    0   |    回顯請求   |
+--------+--------+---------------+
|   10   |    0   |   路由器請求  |
+--------+--------+---------------+
|   13   |    0   |   時間戳請求  |
+--------+--------+---------------+
|   15   |    0   | 信息請求(廢棄)|
+--------+--------+---------------+
|   17   |    0   | 地址掩碼請求  |
+--------+--------+---------------+
ICMP 應答報文
+--------+--------+---------------+
|  類型  |  代碼  |      描述     |
+--------+--------+---------------+
|    0   |    0   |    回顯回答   |
+--------+--------+---------------+
|    9   |    0   |   路由器回答  |
+--------+--------+---------------+
|   14   |    0   |   時間戳回答  |
+--------+--------+---------------+
|   16   |    0   | 信息回答(廢棄)|
+--------+--------+---------------+
|   18   |    0   | 地址掩碼回答  |
+--------+--------+---------------+
ICMP協議請求/回答報文格式
0        8       16                32
+--------+--------+-----------------+
|  類型   |  代碼   |     校驗和    |
+--------+--------+-----------------+
|      標識符      |      序號      |
+-----------------+-----------------+
|             回顯數據              |
+-----------------+-----------------+
*/
//ICMP協議我只定義了請求/回答功能的報文格式。因為不同的類型和代碼報文的內容組成不一樣,ICMP分請求報文，回答報文，差錯報文。差錯報文我沒寫出來，因為用不著
//ICMP內容
//ICMP頭
struct ICMPhead
{
	uint8_t type;//類型
	uint8_t code;//代碼
	uint16_t checkSum;//校驗和
	uint16_t ident;//進程標識符
	uint16_t seqNum;//序號
};
//ICMP回顯請求報文(發送用)
struct ICMPrecvReq
{
	ICMPhead icmphead;//頭部
	uint32_t timeStamp;//時間戳
	char	 data[32];//數據
};
//ICMP應答報文(接收用)
struct ICMPansReply
{
	IPhead iphead;//IP頭
	ICMPrecvReq icmpanswer;//ICMP報文
	char data[1024];//應答報文攜帶的數據緩衝區
};
//計算校驗和，參數1：協議頭的指針，需要轉換成空指針。參數2：計算的協議類型。返回校驗和
uint16_t getCheckSum(void* protocol, char* type);
//ping程序，參數1：為目的IP地址。返回是否發送成功.參數2：只能寫-t,或者不寫
bool ping(const char* dstIPaddr, const char* param = '\0');
//發送ICMP請求回答報文，參數1：套接字.參數2：目的地址.參數3：第num次發送
bool sendICMPReq(SOCKET &mysocket, sockaddr_in &dstAddr, unsigned int num);
//讀取ICMP應答報文，參數1：套接字.參數2：接收方地址.參數3：跳數
uint32_t readICMPanswer(SOCKET &mysocket, sockaddr_in &srcAddr, char &TTL);
//等待socket是否由數據需要讀取
int waitForSocket(SOCKET &mysocket);
//執行一次ping操作,參數1：套接字.參數2：源地址.參數3：目的地址序號ping報文序號
void doPing(SOCKET &mysocket, sockaddr_in &srcAddr, sockaddr_in &dstAddr, int num);

char* isParamEmpty(char *buffer, char *param);
//捕獲終止信號函數
void get_ctrl_stop(int signal);