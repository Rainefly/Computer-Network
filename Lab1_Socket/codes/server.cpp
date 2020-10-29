//#include <WinSock2.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include<iostream>
//
//#pragma comment(lib,"ws2_32.lib")
//
//using namespace std;
//
//int main()
//{
//	//�����ÿ���socket����ϸ��Ϣ�ı���
//	WSADATA wsaData;
//
//	//�ж��Լ���Ҫ�İ汾��ϵͳ���õİ汾�Ƿ�һ��
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
//	{
//		cout << "WSAStartup failed!" << endl;
//		return -1;
//	}
//
//	//�ȴ��������socket
//	SOCKET ServerSocket; //server��socket
//	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //ipv4�ĵ�ַ���ͣ����ṹ�ķ������ͣ�TCP��Э�飻
//
//	if (ServerSocket == INVALID_SOCKET)
//	{
//
//		cout << "socket failed with error code:" << WSAGetLastError() << endl;
//		return -1;
//	}
//
//	//����Ҫ�ѱ��ص�ַbind��socket��
//	SOCKADDR_IN ServerAddr;
//	USHORT uPort = 666;
//	ServerAddr.sin_family = AF_INET;
//	ServerAddr.sin_port = htons(uPort);
//	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//	int iServerAddLen = sizeof(ServerAddr);
//
//	//�󶨲�����
//	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
//	{
//		printf("bind failed with error code: %d\n", WSAGetLastError());
//		closesocket(ServerSocket);
//		return -1;
//	}
//
//	//�����ʼ������ʾ��������������
//	cout << "please input your name as a server: " << endl;
//	char serverName[32] = { 0 };
//	cin >> serverName;
//
//	cout << "start listening...please wait for other users to connect..." << endl;
//
//	//����ɺ�ʼlisten���������õĵȴ����г�����1
//	if (SOCKET_ERROR == listen(ServerSocket, 1))
//	{
//		cout << "listen failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//����Ϊ������accept�����µ��߳����socket��׼��
//	SOCKET Conn_new_Socket;
//	SOCKADDR_IN Conn_new_Addr;
//	int iClientAddrLen = sizeof(Conn_new_Addr);
//
//	//Conn_new_Socket����Ϣ����bind����Ϊ��������serveraccept�ĵ�ַ������
//	Conn_new_Socket = accept(ServerSocket, (SOCKADDR*)&Conn_new_Addr, &iClientAddrLen);
//
//	if (Conn_new_Socket == INVALID_SOCKET)
//	{
//		cout << "accept failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//������ʾ����ʾ�����ߵ���Ϣ
//	cout << "Now a user is successfully connectting to you! Have a chat now! You can input 'seeu' to end the commucation!" << endl;
//	cout << "The connector's informaintion is shown as below:" << endl;
//	cout << "connector's ip is: " << inet_ntoa(Conn_new_Addr.sin_addr) << "    " << "connector's port is: " << htons(Conn_new_Addr.sin_port) << endl << endl;
//
//	//���շ�����Ϣ���д洢����ʾ
//	char clientName[32] = { 0 };
//	char buffer[4096] = { 0 };
//	int RecvLen = 0;  //ʵ���յ����ֽ���
//	int SendLen = 0;  //ʵ�ʷ��͵��ֽ���
//
//	//��serverName�����Է�
//	SendLen = send(Conn_new_Socket, serverName, strlen(serverName), 0);
//	if (SOCKET_ERROR == SendLen)
//	{
//		cout << "send failed with error code:" << WSAGetLastError() << endl;
//		closesocket(Conn_new_Socket);
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//���նԷ�����������Ϣ
//	RecvLen = recv(Conn_new_Socket, clientName, sizeof(clientName), 0);
//	if (SOCKET_ERROR == RecvLen)
//	{
//		cout << "recv failed with error code:" << WSAGetLastError() << endl;
//		closesocket(Conn_new_Socket);
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//	strcat_s(clientName, "\0");
//
//	//������߼���������ʾ��client���յ�����Ϣ���ж��Ƿ��ǽ����ַ���
//	//��������server�Լ��뷢����Ϣ������clientȥ;
//	//����һ�������������������ô�ȶԷ��ظ�һ����Ϣ�����߶�������Ի���
//	while (true)
//	{
//		cout << clientName << ":";
//		memset(buffer, 0, sizeof(buffer));
//		RecvLen = recv(Conn_new_Socket, buffer, sizeof(buffer), 0);
//		if (SOCKET_ERROR == RecvLen)
//		{
//			cout << "recv failed with error code:" << WSAGetLastError() << endl;
//			closesocket(Conn_new_Socket);
//			closesocket(ServerSocket);
//			WSACleanup();
//			return -1;
//		}
//		strcat_s(buffer, "\0");
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			cout << buffer << endl;
//			memset(buffer, 0, sizeof(buffer));
//			cout << serverName << ":";
//			cin >> buffer;
//			SendLen = send(Conn_new_Socket, buffer, strlen(buffer), 0);
//			break;
//		}
//		cout << buffer << endl;
//
//
//		memset(buffer, 0, sizeof(buffer));
//		cout << serverName << ":";
//		cin >> buffer;
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			SendLen = send(Conn_new_Socket, buffer, strlen(buffer), 0);
//			if (SOCKET_ERROR == SendLen)
//			{
//				cout << "send failed with error code:" << WSAGetLastError() << endl;
//				closesocket(Conn_new_Socket);
//				closesocket(ServerSocket);
//				WSACleanup();
//				return -1;
//			}
//			cout << clientName << ":";
//			memset(buffer, 0, sizeof(buffer));
//			RecvLen = recv(Conn_new_Socket, buffer, sizeof(buffer), 0);
//			cout << buffer << endl;
//			break;
//		}
//		SendLen = send(Conn_new_Socket, buffer, strlen(buffer), 0);
//		if (SOCKET_ERROR == SendLen)
//		{
//			cout << "send failed with error code:" << WSAGetLastError() << endl;
//			closesocket(Conn_new_Socket);
//			closesocket(ServerSocket);
//			WSACleanup();
//			return -1;
//		}
//	}
//
//	closesocket(Conn_new_Socket);
//	closesocket(ServerSocket);
//	WSACleanup();
//	system("pause");
//	return 0;
//}