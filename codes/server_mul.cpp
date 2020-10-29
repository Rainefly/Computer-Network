#include <WinSock2.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include<iostream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

#define SEND_OVER 1							 //�Ѿ�ת����Ϣ
#define SEND_YET  0							 //��ûת����Ϣ
int i = 0;
int Status = SEND_YET;

SOCKET ServerSocket = INVALID_SOCKET;		 //������׽���
SOCKADDR_IN ClientAddr = { 0 };			 //�ͻ��˵�ַ
int ClientAddrLen = sizeof(ClientAddr);

bool CheckConnect = false;                //����������
HANDLE HandleRecv[10] = { NULL };				 //�߳̾��
HANDLE mainHandle;							 //����accept���߳̾��

//�ͻ�����Ϣ�ṹ��
typedef struct _Client
{
	SOCKET sClient;      //�ͻ����׽���
	char buffer[128];		 //���ݻ�����
	char userName[16];   //�ͻ����û���
	char identify[16];   //���ڱ�ʶת���ķ�Χ
	char IP[20];		 //�ͻ���IP
	UINT_PTR flag;       //��ǿͻ��ˣ��������ֲ�ͬ�Ŀͻ���
} Client;

Client inClient[10] = { 0 };                  //����һ���ͻ��˽ṹ��,���ͬʱ10������

//���������߳�
unsigned __stdcall ThreadSend(void* param)
{
	int error = 0;
	int flag = *(int*)param;
	SOCKET client = INVALID_SOCKET;					//����һ����ʱ�׽��������Ҫת���Ŀͻ����׽���
	char temp[128] = { 0 };							//����һ����ʱ�����ݻ�������������Ž��յ�������
	memcpy(temp, inClient[flag].buffer, sizeof(temp));
	sprintf_s(inClient[flag].buffer, "%s: %s", inClient[flag].userName, temp); //�ѷ���Դ���������ת������Ϣ��
	if (strlen(temp) != 0 && Status == SEND_YET) //������ݲ�Ϊ���һ�ûת����ת��
	{
		if (strcmp(inClient[flag].identify, "all") == 0)
		{
			for (int j = 0; j < i; j++) {
				if (j != flag) {//����Լ�֮������пͻ��˷�����Ϣ
					error = send(inClient[j].sClient, inClient[flag].buffer, sizeof(inClient[j].buffer), 0);
					if (error == SOCKET_ERROR)
						return 1;
				}
			}
		}
		else
		{
			for(int j=0;j<i;j++)
				if (strcmp(inClient[j].userName, inClient[flag].identify) == 0)
				{
					error = send(inClient[j].sClient, inClient[flag].buffer, sizeof(inClient[j].buffer), 0);
					if (error == SOCKET_ERROR)
						return 1;
				}
		}
	}
		
	Status = SEND_OVER;   //ת���ɹ�������״̬Ϊ��ת��
	return 0;
}

//���������߳�
unsigned __stdcall ThreadRecv(void* param)
{
	SOCKET client = INVALID_SOCKET;
	int flag = 0;
	for (int j = 0; j < i; j++) {
		if (*(int*)param == inClient[j].flag)            //�ж���Ϊ�ĸ��ͻ��˿��ٵĽ��������߳�
		{
			client = inClient[j].sClient;
			flag = j;
		}
	}
	char temp[128] = { 0 };  //��ʱ���ݻ�����
	while (true)
	{
		memset(temp, 0, sizeof(temp));
		int error = recv(client, temp, sizeof(temp), 0); //��������
		if (error == SOCKET_ERROR)
			continue;
		Status = SEND_YET;	//����ת��״̬Ϊδת��
		memcpy(inClient[flag].buffer, temp, sizeof(inClient[flag].buffer));
		if (strcmp(temp, "exit") == 0)   //�ж�����ͻ�����exit������ôֱ�ӹر��̣߳�����ת���߳�
		{
			closesocket(inClient[flag].sClient);//�رո��׽���
			CloseHandle(HandleRecv[flag]); //����ر����߳̾��
			inClient[flag].sClient = 0;  //�����λ�ÿճ����������Ժ������߳�ʹ��
			HandleRecv[flag] = NULL;
			//��printf��ȡ����
			cout << "'"<<inClient[flag].userName <<"'" << "has disconnected from the server. " << endl;
		}
		else
		{
			cout << inClient[flag].userName <<": "<< temp << endl;
			_beginthreadex(NULL, 0, ThreadSend, &flag, 0, NULL); //����һ��ת���߳�,flag����������Ҫ��ת������Ϣ�Ǵ��ĸ��߳����ģ�
		}
	}
	return 0;
}


//��������
unsigned __stdcall MainAccept(void* param)
{
	int flag[10] = { 0 };
	while (true)
	{
		if (inClient[i].flag != 0)   //�ҵ���ǰ�����һ��û�����ӵ�inClient
		{
			i++;
			continue;
		}
		//����пͻ����������Ӿͽ�������
		if ((inClient[i].sClient = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &ClientAddrLen)) == INVALID_SOCKET)
		{
			cout << "Accept error: " << WSAGetLastError() << endl;
			closesocket(ServerSocket);
			WSACleanup();
			return -1;
		}
		recv(inClient[i].sClient, inClient[i].userName, sizeof(inClient[i].userName), 0); //�����û���
		recv(inClient[i].sClient, inClient[i].identify, sizeof(inClient[i].identify), 0); //����ת���ķ�Χ

		cout << "The user: '" << inClient[i].userName <<"'"<< " connect successfully!" << endl;
		memcpy(inClient[i].IP, inet_ntoa(ClientAddr.sin_addr), sizeof(inClient[i].IP)); //��¼�ͻ���IP
		inClient[i].flag = inClient[i].sClient; //��ͬ��socke�в�ͬUINT_PTR���͵���������ʶ
		i++;    //���ȥ������ʼ�ĸ�ֵ�ᱨ��
		for (int j = 0; j < i; j++) 
		{
			if (inClient[j].flag != flag[j]) 
			{
				if (HandleRecv[j])   //����Ǹ�����Ѿ��������ˣ���ô�Ǿ͹ص��Ǹ����
					CloseHandle(HandleRecv[j]);
				HandleRecv[j] = (HANDLE)_beginthreadex(NULL, 0, ThreadRecv, &inClient[j].flag, 0, NULL); //����������Ϣ���߳�
			}
		}

		for (int j = 0; j < i; j++) 
		{
			flag[j] = inClient[j].flag;//��ֹThreadRecv�̶߳�ο���
		}
		Sleep(3000);
	}
	return 0;
}

int main()
{
	WSADATA wsaData = { 0 };

	//��ʼ���׽���
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "WSAStartup error: " << WSAGetLastError() << endl;
		return -1;
	}

	//�����׽���
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET)
	{
		cout << "Socket error: " << WSAGetLastError() << endl;
		return -1;
	}

	SOCKADDR_IN ServerAddr = { 0 };				//����˵�ַ
	USHORT uPort = 1666;						//�����������˿�
	//���÷�������ַ
	ServerAddr.sin_family = AF_INET;//���ӷ�ʽ
	ServerAddr.sin_port = htons(uPort);//�����������˿�
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//�κοͻ��˶����������������

	//�󶨷�����
	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		cout << "Bind error: " << WSAGetLastError() << endl;
		closesocket(ServerSocket);
		return -1;
	}

	//���ü����ͻ���������
	if (SOCKET_ERROR == listen(ServerSocket, 20))
	{
		cout << "Listen error: " << WSAGetLastError() << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return -1;
	}

	cout << "please wait for other clients to wait..." << endl;
	mainHandle = (HANDLE)_beginthreadex(NULL, 0, MainAccept, NULL, 0, 0);   //mainHandle����������client����Ҫ�߳�
	char Serversignal[10];
	cout << "if you want to close the server, please input 'exit' " << endl;
	while (true)
	{
		cin.getline(Serversignal,10);
		if (strcmp(Serversignal, "exit") == 0) 
		{
			cout << "The server is closed. " << endl;
			CloseHandle(mainHandle);
			for (int j = 0; j < i; j++) //���ιر��׽���
			{
				if (inClient[j].sClient != INVALID_SOCKET)
					closesocket(inClient[j].sClient);
			}
			closesocket(ServerSocket);
			WSACleanup();
			exit(1);
			return 1;
		}
	}
	return 0;
}