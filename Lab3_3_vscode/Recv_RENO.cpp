#include <iostream>
#include <fstream>
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <vector>
#include <cstring>
#include <ctime>
#include <process.h>
using namespace std;

#pragma comment(lib, "ws2_32")

WSADATA wsaData;
char* bufSend, * bufRecv;
SOCKET receiver,sender;
SOCKADDR_IN RecvAddr;
SOCKADDR_IN SenderAddr;
const int  MAX_PACLEN = 65535;
bool InitCon = false;
int sock_error;

string  File_Path = ".\\workfile3_3\\";
string  File_Des_Path = ".\\workfile_res3_3\\";
bool FileSent = false;
vector<char*> RecvPool;  // 缓存池

bool PreTickTime = false;   // 计时器退出指令
clock_t   lastTime = 0;
clock_t  timecount = 0;
clock_t  startTime = 0,  endTime = 0;
int waiting_max = 50; // 计时器限度
HANDLE ticktime;   // 计时器句柄

int identifier = 0;   // 当前需要匹配的ID
int Data_length;      // 数据总长
int Gram_length;      // 报文总长
int rwnd = 6;         // 流量控制窗口
int cwnd = 6 ;        // 拥塞控制窗口

string input;

USHORT geneCheckSum(char* buf, int len)
{
    if (len % 2 == 1) len++;
    unsigned char* ubuf = (unsigned char*)buf;
    register ULONG sum = 0;
    while (len)
    {
        USHORT temp = USHORT(*ubuf << 8) + USHORT(*(ubuf + 1));
        sum += temp;
        if (sum & 0xFFFF0000)
        {
            sum &= 0xFFFF;
            sum++;
        }
        ubuf += 2;
        len -= 2;
    }
    return ~(sum & 0xFFFF);
}

char* geneDataGram(SOCKADDR_IN* src, SOCKADDR_IN* tar, char* data, string name)
{
    int len = 12 + Data_length + name.length();
    Gram_length = ((len % 2 == 0) ? len : len + 1);
    bufSend = new char[Gram_length];
    bufSend[0] = rwnd >> 8;
    bufSend[1] = rwnd & 0xFF;
    bufSend[2] = cwnd >> 8;
    bufSend[3] = cwnd & 0xFF;
    bufSend[4] = len >> 8;
    bufSend[5] = len & 0xFF;
    bufSend[6] = 0;
    bufSend[7] = 0;
    bufSend[8] = identifier & 0xFF;
    bufSend[9] = 0;
    bufSend[10] = name.length() >> 8;
    bufSend[11] = name.length() & 0xFF;
    int packet_loc = 12;
    for (int i = 0; i < name.length(); i++)
        bufSend[packet_loc++] = name[i];
    for (int i = 0; i < Data_length; i++)
        bufSend[packet_loc++] = data[i];
    if (len % 2 == 1) bufSend[len] = 0;
    USHORT ck = geneCheckSum(bufSend, len);
    bufSend[6] = ck >> 8;
    bufSend[7] = ck & 0xFF;
    return bufSend;
}

string getFileName()
{
    string name;
    int len = (int)(USHORT((unsigned char)(bufRecv[10]) << 8)+USHORT((unsigned char)(bufRecv[11])));
    for (int i = 0; i < len; i++)
        name += bufRecv[i + 12];
    return name;
}

void updateCheckSum()
{
    bufSend[6] = 0;
    bufSend[7] = 0;
    USHORT ACK = geneCheckSum(bufSend, Gram_length);
    bufSend[6] = ACK >> 8;
    bufSend[7] = ACK & 0xFF;
}

bool checkID() { return (identifier == int(USHORT((unsigned char)bufRecv[8]))); }
bool checkACK() { return bufRecv[9] & 0x01; }
bool checkFIN() { return bufRecv[9] & 0x04; }
bool checkSYN() { return bufRecv[9] & 0x02; }
bool getOVER() { return bufRecv[9] & 0x08; }

void setNAK() { bufSend[9] &= 0xFE; updateCheckSum(); }
void setACK() { bufSend[9] |= 0x01; updateCheckSum(); }
void setNFIN() { bufSend[9] &= 0xFB; updateCheckSum(); }
void setID(USHORT a) { bufSend[8] = a & 0xFF; updateCheckSum(); }
void setNSYN() { bufSend[9] &= 0xFD; updateCheckSum(); }
void setFIN() { 
    bufSend[9] |= 0x04; 
    bufSend[8] = bufRecv[8] + 1;
    updateCheckSum(); 
}
void setSYN() { 
    bufSend[9] |= 0x02; 
    bufSend[8] = bufRecv[8] + 1;    //回复的是原始序列号+1
    //cout<<"send buf"<<int(USHORT((unsigned char)bufSend[8]))<<endl;
    updateCheckSum();
}

int getID(char* buf) { return int(USHORT((unsigned char)buf[8])); }

// 开线程： 用来计时（用于超时等待）
DWORD WINAPI TimeCheck(LPVOID lparam)
{
    int* waiting_max = (int*)lparam;
    while (true)
    {
        if (PreTickTime) return 0;    //退掉原计时线程，以免开的太多忘记关...
        endTime = clock();
        lastTime = endTime - startTime;
        if (lastTime / CLOCKS_PER_SEC > timecount)    // CLOCKS_PER_SEC=1000
        {
            timecount = lastTime / CLOCKS_PER_SEC;
            cout <<  "*******    ticks : timed waited for information: ";
            cout << lastTime * 1.0 / CLOCKS_PER_SEC << "s     ********" << endl;
        }
        if (lastTime / CLOCKS_PER_SEC == *waiting_max)
        {
            delete[] bufSend;
            Data_length = 0;
            bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
            setFIN();    //告知对方关闭
            sock_error = sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, sizeof(SOCKADDR));
            cout <<  "The program is going to close ... " << endl;
            timecount = 0;
            CloseHandle(ticktime);
            delete[] bufRecv;
            delete[] bufSend;
            exit(0);
        }
    }
    return 0;
}

int main()
{
    const WORD wVersionRequested = MAKEWORD(2, 2);
    sock_error = WSAStartup(wVersionRequested, &wsaData);
    if (sock_error != 0)
    {
        cout <<   "sock_error code " << WSAGetLastError() << endl;
        cout <<   "initialize WSA failed" << endl;
        return -1;
    }

    receiver = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver == INVALID_SOCKET)
    {
        cout <<   "bind sock_error" << endl;
        sock_error = WSACleanup();
        return -1;
    }

    RecvAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(6665);
    sock_error = bind(receiver, (SOCKADDR*)&RecvAddr, sizeof(SOCKADDR));
    if (sock_error != 0)
    {
        cout <<   "sock_error code " << WSAGetLastError() << endl;
        cout <<   "bind sock_error" << endl;
        sock_error = WSACleanup();
        return -1;
    }
    cout <<   "bind successfully" << endl;

    int len = sizeof(SOCKADDR);
    SenderAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    SenderAddr.sin_family = AF_INET;
    SenderAddr.sin_port = htons(8887);

    cout << "The Receiver's max waiting time is :"<< waiting_max <<endl;

    startTime = clock();
    PreTickTime = false;
    // 等待的时候就要开始计时
    ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);

    while (!InitCon)
    {
        cout << "wait for others to connect  ..." << endl;
        bufRecv = new char[MAX_PACLEN];
        sock_error = recvfrom(receiver, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &len);
        if (sock_error == SOCKET_ERROR || !checkSYN())
        {
            delete[] bufRecv;
            continue;
        }
        Data_length = 0;
        bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
        setACK();
        setSYN();
        sock_error = sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, len);
        if (sock_error != Gram_length)
        {
            delete[] bufRecv;
            continue;
        }
        InitCon = true;
        PreTickTime = true;    //原线程应该会关掉
        Sleep(10);
        CloseHandle(ticktime);    //（确保关闭） ->   这里关掉了，下面要再开起来
        delete[] bufRecv;
    }

    // wait & send
    cout <<   "connect successfully" << endl;

    // 连接建立后，重新开始计时
    startTime = clock();
    timecount = 0;
    ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);
    identifier=0;
    Data_length = 0;
    bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
    setACK();
    PreTickTime = false;
    while (true)
    {
        // receive
        bufRecv = new char[ MAX_PACLEN];
        sock_error = recvfrom(receiver, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &len);
        rwnd = (int)((USHORT)((unsigned char)bufRecv[0] << 8)+(USHORT)(unsigned char)bufRecv[1]);
        USHORT CheckSum = geneCheckSum(bufRecv, sock_error);
        if (!CheckSum && checkID())
        {
            if (checkFIN())
            {
                //发一个回复FIN的报文
                bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
                setFIN();
                sock_error = sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, len);
                cout <<  "The program is going to close ... " ;
                delete[] bufRecv;
                delete[] bufSend;

                PreTickTime = true;
                Sleep(10);
                CloseHandle(ticktime);
                InitCon = false;
                PreTickTime = false;
                timecount = 0;
                system("pause");
                return 0;
            }
            cout<< "get packet: "<< identifier <<endl;
            RecvPool.push_back((char*)"");
            RecvPool[identifier] = new char[sock_error];
            for (int i = 0; i < sock_error; i++)
                RecvPool[identifier][i] = bufRecv[i];
            setID(identifier);
            identifier = (identifier + 1) % 256;
            if (getOVER())          // 借用了分片里面的结束符思想
            {
                string name = getFileName();
                ofstream os(( File_Des_Path + name).c_str(), ios::app | ios::binary);
                for (int j = 0; j <= RecvPool.size()-1; j++)
                {
                    bufRecv = RecvPool[j];
                    Gram_length = (int)((USHORT)((unsigned char)bufRecv[4] << 8)+(USHORT)(unsigned char)bufRecv[5]);
                    os.seekp(0, os.end);
                    for (int i = 0; i < Gram_length - name.length() - 12; i++)
                        os << (unsigned char)bufRecv[i + 12 + name.length()];
                }
                os.close();
                cout <<   "file " << name << " saved" << endl;
                FileSent = true;
            }
        }
        else
        {
            cout << " Received ID is: " << getID(bufRecv) << endl;
        }

        sock_error = sendto(receiver,bufSend, 12 ,0,(SOCKADDR*)&SenderAddr,len);

        if (FileSent)
        {
            FileSent = false;
            RecvPool.clear();       //开始接收新文件
            Data_length = 0;
            identifier = 0;          // 做好重新接收的准备
            delete[] bufSend;
            bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
            setACK();
        }

        // 每收到新的报文后，重新计时
        startTime = clock();
        timecount = 0;
        PreTickTime = true;
        Sleep(10);           // 关了原线程
        PreTickTime = false;
        ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);
    }

    // close
    CloseHandle(ticktime);
    sock_error = closesocket(receiver);
    sock_error = WSACleanup();
    return 0;
}