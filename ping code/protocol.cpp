#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "protocol.h"

char ICMP[] = "ICMP";
static volatile int keepping = 1;
//����פ�H�����,�M���B�z�L��ping�ɪ��ާ@
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
	if (!strcmp(type, "ICMP")) {//�p�⦳�h�֭Ӧr�`
		size = (sizeof(ICMPrecvReq));
	}
	while (size > 1)//��32���ܶq�Ӧs�O�]���n�s�x16��Ƭۥ[�i��o�ͪ����X���p�A�N���X���̰���̫�[��16�쪺�̧C��W
	{
		checkSum += *word++;
		size -= 2;
	}
	if (size == 1)
	{
		checkSum += *(uint8_t*)word;
	}
	//�G�i��ϽX�D�M�B��A�����Ϧb�ۥ[�M���ۥ[�b���Ϫ����G�O�@�˪��A�ҥH�������ۥ[�b����
	//�p��[�W���X�᪺���G
	while (checkSum >> 16) checkSum = (checkSum >> 16) + (checkSum & 0xffff);
	//����
	return (~checkSum);
}

bool ping(const char * dstIPaddr, const char* param)
{
	SOCKET		rawSocket;//socket
	sockaddr_in srcAddr;//socket���a�}
	sockaddr_in dstAddr;//socket�ت��a�}
	int			Ret;//���򪬺A��
	char		TTL = '0';//����

	//�ͦ��@�ӮM���r
	//TCP/IP��ĳ��,RAW�Ҧ��AICMP��ĳ
	//RAW�Ыت��O�@�ӭ�l�M���r�A�̧C�i�H�X�ݨ�ƾ�����h���ƾڡA�]�N�O���b�����h��IP�Y���ƾڤ]�i�H����F�C
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	//�]�m�ؼ�IP�a�}
	dstAddr.sin_addr.S_un.S_addr = inet_addr(dstIPaddr);
	//�ݤf
	dstAddr.sin_port = htons(0);
	//��ĳ��
	dstAddr.sin_family = AF_INET;

	//���ܫH��
	std::cout << "���b Ping " << inet_ntoa(dstAddr.sin_addr) << " �㦳 "
		<< sizeof(ICMPrecvReq::data) << " �r�`���ƾ�:" << std::endl;
	std::cout << param << std::endl;
	keepping = 1;
	if (!strcmp(param, "-t"))
	{
		uint32_t i = 0;

		while (keepping)
		{
			//�L������ping�ާ@
			doPing(rawSocket, srcAddr, dstAddr, i);
			//�ίv1ms
			Sleep(1000);
			i++;
		}
		keepping = 1;
	}
	else if (!strcmp(param, "no_param"))
	{	//����4��ping
		for (int i = 0; i < 4; i++)
		{
			doPing(rawSocket, srcAddr, dstAddr, i);
			Sleep(1000);
		}
	}

	Ret = closesocket(rawSocket);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket�����ɵo�Ϳ��~:" << WSAGetLastError() << std::endl;
	}

	return true;
}

bool sendICMPReq(SOCKET &mysocket, sockaddr_in &dstAddr, unsigned int num)
{
	//�Ы�ICMP�ШD�^�����
	//�]�m�^��ШD
	ICMPrecvReq myIcmp;//ICMP�ШD����
	myIcmp.icmphead.type = 8;
	myIcmp.icmphead.code = 0;
	//�]�m��l����M��0
	myIcmp.icmphead.checkSum = 0;
	//��o�@�Ӷi�{���ѡA�o�˴N����ھڮ�����ѧO�����쪺�O���O�o�Ӷi�{�o�X�h��ICMP����
	myIcmp.icmphead.ident = (uint16_t)GetCurrentProcessId();
	//�]�m��e�Ǹ���0
	myIcmp.icmphead.seqNum = ++num;
	//�O�s�o�e�ɶ�
	myIcmp.timeStamp = GetTickCount();
	//�p��åB�O�s����M
	myIcmp.icmphead.checkSum = getCheckSum((void*)&myIcmp, ICMP);
	//�o�e����
	int Ret = sendto(mysocket, (char*)&myIcmp, sizeof(ICMPrecvReq), 0, (sockaddr*)&dstAddr, sizeof(sockaddr_in));

	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket �o�e���~:" << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

uint32_t readICMPanswer(SOCKET &mysocket, sockaddr_in &srcAddr, char &TTL)
{
	ICMPansReply icmpReply;//������������
	int addrLen = sizeof(sockaddr_in);
	//��������
	int Ret = recvfrom(mysocket, (char*)&icmpReply, sizeof(ICMPansReply), 0, (sockaddr*)&srcAddr, &addrLen);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket �������~:" << WSAGetLastError() << std::endl;
		return false;
	}
	//Ū������í��s�p����
	uint16_t checkSum = icmpReply.icmpanswer.icmphead.checkSum;
	//�]���o�X�h���ɭԭp�⪺����M�O0
	icmpReply.icmpanswer.icmphead.checkSum = 0;
	//���s�p��
	if (checkSum == getCheckSum((void*)&icmpReply.icmpanswer, ICMP)) {
		//���TTL��
		TTL = icmpReply.iphead.timetoLive;
		return icmpReply.icmpanswer.timeStamp;
	}

	return -1;
}

int waitForSocket(SOCKET &mysocket)
{
	//5S ���ݮM���r�O�_�Ѽƾ�
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
	uint32_t	timeSent;//�o�e�ɪ��ɶ�
	uint32_t	timeElapsed;//����ɶ�
	char		TTL;//����
	//�o�eICMP�^��ШD
	sendICMPReq(mysocket, dstAddr, num);
	//���ݼƾ�
	int Ret = waitForSocket(mysocket);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket�o�Ϳ��~:" << WSAGetLastError() << std::endl;
		return;
	}
	if (!Ret)
	{
		std::cout << "�ШD�W��:" << std::endl;
		return;
	}
	timeSent = readICMPanswer(mysocket, srcAddr, TTL);
	if (timeSent != -1) {
		timeElapsed = GetTickCount() - timeSent;
		//��X�H���A�`�NTTL�ȬOASCII�X�A�n�i���ഫ
		std::cout << "�Ӧ� " << inet_ntoa(srcAddr.sin_addr) << " ���^�`: �r�`= " << sizeof(ICMPrecvReq::data) << " �ɶ�= " << timeElapsed << "ms TTL= " << fabs((int)TTL) << std::endl;
	}
	else {
		std::cout << "�ШD�W��" << std::endl;
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