////����ʵ�ֵ��Ǽ򵥵�һ̨��������һ���ͻ��˵Ľ�����
//
//#include <WinSock2.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include<iostream>
//using namespace std;
//#pragma comment(lib,"ws2_32.lib")
//
//
//int main()
//{
//	WSADATA wsaData;
//	//�ж��Լ���Ҫ�İ汾��ϵͳ���õİ汾�Ƿ�һ��
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
//	{
//		cout << "WSAStartup failed!" << endl;
//		return 1;
//	}
//
//	//����������client�����socket
//	SOCKET ClientSocket;
//	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (ClientSocket == INVALID_SOCKET)
//	{
//		cout << "socket failed with error code:" << WSAGetLastError() << endl;
//		return -1;
//	}
//
//	char clientName[32] = { 0 };
//	cout << "please input your name as a client: " << endl;
//	cin >> clientName;
//
//	//����Ϊ��Ҫ���ӵ�Զ�˽������á�
//	char IP[32] = { 0 };
//	cout << "please input the IP of the server you want to connect: " << endl;
//	cin >> IP;
//
//	SOCKADDR_IN ServerAddr;
//	USHORT uPort = 666;
//	ServerAddr.sin_family = AF_INET;
//	ServerAddr.sin_port = htons(uPort);
//	ServerAddr.sin_addr.S_un.S_addr = inet_addr(IP);
//
//
//	cout << "start connecting...please wait for the server to connect..." << endl;
//	//���Լ���socket��Զ�����ӣ�
//	if (SOCKET_ERROR == connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
//	{
//		cout << "connect failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ClientSocket);
//		WSACleanup();
//		return -1;
//	}
//	
//	cout << "Now you are successfully connected to a server!Have a chat now!You can input 'seeu' to end the commucation!" << endl;
//	cout << "The server's information is shown as below:" << endl;
//	cout << "The server's Ip is: "<<inet_ntoa(ServerAddr.sin_addr)<<"      "<<"The server's port is: "<< htons(ServerAddr.sin_port)<<endl<<endl;
//
//	//�����Ƕ��շ���Ϣ�Ĵ洢����ʾ
//	char buffer[4096] = { 0 };
//	char serverName[32] = { 0 };
//	int RecvLen = 0;    //ʵ���յ����ֽ���
//	int SendLen = 0;    //ʵ�ʷ��͵��ֽ���
//
//	//���Լ������ַ���ȥ
//	SendLen = send(ClientSocket, clientName, strlen(clientName), 0);
//	if (SOCKET_ERROR == SendLen)
//	{
//		cout << "send failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ClientSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//���նԷ������֣����serverName
//	//˫������˭�ȷ��ͽ����ַ����ڵȵ��Է��ظ�һ����Ϣ��˫�����������
//	RecvLen = recv(ClientSocket, serverName, sizeof(serverName), 0);
//	if (SOCKET_ERROR == RecvLen)
//	{
//		cout << "recv failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ClientSocket);
//		WSACleanup();
//		return -1;
//	}
//	strcat_s(serverName, "\0");
//
//	while (true)
//	{
//		memset(buffer, 0, sizeof(buffer));
//		cout << clientName << ":";
//		cin >> buffer;
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			SendLen = send(ClientSocket, buffer, strlen(buffer), 0);
//			if (SOCKET_ERROR == SendLen)
//			{
//				cout << "send failed with error code:" << WSAGetLastError() << endl;
//				closesocket(ClientSocket);
//				WSACleanup();
//				return -1;
//			}
//			cout << serverName << ":";
//			memset(buffer, 0, sizeof(buffer));
//			RecvLen = recv(ClientSocket, buffer, sizeof(buffer), 0);
//			cout << buffer << endl;
//			break;
//		}
//		SendLen = send(ClientSocket, buffer, strlen(buffer), 0);
//		if (SOCKET_ERROR == SendLen)
//		{
//			cout << "send failed with error code:" << WSAGetLastError() << endl;
//			closesocket(ClientSocket);
//			WSACleanup();
//			return -1;
//		}
//
//		cout << serverName<<":";
//		memset(buffer, 0, sizeof(buffer));
//		RecvLen = recv(ClientSocket, buffer, sizeof(buffer), 0);
//		if (SOCKET_ERROR == RecvLen)
//		{
//			cout << "recv failed with error code:" << WSAGetLastError() << endl;
//			closesocket(ClientSocket);
//			WSACleanup();
//			return -1;
//		}
//		strcat_s(buffer, "\0");
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			cout << buffer << endl;
//			memset(buffer, 0, sizeof(buffer));
//			cout << clientName << ":";
//			cin >> buffer;
//			SendLen = send(ClientSocket, buffer, strlen(buffer), 0);
//			break;
//		}
//		cout << buffer << endl;
//	}
//
//	closesocket(ClientSocket);
//	WSACleanup();
//	system("pause");
//	return 0;
//}