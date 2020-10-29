#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include<iostream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

#define RECV_OVER 1    //���յ�
#define RECV_YET 0     //��û�յ�
char userName[16] = { 0 };
boolean isPrint = false;  // �ж��Ƿ�Ҫ�ڿͻ��˴�ӡ����
int Status = RECV_YET;

//���������߳�
unsigned __stdcall ThreadRecv(void* param)
{
	char bufferRecv[128] = { 0 };
	while (true)
	{
		int error = recv(*(SOCKET*)param, bufferRecv, sizeof(bufferRecv), 0);
		if (error == SOCKET_ERROR)
		{
			Sleep(500);
			continue;
		}
		if (strlen(bufferRecv) != 0)
		{
			if (isPrint)
			{
				for (int i = 0; i <= strlen(userName) + 2; i++)
					cout << "\b";
				cout << bufferRecv << endl;
				//printf("\r");    //����֮ǰ������
                //printf("%s\n", bufferRecv);
				cout << userName << ": ";   //��Ϊ�������û���send̬ʱ���ѱ�����ӡ������userName���˻�ȥ�ˣ������յ��Ժ���Ҫ�ٰ�userName��ӡ����
				Status = RECV_OVER;
			}
			else
				cout << bufferRecv << endl;
		}
		else
			Sleep(100);
	}
	return 0;
}


int main()
{
	WSADATA wsaData = { 0 };//����׽�����Ϣ
	SOCKET ClientSocket = INVALID_SOCKET;//�ͻ����׽���
	SOCKADDR_IN ServerAddr = { 0 };//����˵�ַ
	USHORT uPort = 1666;//����˶˿�
						 //��ʼ���׽���
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))//�����ļ��ͳ����
	{
		cout << "WSAStartup error: " << WSAGetLastError() << endl;
		return -1;
	}

	//�����׽���
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET)
	{
		cout << "Socket error: " << WSAGetLastError() << endl;
		return -1;
	}

	//���������IP
	/*cout << "Please input the server IP:";
	char IP[32] = { 0 };
	cin.getline(IP,32);*/


	//���÷�������ַ
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(uPort);//�������˿�
	ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//��������ַ

	cout << "connecting...please wait..." << endl;

	//���ӷ�����
	if (SOCKET_ERROR == connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		cout << "Connect error: "<<WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	cout << "Connect successfully!" << "The server's ip is:" << inet_ntoa(ServerAddr.sin_addr);
	cout << "The server's port is:" << htons(ServerAddr.sin_port) << endl << endl;
	cout << "Please input your name: ";

	cin.getline(userName,16);
	cout << "You can input 'exit' to close the client. " << endl;
	send(ClientSocket, userName, sizeof(userName), 0);
	cout << endl;

	char identify[16] = { 0 };
	cout << "You can choose the other client to chat. If you input'all',then everyone can receive your message; if you input the client's name, then the only client can receive your message. " << endl;
	cout << "choose the client: ";
	cin.getline(identify, 16);
	send(ClientSocket, identify, sizeof(identify), 0);

	_beginthreadex(NULL, 0, ThreadRecv, &ClientSocket, 0, NULL); //���������߳�

	char bufferSend[128] = { 0 };

	while (1)
	{
		if (isPrint != true)
		{
			cout << userName << ": ";
			isPrint = true;
		}
		cin.getline(bufferSend,128);
		if (strcmp(bufferSend, "exit") == 0)
		{
			cout << "you are going to exit. " << endl;
			Sleep(2000);
			int error = send(ClientSocket, bufferSend, sizeof(bufferSend), 0);
			if (error == SOCKET_ERROR)
				return -1;    //�˳���ǰ�߳�
			return 0;   //�̻߳�ر�
		}

		int error = send(ClientSocket, bufferSend, sizeof(bufferSend), 0);
		if (error == SOCKET_ERROR)
			return -1;    //�˳���ǰ�߳�
		if (error != 0)
		{
			isPrint = false;
		}
	}
	for (int k = 0; k < 1000; k++)    //�����߳�һֱת
		Sleep(10000000);

	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}