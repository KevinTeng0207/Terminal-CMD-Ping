#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "protocol.h"
int main(int argc, char* argv[])
{

	char dstAddr[20];
	char noparam[] = "no_param";
	char *param = NULL;

	WSAData wsadata;
	WORD	versionreq = MAKEWORD(2, 2);//使用WINsock2.2
	if (WSAStartup(versionreq, &wsadata))//初始化WINsock
	{
		std::cout << "初始化WINsock失敗" << std::endl;
	}

	/*if (versionreq != wsadata.wVersion)
	{
		std::cout << "WINsock 版本不支持" << std::endl;
	}*/

	signal(SIGINT, get_ctrl_stop);//註冊信號函數
	while (1) {
		std::cout << "Input a ping:";
		dstAddr[0] = '\0';
		std::cin.getline(dstAddr, 20);
		if ((param = isParamEmpty(dstAddr, param)) == NULL) 
			param = noparam;
		ping(dstAddr, param);
		std::cin.clear();
		std::cin.sync();
	}

	WSACleanup();



	//關閉WSA

	system("pause");
	return 0;
}