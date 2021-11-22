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

//�����w�q�F�ŦXICMP��ĳ�٦�IP��ĳ���@�Ǽƾڵ��c����
//�H�Τ@��Ping�{�ǳB�z�����
/*                       IP����榡
0            8           16    19                   31
+------------+------------+-------------------------+
| ver + hlen |  �A������  |          �`����         |
+------------+------------+----+--------------------+
|           ���Ѧ�        |flag|    ��������(13��)  |
+------------+------------+----+--------------------+
|   �ͦs�ɶ� | ���h��ĳ�� |        ��������M       |
+------------+------------+-------------------------+
|                   �� IP �a�}                      |
+---------------------------------------------------+
|                  �ت� IP �a�}                     |
+---------------------------------------------------+
*/
//IP�Y
struct IPhead
{
	//�o�̨ϥΤFC�y�������A�]�N�O����version�ܶq�����j�p�b���s���O��4bit�A�Ӥ��O8bit
	uint8_t		version : 4; //IP��ĳ����
	uint8_t		headLength : 4;//��������
	uint8_t		serverce;//�Ϥ��A��
	uint16_t	totalLength;//�`����
	uint16_t	flagbit;//����
	uint16_t	flag : 3;//�лx
	uint16_t	fragmentOffset : 13;//������
	char		timetoLive;//�ͦs�ɶ��]���ơ^
	uint8_t		protocol;//�ϥΨ�ĳ
	uint16_t	headcheckSum;//��������M
	uint32_t	srcIPadd;//��IP
	uint32_t	dstIPadd;//�ت�IP
	//�i�ﶵ�M��R�ڴN���w�q�F
};
/*
ICMP �ШD����
+--------+--------+---------------+
|  ����  |  �N�X   |      �y�z    |
+--------+--------+---------------+
|    8   |    0   |    �^��ШD   |
+--------+--------+---------------+
|   10   |    0   |   ���Ѿ��ШD  |
+--------+--------+---------------+
|   13   |    0   |   �ɶ��W�ШD  |
+--------+--------+---------------+
|   15   |    0   | �H���ШD(�o��)|
+--------+--------+---------------+
|   17   |    0   | �a�}���X�ШD  |
+--------+--------+---------------+
ICMP ��������
+--------+--------+---------------+
|  ����  |  �N�X  |      �y�z     |
+--------+--------+---------------+
|    0   |    0   |    �^��^��   |
+--------+--------+---------------+
|    9   |    0   |   ���Ѿ��^��  |
+--------+--------+---------------+
|   14   |    0   |   �ɶ��W�^��  |
+--------+--------+---------------+
|   16   |    0   | �H���^��(�o��)|
+--------+--------+---------------+
|   18   |    0   | �a�}���X�^��  |
+--------+--------+---------------+
ICMP��ĳ�ШD/�^������榡
0        8       16                32
+--------+--------+-----------------+
|  ����   |  �N�X   |     ����M    |
+--------+--------+-----------------+
|      ���Ѳ�      |      �Ǹ�      |
+-----------------+-----------------+
|             �^��ƾ�              |
+-----------------+-----------------+
*/
//ICMP��ĳ�ڥu�w�q�F�ШD/�^���\�઺����榡�C�]�����P�������M�N�X���媺���e�զ����@��,ICMP���ШD����A�^������A�t������C�t������ڨS�g�X�ӡA�]���Τ���
//ICMP���e
//ICMP�Y
struct ICMPhead
{
	uint8_t type;//����
	uint8_t code;//�N�X
	uint16_t checkSum;//����M
	uint16_t ident;//�i�{���Ѳ�
	uint16_t seqNum;//�Ǹ�
};
//ICMP�^��ШD����(�o�e��)
struct ICMPrecvReq
{
	ICMPhead icmphead;//�Y��
	uint32_t timeStamp;//�ɶ��W
	char	 data[32];//�ƾ�
};
//ICMP��������(������)
struct ICMPansReply
{
	IPhead iphead;//IP�Y
	ICMPrecvReq icmpanswer;//ICMP����
	char data[1024];//����������a���ƾڽw�İ�
};
//�p�����M�A�Ѽ�1�G��ĳ�Y�����w�A�ݭn�ഫ���ū��w�C�Ѽ�2�G�p�⪺��ĳ�����C��^����M
uint16_t getCheckSum(void* protocol, char* type);
//ping�{�ǡA�Ѽ�1�G���ت�IP�a�}�C��^�O�_�o�e���\.�Ѽ�2�G�u��g-t,�Ϊ̤��g
bool ping(const char* dstIPaddr, const char* param = '\0');
//�o�eICMP�ШD�^������A�Ѽ�1�G�M���r.�Ѽ�2�G�ت��a�}.�Ѽ�3�G��num���o�e
bool sendICMPReq(SOCKET &mysocket, sockaddr_in &dstAddr, unsigned int num);
//Ū��ICMP��������A�Ѽ�1�G�M���r.�Ѽ�2�G������a�}.�Ѽ�3�G����
uint32_t readICMPanswer(SOCKET &mysocket, sockaddr_in &srcAddr, char &TTL);
//����socket�O�_�ѼƾڻݭnŪ��
int waitForSocket(SOCKET &mysocket);
//����@��ping�ާ@,�Ѽ�1�G�M���r.�Ѽ�2�G���a�}.�Ѽ�3�G�ت��a�}�Ǹ�ping����Ǹ�
void doPing(SOCKET &mysocket, sockaddr_in &srcAddr, sockaddr_in &dstAddr, int num);

char* isParamEmpty(char *buffer, char *param);
//����פ�H�����
void get_ctrl_stop(int signal);