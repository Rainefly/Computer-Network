#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <io.h>
#include <iomanip>
#include <process.h>
#include <vector>

using namespace std;

#pragma comment(lib,"ws2_32.lib")

// WSA & Socket 
WSADATA wsaData;
SOCKADDR_IN SenderAddr, RecvAddr;
SOCKET sender;
char* bufSend, * bufRecv;
int sock_error;

// 发送所需变量
vector<char*> SendReserve;              
int SendBase = 0;  
int identifier = 0;  
bool FileSent = false;                // 文件传输结束标志  
int Data_length;                        // 数据长度
int Gram_length;                        // 报文长度
int Max_Resend = 10, Cur_Resend = 0;   // 最大失败限度
const int  MAX_PACLEN = 65535 - 256;

// 读写文件所需
string  File_Path = ".\\workfile3_3\\";
string  File_Des_Path = ".\\workfile_res3_3\\";
struct _finddata_t info;
vector<string> files;
int packetNum=0;        // 算得的发送该文件所需的总报文数
int out=0;              // 用于和上面的packetNum比较，跳出读文件

// 计时线程所需
int RTO = 2;                 // 超时时间(s)
clock_t  startTime,  endTime;          // 局部计时
clock_t fileStartTime,fileEndTime;          // 一个文件的总发送时间
int  timecount = 0;
bool PreTickTime = false;             // 计时器退出标志

// 线程所需
HANDLE recver, ticktime;
long handle;

// 用户输入
string input;

int swnd = 6;              
float cwnd = 1;

// RENO状态机变量
int dupACK=-1;
int dupcount=0;
int ssthresh = 8 ;
int state=0;   //0是慢启动和拥塞避免状态 ；1是快速恢复状态


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
    bufSend[0] = swnd >> 8;
    bufSend[1] = swnd & 0xFF;
    bufSend[2] = int(cwnd) >> 8;
    bufSend[3] = int(cwnd) & 0xFF;
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

void updateCheckSum()
{
    USHORT CheckSum = (USHORT(((unsigned char)bufSend[6]) << 8) + USHORT((unsigned char)bufSend[7]));
    register ULONG sum = (~CheckSum & 0xFFFF) + USHORT((unsigned char)bufSend[9]);
    if (sum & 0xFFFF0000)
        CheckSum = (sum & 0xFFFF) + 1;
    else
        CheckSum = sum & 0xFFFF;
    CheckSum = ~CheckSum;
    bufSend[6] = CheckSum >> 8;
    bufSend[7] = CheckSum & 0xFF;
}

bool checkACK() { return bufRecv[9] & 0x01; }
bool checkFIN() { return bufRecv[9] & 0x04; }
bool checkSYN()  { return ((identifier + 1) == int(USHORT((unsigned char)bufRecv[8])))&&(bufRecv[9] & 0x02);}   //对于SYN的应答要检查两项
bool checkRecvFIN()  {return ((out + 1) == int(USHORT((unsigned char)bufRecv[8]))); }
bool checkID() { return (SendBase == int(USHORT((unsigned char)bufRecv[8]))); }
bool checknewACK(){ return dupACK!=int(USHORT((unsigned char)bufRecv[8]));}

void setNAK() { bufSend[9] &= 0xFE; updateCheckSum(); }
void setACK() { bufSend[9] |= 0x01; updateCheckSum(); }
void setOVER() { bufSend[9] |= 0x08; updateCheckSum(); }
void setNFIN() { bufSend[9] &= 0xFB; updateCheckSum(); }
void setFIN() { bufSend[9] |= 0x04; updateCheckSum(); }
void setID(USHORT x) { bufSend[8] = x & 0xFF; updateCheckSum(); }
void setNSYN() { bufSend[9] &= 0xFD; updateCheckSum(); }
void setSYN() { bufSend[9] |= 0x02; updateCheckSum(); }
void setDup()
{
    if(state==0)
    {
        if(dupcount==3)
        {
            ssthresh=cwnd/2;
            cwnd=ssthresh+3;
            state=1;
        }
    }
    else if(state==1)
    {
        cwnd+=1;
    }
}

int getID(char* buf) { return USHORT((unsigned char)buf[8]); }


DWORD WINAPI TimeCheck(LPVOID lparam)
{
    while (true)
    {
        if (PreTickTime)
        {
            //cout << " Check presend-time out... " << SendBase - 1 << endl;
            return 0;
        }
        endTime = clock();
        clock_t   lastTime =  endTime -  startTime;
        if (  lastTime / CLOCKS_PER_SEC == RTO)
        {
            // 把重发线程置为最高优先级的线程，避免其他线程并行化出现bug
            SetPriorityClass(ticktime, HIGH_PRIORITY_CLASS);
            cout << "re-send packet from " << SendBase << " to "<<identifier -1 <<endl;
            for (int i = SendBase; i < identifier; i++)   //即发出：SendBase ~ identifier-1 的报文
            {
                int length = (int)((USHORT)((unsigned char)SendReserve[i][4] << 8)+(USHORT)(unsigned char)SendReserve[i][5]);
                length = (length % 2 == 0 ? length : length + 1);
                sendto(sender,SendReserve[i],length,0,(SOCKADDR*)&RecvAddr,sizeof(SOCKADDR));
                //cout << "Data_length is : " << i << ": " << length << endl;
                Sleep(10);
            }
            ssthresh=cwnd/2;
            cwnd=1;
            cout<<"Time out ... The window size is fixed ... "<<cwnd<<endl;
            dupcount=0;
            state=0;
            startTime = clock();
            PreTickTime = false;
            timecount = 0;
            // 恢复线程并行
            SetPriorityClass(ticktime, NORMAL_PRIORITY_CLASS);
            Sleep(10);
        }
    }
    return 0;
}

DWORD WINAPI RecvThread(LPVOID lparam)
{
    int length = sizeof(SOCKADDR);
    while (true)
    {
        int sock_error = recvfrom(sender,bufRecv,MAX_PACLEN,0,(SOCKADDR*)&RecvAddr,&length);
        int Gram_length = (int)((USHORT)((unsigned char)bufRecv[4] << 8)+(USHORT)(unsigned char)bufRecv[5]);
        Gram_length = ((Gram_length % 2 == 0) ? Gram_length : Gram_length + 1);
        if (sock_error != Gram_length) continue;        //收报文有误，再收
        USHORT CheckSum = geneCheckSum(bufRecv, sock_error);
        if (!CheckSum&&checkID()&&checknewACK())   //Checksum应该是0,ID是应该是Sendbase
        {
            dupACK=getID(bufRecv);     //更新收到的ACK
            dupcount=0;
            SendBase = getID(bufRecv) + 1;
            cout << "Right ACK: SendBase is : " << SendBase << " ID is: " << identifier << endl;
            PreTickTime = true;
            Sleep(10);
            if(state==0)
            {
                if(cwnd<ssthresh)   //慢启动阶段
               {
                  cwnd+=1;
                  cout<<" slow start-- now the cwnd size is: "<<cwnd<<endl;
               }
                else
                {
                    cwnd=cwnd+1.0/cwnd;   //拥塞避免阶段
                    cout<<"control-- now the cwnd size is: "<<cwnd<<"  integer formatis : "<<int(cwnd)<<endl;
                }
            }
            else if(state==1)
            {
                cwnd=ssthresh;
                cout<<"quick resend --now the cwnd size is: "<<cwnd<<endl;
                state=0;
            }
            if (SendBase == SendReserve.size())
            {
                FileSent = true;
                return 0;
            }
            if (SendBase != identifier)
            {
                cout << "the Sendbase is not up to the identifier. Now the Sendbase is:  " << SendBase << endl;
                PreTickTime = false;
                ticktime = CreateThread(NULL, NULL, TimeCheck, (LPVOID)sender, NULL, NULL);
            }
        }
        else
        {
            if (CheckSum) cout << "CheckSum: " << CheckSum <<"The receiver's packet has something wrong ..."<< endl;
            if (!checkID()) cout << "Sender need ID: " << SendBase << endl;
            if(!checknewACK()) 
            {
                cout<<"Dupclicated ACK: "<<dupACK<<endl;
                dupcount++;
                setDup();
            }
        }
        Sleep(10);
    }
    return 0;
}

int main()
{
    const WORD wVersionRequested = MAKEWORD(2, 2);
    sock_error = WSAStartup(wVersionRequested, &wsaData);
    if (sock_error != 0)
    {
        cout <<   "sock_error message: " << WSAGetLastError() << endl;
        cout <<   "init failed" << endl;
        return -1;
    }
    cout <<   "initialize WSA successfully" << endl;

    sender = socket(AF_INET, SOCK_DGRAM, 0);
    if (sender == INVALID_SOCKET)
    {
        cout <<   "bind sock_error" << endl;
        sock_error = WSACleanup();
        return -1;
    }

    SenderAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    SenderAddr.sin_family = AF_INET;
    SenderAddr.sin_port = htons(8887);
    sock_error = bind(sender, (SOCKADDR*)&SenderAddr, sizeof(SOCKADDR));
    if (sock_error != 0)
    {
        cout << "sock_error code " << WSAGetLastError() << endl;
        cout << "bind sock_error" << endl;
        sock_error = WSACleanup();
        return -1;
    }
    cout <<   "bind successfully" << endl;

    cout <<   "Please input receiver's ip address: ";
    cin >> input;
    int len = sizeof(SOCKADDR);
    RecvAddr.sin_addr.S_un.S_addr = inet_addr(input.c_str());
    RecvAddr.sin_family = AF_INET;
    cout <<   "Please input receiver port: ";
    cin >> input;
    RecvAddr.sin_port = htons(atoi(input.c_str()));

    cout <<   "The sender is sending SYN message ..." << endl;
    Data_length = 0;
    bufSend = geneDataGram(&SenderAddr, &RecvAddr, (char*)"", "");
    setSYN();
    bool InitCon = false;
    while (!InitCon)
    {
        sock_error = sendto(sender,bufSend,Gram_length,0,(SOCKADDR*)&RecvAddr,len);
        //cout<<sock_error<<"  "<<Gram_length<<endl;
        if (sock_error != Gram_length)
        {
            cout <<   "wait..." << endl;
            Cur_Resend++;
            if (Cur_Resend == Max_Resend)
            {
                cout <<   "failed to connect" << endl;
                cout <<   "The program is going to close ..." << endl;
                delete[] bufRecv;
                delete[] bufSend;
                return -1;
            }
            Sleep(1000);
            continue;
        }
        bufRecv = new char [MAX_PACLEN];
        sock_error = recvfrom(sender,bufRecv,MAX_PACLEN,0,(SOCKADDR*)&RecvAddr,&len);
        if (sock_error == SOCKET_ERROR || !checkACK() || !checkSYN())
        {
            delete[] bufRecv;
            cout <<   "wait..." << endl;
            Cur_Resend++;
            if (Cur_Resend == Max_Resend)
            {
                cout <<   "Sender failed to connect" << endl;
                delete[] bufRecv;
                delete[] bufSend;
                cout <<   "The program is going to close ... " << endl;
                return -1;
            }
            Sleep(1000);
            continue;
        }
        InitCon = true;
        delete[] bufRecv;
    }

    cout <<  "connect successfully" << endl;
    cout << "Notice: The max resend chance is 10 "<<endl;
    cout << "Notice: The max waiting time for sender is 2 seconds" << endl;

    // cout<<"Notice: Please input the send_window size: "<<endl;
    // cin>> swnd;

    handle = _findfirst(( File_Path + "*").c_str(), &info);
    if (handle == -1)
    {
        cout <<   "failed to open directory ... " << endl;
        return -1;
    }
    do {
        files.push_back(info.name);
    } while (!_findnext(handle, &info));
    
    Cur_Resend = 0;

    while (true) {
        cout << "There are the files existing in the path. " << endl;
        for (int i = 2; i < files.size(); i++)
            cout << "( "<<i - 2 << " ) " << files[i] << endl;
        cout <<   "You can input the num '-1' to quit" << endl;
        cout <<  "Please input the number of the file which you want to choose to send: ";
        cin >> input;
        while (input == "-1")
        {
            Data_length = 0;
            bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
            out = identifier;
            setFIN();
            sock_error = sendto(sender, bufSend, Gram_length, 0, (SOCKADDR*)&RecvAddr, len);
            bufRecv = new char[MAX_PACLEN];
            sock_error = recvfrom(sender, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&RecvAddr, &len);
            if (checkRecvFIN())       //发送方要检验接收方是否收到并回复正确信息
            {
                cout <<  "The program is going to shut down..." << endl;
                delete[] bufRecv;
                delete[] bufSend;
                system("pause");
                exit(0);
            }
        }

        fileStartTime=clock();
        int i = atoi(input.c_str()) + 2;
        cout <<   "send file " << files[i] << endl;
        ifstream is(( File_Path + files[i]).c_str(), ifstream::in | ios::binary);
        is.seekg(0, is.end);
        int File_length = is.tellg();
        is.seekg(0, is.beg);
        packetNum=File_length/MAX_PACLEN;
        is.close();
        Data_length = File_length;


        int packet_loc = 0, packet_length;
        cout<<"file_length is : "<<File_length<<endl;
        int start=0;       //用来给读入文件计数
        while(start<=packetNum)
        {
            packet_length = ((File_length - packet_loc <  MAX_PACLEN) ? File_length - packet_loc :  MAX_PACLEN);
            start++;
            ifstream is((File_Path + files[i]).c_str(), ifstream::in | ios::binary);
            is.seekg(packet_loc);
            bufSend = new char[packet_length];
            is.read(bufSend, packet_length);
            is.close();
            packet_loc += packet_length;
            Data_length = packet_length;
            bufSend = geneDataGram(&SenderAddr, &RecvAddr, bufSend, files[i]);
            identifier++;
            if (identifier==packetNum+1) setOVER();
            SendReserve.push_back(bufSend);
        }

        cout <<   "total packages: " << SendReserve.size() << endl;
        //Sleep(3000);
        cout << "After all data is packed into the queue, identifier is: "<< identifier << endl;
        //Sleep(2000);
        identifier = 0;
        FileSent = false;
        startTime = clock();
        recver = CreateThread(NULL, NULL, RecvThread, (LPVOID)sender, NULL, NULL);
        for (int i = 0; i < SendReserve.size(); i++)
        {
            bool isSend = false;
            bufSend = SendReserve[i];
            while (!isSend)
            {
                if (identifier >= SendBase + cwnd)
                {
                    cout << "********    Flow control ... Preparing to resend    *********" << endl;
                    Sleep(1000);
                    continue;
                }

                //这里调整线程优先级 (线程真的好难用)
                //这个把当前线程置为最高优先级
                SetPriorityClass(GetCurrentThread(), HIGH_PRIORITY_CLASS);
                Gram_length = (int)((USHORT)((unsigned char)bufSend[4] << 8)+(USHORT)(unsigned char)bufSend[5]);
                Gram_length = (Gram_length % 2 == 0 ? Gram_length : Gram_length + 1);
                sock_error = sendto(sender,bufSend,Gram_length,0,(SOCKADDR*)&RecvAddr,sizeof(SOCKADDR));
                cout<<"now send ID: "<<identifier<<endl;
                //把当前线程和其他线程再次并行化
                SetPriorityClass(GetCurrentThread(), NORMAL_PRIORITY_CLASS);
                if (SendBase == identifier)
                {
                    PreTickTime = false;
                    ticktime = CreateThread(NULL, NULL, TimeCheck, (LPVOID)sender, NULL, NULL);
                }
                identifier = (identifier + 1) % 256;
                isSend = true;
            }
        }
        while (!FileSent) {}     // 判断当前文件是不是已经发出去了
        endTime = clock();
        fileEndTime=clock();
        cout <<   "Total time : " << ( fileEndTime -  fileStartTime) * 1.0 / 1000 << endl;
        cout <<   "Throughout Rate : " << File_length * 1.0 / ( fileEndTime -  fileStartTime) << endl;
        PreTickTime = true;
        FileSent = false;
        SendBase = 0;
        identifier = 0;
        state=0;
        ssthresh=8;
        cwnd=1;
        SendReserve.clear();
        Sleep(10);
    }

    sock_error = closesocket(sender);
    sock_error = WSACleanup();
    return 0;
}

