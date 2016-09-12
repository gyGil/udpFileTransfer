/*----------------------------------------------------------------------------------*/
/*Program:			udpComms Header													*/
/*Programmers:		Geun Young Gil & Marcus Rankin									*/
/*Filename:			udpComms.h														*/
/*Description:		This header has all the includes and defines for the UDP Comms  */
/*					verification application to function.										*/
/*----------------------------------------------------------------------------------*/

/// \class udpComms.h
///
/// \brief This file contains all includes, defines and prototypes for the UDP Communication
///  verification source file.

//====================================[HEADER FILES]====================================//	

#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <tchar.h>
#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <regex>
#include <cstring>
#include <iostream>
#include <exception>

#pragma comment(lib, "Ws2_32.lib")	//Winsock library
#pragma warning(disable: 4996)

using namespace net;

//====================================[GLOBAL CONSTANTS]====================================//	

#define CONESTOGA_PORT			1500	///< Connection port

#define MAX_CLIENTS				100		///< Maximum amount of clients (arbitrary amount)

#define IDC_EDIT_IN				101		///< Client/Operation Server message window
#define IDC_EDIT_OUT			102		///< Server data/results message window

#define IDC_MAIN_BUTTON			103		///< Broadcast message button
#define IDC_CRC_BUTTON			131

#define WM_SOCKET				104		///< Main socket communications

#define IDC_SERVER_BUTTON		105		///< Server select button
#define IDC_CLIENT_BUTTON		106		///< Client select button

#define IDC_IP_A_EDIT			119		///< IP first holder
#define IDC_IP_B_EDIT			120		///< IP second holder
#define IDC_IP_C_EDIT			121		///< IP third holder
#define IDC_IP_D_EDIT			122		///< IP fourth holder

#define IDC_PORT_EDIT			123		///< Port number holder

#define IDC_FILE_EDIT			124		///< File name for sending

#define INSERT					1		///< Insert new member information
#define UPDATE					2		///< Update previously added member information
#define FIND					3		///< Find previously added member information
#define MAX_CHARS_IN_LINE_DB	200		///< Maximum amount of characters per line in the database file

#define MAX_SIZE_BUFFER_INT		2600

#define MAX_EDIT_STR_SIZE		20		// maximum string size for edit boxes

#define HUNDRED_MB_FILE_SIZE	1000 * 1000 * 100	///< Max file size (100MB)

#define NON_BLOCKING_ERROR		10035	///< Normal UDP non-blocking error due to completed transfer (ignore)
#define CRC32_POLYNOMIAL     0xEDB88320L///< Used when calculating CRC values

#ifdef PKZIP_COMPATIBLE
#define CRC_FACTOR				0xFFFFFFFF
#else
#define CRC_FACTOR				0
#endif

typedef unsigned __int32 uint32_t;

//====================================[GLOBAL APPLICATION FUNCTIONS]====================================//	

bool IsIpAddr(const char* newInput)
{
	return std::regex_match(newInput, std::regex("^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$"));
}	///< check for ip address

void AppendText(const HWND &hwnd, const char io, TCHAR *newText);
void ServerThread(const HWND &hWnd);
void ClientThread(const HWND &hWnd, char* a_str, char* b_str, char* c_str, char* d_str, int ServerPort);
LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);	///< Windows callback process
void BuildCRCTable(void);
unsigned long CalculateBufferCRC(unsigned int count, unsigned long crc, void *buffer);
void Convert_UnsignedLong_UnsignedChar(int type, unsigned long* n, unsigned char* bytes);
bool SendPacketAppLayer(const unsigned char data[], int size, unsigned long orderNum, ReliableConnection* rConnection);
int ReceivePacketAppLayer(const HWND &hWnd, unsigned char data[], int size, ReliableConnection* rConnection);

//====================================[OLD CODE]====================================//	
int sender(char transportType, int blockSize, char* receiverAddr, int port);
int reciever(char transportType, int blockSize, int port);
int initSocket(void);
int cleanSocket(void);
int closeSocket(SOCKET sock);
SOCKET makeSocket(char transportType);
SOCKET checkSocketValid(SOCKET sock);

//====================================[END OLD CODE]====================================//	