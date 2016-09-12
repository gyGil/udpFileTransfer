/*----------------------------------------------------------------------------------------------
*Project:			Assignment 02 - UDP Communications Verification								
*Programmers:		Geun Young Gil & Marcus Rankin												
*References:		Ed Barsalou - Industrial Application Development (Semester 4)				
*								  Modules & References											
*					MSDN: https://msdn.microsoft.com												
*					GAFFER ON GAMES: http://gafferongames.com/networking-for-game-programmers/				
*					File Verification Using CRC: http://marknelson.us/1992/05/01/file-verification-using-crc-2/					
*Date:				February 20, 2016															
*Description:		This program creates a UDP communications network between 2 separate 
*					computers which checks/verifies message delivery and error checking 
*----------------------------------------------------------------------------------------------*/

/// \file udpComms.cpp
/// 
/// \brief 
/// - Program Name:	iad_A2_UDP_Verification
/// - This program creates the GUI and handles both the server side and 
///					client side of the UDP communications along with the message delivery
///					checking / verification.
/// - 
/// - This client make windows for communication with Server application
/// - It can send "add, update, find" requests to Server
#include "Net.h"
#include "FlowControl.h"
#include "udpComms.h"
#include <thread> 
#include <Windows.h>

//====================================================[ OLD CODE ]====================================================//


//====================================================[END OLD CODE ]====================================================//					

//=========================================[CONSTANTS]=========================================//
const int PROTOCOL_ID = 0x11223344;
const int PACKET_SIZE = 230;
const float TIMEOUT = 10.0f;
const float DELTA_TIME = 1.0f / 30.0f;
const float SEND_RATE = 1.0f / 30.0f;

//====================================[WIN32 GLOBAL VAR]====================================//	
HWND hInbox = NULL;								///< Server hInbox window handler
HWND hOutbox = NULL;							///< Server hOutbox window handler
HWND hIpAEdit = NULL;							///< First IP number holder
HWND hIpBEdit = NULL;							///< Second IP number holder
HWND hIpCEdit = NULL;							///< Third IP number holder
HWND hIpDEdit = NULL;							///< Fourth IP number holder
HWND hPortEdit = NULL;							///< Port number selector
HWND hFileEdit = NULL;							///< File name for sending

//===============================[DEVELOPER DEFINED STRUCTURES]===============================//
enum Mode
{
	Client,
	Server,
	None
};

//====================================[NETWORK GLOBAL VAR]====================================//	
Mode kMode = None;
ReliableConnection kConnection(PROTOCOL_ID, TIMEOUT);
FlowControl kFlowControl;
std::thread* k_thread_server;
std::thread* k_thread_client;
bool k_isExit = false;

// FILE INFO
bool k_isFileReadyToSend = false;
bool k_isGetAllFile = false;
unsigned char* k_buffer = NULL;
unsigned long k_buffer_size = 0;
unsigned long k_left_buffer_size = 0;
char k_fileName[100] = "";

// CRC
unsigned long CRCTable[256];

// SYSTEM
char systemStatus[UNICODE_STRING_MAX_BYTES] = "";	///< hInbox/hOutbox window messaging
char tempStatus[MAX_CHARS_IN_LINE_DB] = "";		///< hInbox/hOutbox temp window messaging holder
char tmp[MAX_CHARS_IN_LINE_DB] = { 0 };

//=================================== [TIMER SETUP] ===================================================//
double startTime = 0.0;	// start time
double endTime = 0.0;	// end time

LARGE_INTEGER tickPerSec;
LARGE_INTEGER st, et;	// start timer and end timer

//=================================== [DATA COLLECTION] ===================================================//
int totalDataReceived = 0;
int crcFails = 0;
bool k_generate_wrong_crc = false;


int WINAPI WinMain(HINSTANCE currProgInst, HINSTANCE prevProgInst, LPSTR lpCmdLine, int nShowCmd)
{
	// Setup the window class to display
	WNDCLASSEX wClass;
	ZeroMemory(&wClass, sizeof(WNDCLASSEX));
	wClass.cbClsExtra = NULL;
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.cbWndExtra = NULL;
	wClass.hbrBackground = CreateSolidBrush(0x00414748);
	wClass.hCursor = LoadCursor(NULL, IDC_HAND);
	wClass.hIcon = NULL;
	wClass.hIconSm = NULL;
	wClass.hInstance = currProgInst;
	wClass.lpfnWndProc = (WNDPROC)WinProc;
	wClass.lpszClassName = "Window Class";
	wClass.lpszMenuName = NULL;
	wClass.style = CS_HREDRAW | CS_VREDRAW;

	// Check that the window class was created successfully
	if (!RegisterClassEx(&wClass))
	{
		int nResult = GetLastError();
		MessageBox(NULL, "Window class creation failed", "Window Class Failed", MB_ICONERROR);
	}

	// Create main Server window
	HWND hWnd = CreateWindowEx(NULL, "Window Class", "UDP Verification (Version: 1.0.0.1)             Developed by: Geun Young Gil & Marcus Rankin", WS_OVERLAPPEDWINDOW,
		500,
		300,
		640,
		640,
		NULL, NULL, currProgInst, NULL);

	// Check that the Server window was created successfully
	if (!hWnd)
	{
		int nResult = GetLastError();

		MessageBox(NULL, "Window creation failed", "Window Creation Failed", MB_ICONERROR);
	}

	// Display the window
	ShowWindow(hWnd, nShowCmd);

	// Create message handler
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	// Setup timer: Currently set to save every 10 seconds
	//SetTimer(hWnd, IDT_TIMER1, 10000, (TIMERPROC)NULL);

	// Handle messages by translating and sending messages to the callback procedure
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}


/*----------------------------------------------------------------------------------
* Function:			Windows Process Call Back										
* Programmer:		Geun Young Gil & Marcus Rankin													
* References:		SET Relational Databases - Module 01 Win32						
* Description:		Main Server window for handling all client connections and		
*					communications.													
*----------------------------------------------------------------------------------*/

/// \details <b>Details</b> \n
/// Windows callback procedure for event handling. Including broadcast button, windows (hInbox, hOutbox) creation.
///	Socket creation/binding, connection handling, communication handling and database control.
/// \param hWnd - <b>HWND</b> - Main window handler
/// \param msg - <b>UINT</b> - Message handler
/// \param wParam - <b>WPARAM</b> - Message parameter: UINT_PTR
/// \param lParam - <b>LPARAM</b> - Message parameter: LONG_PTR
/// \return - <b>DefWindowProc</b> - Same parameters as the WinProc function: Ensures every message is processed.
///
LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_SERVER_BUTTON:
				{
					if (kMode == Client)
						MessageBox(hWnd, "You are already in \"Client Mode\"!", "Mode Error", MB_ICONERROR);

					else if (kMode == Server)
						MessageBox(hWnd, "You are already in \"Server Mode\"!", "Mode Error", MB_ICONERROR);

					else if (kMode == None)
					{
						kMode = Server;

						k_thread_server = new std::thread(ServerThread, hWnd);	
						
					}// end of else if (kMode == None)

					break;

				}// end of case IDC_SERVER_BUTTON:									

				case IDC_CRC_BUTTON:
					k_generate_wrong_crc = true;
					strcpy(systemStatus, "\r\n\r\nWrong CRC generated!");
					AppendText(hWnd, 'i', systemStatus);
					break;

				case IDC_CLIENT_BUTTON:
				{
					if (kMode == Client)
						MessageBox(hWnd, "You are already in \"Client Mode\"!", "Mode Error", MB_ICONERROR);

					else if (kMode == Server)
						MessageBox(hWnd, "You are already in \"Server Mode\"!", "Mode Error", MB_ICONERROR);

					else if (kMode == None)
					{
						kMode = Client;

						// get the port number
						char tmpStr[200] = "";
						int ServerPort = CONESTOGA_PORT;
						if (GetWindowText(hPortEdit, tmpStr, sizeof(tmpStr)) != NULL)
							ServerPort = atoi(tmpStr);

						// get ip address
						char a_str[4] = "";
						char b_str[4] = "";
						char c_str[4] = "";
						char d_str[4] = "";
						if (GetWindowText(hIpAEdit, a_str, sizeof(a_str)) == NULL ||
							GetWindowText(hIpBEdit, b_str, sizeof(b_str)) == NULL ||
							GetWindowText(hIpCEdit, c_str, sizeof(c_str)) == NULL ||
							GetWindowText(hIpDEdit, d_str, sizeof(d_str)) == NULL)
						{
							MessageBox(hWnd, "Invalid IP address!", "Critical Error", MB_ICONERROR);
							kMode = None;
							break;
						}

						// run client thread
						unsigned char* c = NULL;
						k_thread_client = new std::thread(ClientThread, hWnd, a_str, b_str, c_str, d_str, ServerPort);
					}
						

					break;
				
				}// end of case IDC_CLIENT_BUTTON:

				case IDC_MAIN_BUTTON:	// For sending message/file to server
				{						
					if (kMode != Client)
					{
						MessageBox(hWnd, "It is available for Client Mode!", "Access Error!", MB_ICONERROR);
						break;
					}


					long fileSize = 0;
					bool fileReady = false;
					FILE* filePtr;

					//Extract file for sending
					GetWindowText(hFileEdit, tmp, sizeof(tmp));
					char originalFileName[200] = "";
					strcpy(originalFileName, tmp);

					if (strcmp(tmp, "") != 0)
					{
						char* drivePtr = NULL;							///< Drive of file path check
						char dirPath[MAX_EDIT_STR_SIZE] = "C:\\";		///< Default drive check
						char genericPath[MAX_EDIT_STR_SIZE] = ":\\";	///< Generic drive check

						// Check if C: already specified
						if (strncmp(tmp, dirPath, 3) != 0)
						{	// Check if other drive location specified
							if ((drivePtr = strstr(tmp, genericPath)) == NULL)
							{
								// Default to C: if no drive specified
								strcat(dirPath, tmp);
								strcpy(tmp, dirPath);
							}
						}

						// Open file for binary reading
						filePtr = fopen(tmp, "rb");
						if (filePtr == NULL)	// Check if file exists in location specified
						{
							strcpy(systemStatus, "\r\nUnable to open file!");
							AppendText(hWnd, 'i', systemStatus);

							MessageBox(hWnd, "Unable to open/locate specified file", "File Access Error!", MB_ICONERROR);
							break;
						}

						strcpy(k_fileName, originalFileName);	// get file name

						// Determine file size
						fseek(filePtr, 0, SEEK_END);
						fileSize = ftell(filePtr);
						size_t readCheck = 0;

						// Check if file size is too large before sending
						if (fileSize > HUNDRED_MB_FILE_SIZE)
						{
							strcpy(systemStatus, "\r\n\r\nFile too large for sending! Unable to send file!");
							AppendText(hWnd, 'i', systemStatus);

							MessageBox(hWnd, "Unable to send specified file. File too large!", "File Size Error!", MB_ICONERROR);
							break;
						}

						// Set file pointer back to beginning
						fseek(filePtr, 0, SEEK_SET);

						// Allocate memory for file
						k_buffer = new unsigned char[fileSize + 1];
						memset(k_buffer, 0, sizeof(k_buffer));

						if (k_buffer == NULL)	// Check if memory was successfully allocated
						{
							strcpy(systemStatus, "\r\nFile is empty or does not exist!");
							AppendText(hWnd, 'i', systemStatus);
							fclose(filePtr);
							free(k_buffer);
							break;
						}
						
						// Copy file contents to allocated memory location
						readCheck = fread(k_buffer, sizeof(unsigned char), fileSize, filePtr);
						if (readCheck == fileSize)	// Check if all contents were copied
						{
							fileReady = true;
							sprintf(systemStatus, "\r\nComplete file read (Bytes: %d).", readCheck);
							AppendText(hWnd, 'i', systemStatus);
						}
						else if (readCheck == 0)	// Check if file was empty
						{
							strcpy(systemStatus, "\r\n\r\nFile is empty or corrupted!.");
							AppendText(hWnd, 'i', systemStatus);
						}
						else if (ferror(filePtr))
						{	// Check error if entire copy failed
							perror("fread");
							MessageBox(hWnd, "fread returned nothing!", "File Read Error!", MB_ICONERROR);
							break;
							
							sprintf(systemStatus, "\r\nComplete file NOT read (File Size Bytes: %d, Read Check Bytes: %d).", fileSize, readCheck);
							AppendText(hWnd, 'i', systemStatus);
						}
					}
					else
					{	// Check if File Path edit box was left empty and provide usage instructions
						MessageBox(hWnd, "Unable to find specified file!", "File Location Error!", MB_ICONERROR);
						strcpy(systemStatus, "\r\n\r\nFILE Usage: somefile.txt | anotherfile.doc (Cannot leave File name blank!)");
						AppendText(hWnd, 'i', systemStatus);
						break;
					}
					
					int size = 0;	// Amount of bytes that were sent

					if (fileReady)
					{
						k_left_buffer_size = k_buffer_size = (unsigned long)fileSize;

						k_isFileReadyToSend = true;
					}// end of "if (fileReady)"
					else
					{
						strcpy(systemStatus, "\r\n\r\nUnable to send file. Check for above error messages!");
						AppendText(hWnd, 'i', systemStatus);
					}

					// Close and clean up
					fclose(filePtr);
					//free(k_buffer);

					break;
				}// end of case IDC_MAIN_BUTTON:		
			}// end of case WM_COMMAND:
			break;
		
		case WM_PAINT:
		{
			// Display specific labels to edit boxes
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			SetBkMode(hdc, TRANSPARENT);				// set background color of TextOut to transparent.

			// write text to main window
			TextOut(hdc, 10, 20, "Type:", (int)_tcslen("Type:"));	// _tcslen returns size_t
			TextOut(hdc, 235, 20, "Port#:", (int)_tcslen("Port#:"));
			TextOut(hdc, 350, 20, "Server IP:", (int)_tcslen("Server IP :"));
			TextOut(hdc, 453, 24, ".", (int)_tcslen("."));
			TextOut(hdc, 493, 24, ".", (int)_tcslen("."));
			TextOut(hdc, 533, 24, ".", (int)_tcslen("."));
			TextOut(hdc, 17, 120, "File:", (int)_tcslen("File:"));
			EndPaint(hWnd, &ps);
		}
		break;

		case WM_CREATE:		// Create hInbox, hOutbox, broadcast button and Server socket
		{
			// Create hInbox for client messages
			hOutbox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE |
				ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
				50,
				160,
				520,
				420,
				hWnd, (HMENU)IDC_EDIT_OUT, GetModuleHandle(NULL), NULL);

			if (!hOutbox)
			{
				MessageBox(hWnd,
					"Unable to create Server hInbox!",
					"Error",
					MB_OK | MB_ICONERROR);
			}

			HGDIOBJ hfDefault = GetStockObject(DEFAULT_GUI_FONT);

			SendMessage(hOutbox, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

			// Update input message window
			strcpy(systemStatus, "Waiting to make connection...\r\n");
			AppendText(hWnd, 'i', systemStatus);

			// Create hOutbox for client messages
			hInbox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE |
				ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, //ES_AUTOHSCROLL,
				50,
				50,
				520,
				60,
				hWnd, (HMENU)IDC_EDIT_IN, GetModuleHandle(NULL), NULL);

			if (!hInbox)
			{
				MessageBox(hWnd,
					"Unable to create Server hOutbox!",
					"Error",
					MB_OK | MB_ICONERROR);
			}

			SendMessage(hInbox, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

			// Main Server button for sending messages to client
			HWND sendButton = NULL;
			sendButton = CreateWindowEx(NULL, "BUTTON", "SEND FILE", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				370,
				115,
				100,
				24,
				hWnd, (HMENU)IDC_MAIN_BUTTON, GetModuleHandle(NULL), NULL);

			SendMessage(sendButton, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

			// Main Server button for sending messages to client
			HWND crcButton = NULL;
			crcButton = CreateWindowEx(NULL, "BUTTON", "CRC CHECK", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				490,
				115,
				80,
				24,
				hWnd, (HMENU)IDC_CRC_BUTTON, GetModuleHandle(NULL), NULL);

			SendMessage(crcButton, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

			// Main Server button for sending messages to clients
			HWND serverButton = NULL;
			serverButton = CreateWindowEx(NULL, "BUTTON", "SERVER", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				50,
				15,
				75,
				24,
				hWnd, (HMENU)IDC_SERVER_BUTTON, GetModuleHandle(NULL), NULL);

			SendMessage(serverButton, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

			// Main Client button for sending messages to clients
			HWND clientButton = NULL;
			clientButton = CreateWindowEx(NULL, "BUTTON", "CLIENT", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				135,
				15,
				75,
				24,
				hWnd, (HMENU)IDC_CLIENT_BUTTON, GetModuleHandle(NULL), NULL);

			SendMessage(clientButton, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

			hIpAEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT, 420, 20, 30, 20,
				hWnd, (HMENU)IDC_IP_A_EDIT, GetModuleHandle(NULL), NULL);

			hIpBEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT, 460, 20, 30, 20,
				hWnd, (HMENU)IDC_IP_B_EDIT, GetModuleHandle(NULL), NULL);

			hIpCEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT, 500, 20, 30, 20,
				hWnd, (HMENU)IDC_IP_C_EDIT, GetModuleHandle(NULL), NULL);

			hIpDEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT, 540, 20, 30, 20,
				hWnd, (HMENU)IDC_IP_D_EDIT, GetModuleHandle(NULL), NULL);

			hPortEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT, 280, 20, 50, 20,
				hWnd, (HMENU)IDC_PORT_EDIT, GetModuleHandle(NULL), NULL);

			hFileEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT, 50, 115, 300, 24,
				hWnd, (HMENU)IDC_FILE_EDIT, GetModuleHandle(NULL), NULL);

			strcpy(systemStatus, "Winsock Initialization SUCCESS!\r\n\r\n");
			strcat(systemStatus, "CLIENT Usage: Set \"Port Number\" & \"IP Address\" before choosing \"Client\" connection.\r\n");
			strcat(systemStatus, "SERVER Usage: Set \"Port Number\" ONLY! before choosing \"SERVER\" connection.\r\n");
			AppendText(hWnd, 'i', systemStatus);
		}
		break;


		// Close and clean up the server socket before ending program
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			k_isExit = true;
			k_thread_server->join();
			k_thread_client->join();

			return 0;
		}
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}



void ServerThread(const HWND &hWnd)
{
	unsigned char buffer[100000];
	memset(buffer, 0, 100000);
	unsigned char* pBuffer = buffer;

	int tmp_counter = 0;
	int total_dataSize = 0;

	QueryPerformanceFrequency(&tickPerSec);		// Ticks per second

	// get the port number
	char tmpStr[200] = "";
	int ServerPort = CONESTOGA_PORT;
	if (GetWindowText(hPortEdit, tmpStr, sizeof(tmpStr)) != NULL)
		ServerPort = atoi(tmpStr);

	// initialize socket
	if (InitializeSockets() != 0)
	{
		MessageBox(hWnd, "Failed to initialize sockets!", "Critical Error", MB_ICONERROR);
		return;
	}

	// binding
	if (!kConnection.Start(ServerPort))
	{
		sprintf(tmpStr, "could not start connection on port %d!", ServerPort);
		MessageBox(hWnd, tmpStr, "Critical Error", MB_ICONERROR);
		return;
	}

	// listiening
	kConnection.Listen();

	MessageBox(hWnd, "Server is listening...", "Information", MB_ICONINFORMATION);

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;
	int waitCountAfterGetLastPacket = 0;

	while (true)
	{
		// exit
		if (k_isExit)
			break;

		// update flow control
		if (kConnection.IsConnected())
			kFlowControl.Update(DELTA_TIME, kConnection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = kFlowControl.GetSendRate();

		// detect changes in connection state
		if (kMode == Server && connected && !kConnection.IsConnected())
		{
			kFlowControl.Reset();
			sprintf(tmpStr, "\nreset flow control\n");
			AppendText(hWnd, 'i', tmpStr);
			connected = false;
		}

		if (!connected && kConnection.IsConnected())
		{
			sprintf(tmpStr, "\nclient connected to server\n");
			AppendText(hWnd, 'i', tmpStr);
			connected = true;
		}

		if (!connected && kConnection.ConnectFailed())
		{
			sprintf(tmpStr, "\nconnection failed\n");
			AppendText(hWnd, 'i', tmpStr);
			break;
		}

		sendAccumulator += DELTA_TIME;

		// send packet
		while (sendAccumulator > 1.0f / sendRate)
		{
			kConnection.SendPacket(NULL, 0);
			sendAccumulator -= 1.0f / sendRate;
		}

		
		// receive packet
		int i = 0;
		while (true)
		{
			unsigned char packet[PACKET_SIZE];

			int bytes_read = ReceivePacketAppLayer(hWnd, packet, PACKET_SIZE, &kConnection);

			if (bytes_read == 0)
				break;
			else
				totalDataReceived += bytes_read;// Total up received data


			if (++i > 3) break;
		}

		// after get all file
		if (k_isGetAllFile)
		{
			if (++waitCountAfterGetLastPacket > 23)	// wait for a while
			{
				// Stop timer at end of file transmission
				QueryPerformanceCounter(&et);
				endTime = (et.QuadPart * 1000.0) / tickPerSec.QuadPart;

				// write file
				try{
					char path[200] = "c:\\";
					strcat(path, k_fileName);
					std::ofstream outfile(path, std::ofstream::binary);
					outfile.write((char *)k_buffer, k_buffer_size);
					outfile.close();
				}
				catch (std::exception e)
				{
					MessageBox(hWnd, e.what(), "Error", MB_ICONERROR);
				}
				
				MessageBox(hWnd, "file saved", "Information", MB_ICONINFORMATION);

				// Update output message window with file transfer results
				strcpy(systemStatus, "\r\n\r\n============================ Result ============================\r\n\r\n");
				sprintf(tempStatus, "- Total Elapsed time in milliseconds is: %.2f\r\n\r\n", endTime - startTime);
				strcat(systemStatus, tempStatus);
				sprintf(tempStatus, "- Speed of transmission = %.2f (KBps) \r\n\r\n", totalDataReceived / (endTime - startTime));
				strcat(systemStatus, tempStatus);
				sprintf(tempStatus, "- Total received data = %d KBytes\r\n\r\n", totalDataReceived / 1000);
				strcat(systemStatus, tempStatus);
				sprintf(tempStatus, "- Amount of packets that failed redundancy check (CRC) = %d Packets\r\n\r\n", crcFails);
				strcat(systemStatus, tempStatus);

				strcat(systemStatus, "\r\n============================   End  ============================\r\n");
				AppendText(hWnd, 'o', systemStatus);

				// Reset timer
				startTime = 0.0;
				endTime = 0.0;
				totalDataReceived = 0;

				// free resouce 
				waitCountAfterGetLastPacket = 0;
				k_isGetAllFile = false;
				delete k_buffer;
				k_buffer = NULL;
				k_buffer_size = k_left_buffer_size = 0;
				memset(k_fileName,0,100);
			}
		}

		// update connection
		kConnection.Update(DELTA_TIME);

		net::wait(DELTA_TIME);
	}// end of while(true)

	ShutdownSockets();
}

void ClientThread(const HWND &hWnd, char* a_str, char* b_str, char* c_str, char* d_str, int ServerPort)
{
	int a, b, c, d;
	char tmpStr[200] = "";

	a = atoi(a_str);
	b = atoi(b_str);
	c = atoi(c_str);
	d = atoi(d_str);

	Address address = Address(a, b, c, d, ServerPort);

	// initialize socket
	if (InitializeSockets() != 0)
	{
		MessageBox(hWnd, "Failed to initialize sockets!", "Critical Error", MB_ICONERROR);
		return;
	}

	// binding
	if (!kConnection.Start(ServerPort))
	{
		sprintf(tmpStr, "could not start connection on port %d!", ServerPort);
		MessageBox(hWnd, tmpStr, "Critical Error", MB_ICONERROR);
		return;
	}

	// connect to server
	kConnection.Connect(address);

	sprintf(tmpStr, "\nAttempting to connect with Server....\n");
	AppendText(hWnd, 'i', tmpStr);
	
	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	int filePacketCnt = 0;

	while (true)
	{
		if (k_isExit)
			break;

		// update flow control
		if (kConnection.IsConnected())
			kFlowControl.Update(DELTA_TIME, kConnection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = kFlowControl.GetSendRate();

		// detect changes in connection state
		if (kMode == Server && connected && !kConnection.IsConnected())
		{
			kFlowControl.Reset();
			sprintf(tmpStr, "\nreset flow control\n");
			AppendText(hWnd, 'i', tmpStr);
			connected = false;
		}

		if (!connected && kConnection.IsConnected())
		{
			MessageBox(hWnd, "client connected to server", "Information", MB_ICONINFORMATION);
			connected = true;
		}

		if (!connected && kConnection.ConnectFailed())
		{
			MessageBox(hWnd, "connection failed", "Error", MB_ICONERROR);
			break;
		}

		sendAccumulator += DELTA_TIME;

		// send packet
		while (sendAccumulator > 1.0f / sendRate)
		{
			// send a file 
			if (k_isFileReadyToSend)
			{					
				unsigned char* nextPos = k_buffer + k_buffer_size - k_left_buffer_size;
				
				// send first packet
				if (nextPos == k_buffer)
				{
					filePacketCnt = 1;
					SendPacketAppLayer(NULL, 0, filePacketCnt, &kConnection);
				}
					
				// send the last packet
				if (k_left_buffer_size < PACKET_SIZE)
				{
					SendPacketAppLayer(nextPos, k_left_buffer_size, ++filePacketCnt, &kConnection);

					// free resource for a file buffer
					delete k_buffer;
					k_buffer_size = k_left_buffer_size = 0;					
					k_isFileReadyToSend = false;
					filePacketCnt = 0;

				}
				else   //  send packets
				{
					SendPacketAppLayer(nextPos, PACKET_SIZE, ++filePacketCnt, &kConnection);
					k_left_buffer_size -= PACKET_SIZE;
				}				
			}
			else    
				kConnection.SendPacket(NULL, 0);

			sendAccumulator -= 1.0f / sendRate;
		}

		// receive packet
		while (true)
		{
			unsigned char packet[PACKET_SIZE];
			int bytes_read = ReceivePacketAppLayer(hWnd, packet, PACKET_SIZE, &kConnection);
			if (bytes_read == 0)	
				break;
		}

		// update connection
		kConnection.Update(DELTA_TIME);

		net::wait(DELTA_TIME);
	}// end of while(true)

	ShutdownSockets();
}


/// orderNum = 1 :  first send(client -> server), 0: terminate transmission(server -> client), 1 ~ max : normal order number
bool SendPacketAppLayer(const unsigned char data[], int size, unsigned long orderNum, ReliableConnection* rConnection)
{
	unsigned char* packet = NULL;

	if (orderNum == 0)		// terminate transmission(server -> client)
	{
		size = 0;
		packet = new unsigned char[size + 8];
		Convert_UnsignedLong_UnsignedChar(1, &orderNum, packet);
	}
	else if (orderNum == 1)	// first send(client -> server)
	{
		size = 104;		// fileName (char[100]), total file size (unsigned long)
		packet = new unsigned char[size + 8];
		Convert_UnsignedLong_UnsignedChar(1, &orderNum, packet);
		Convert_UnsignedLong_UnsignedChar(1, &k_buffer_size, packet + 4);	// file size
		memcpy(&packet[8], k_fileName, sizeof(k_fileName));	// file name
	}
	else	// normal
	{
		packet = new unsigned char[size + 8];
		Convert_UnsignedLong_UnsignedChar(1, &orderNum, packet);
		memcpy(&packet[4], data, size);
	}

	if (packet == NULL) return false;

	unsigned long crc = CalculateBufferCRC((unsigned long)(size + 4), CRC_FACTOR, packet);	// get CRC value

	if (k_generate_wrong_crc)
	{
		crc += 10;
		k_generate_wrong_crc = false;
	}

	Convert_UnsignedLong_UnsignedChar(1, &crc, packet + size + 4);	// file size
	
	bool ret = rConnection->SendPacket(packet, size + 8);
	delete packet;

	return ret;
}

int ReceivePacketAppLayer(const HWND &hWnd, unsigned char data[], int size, ReliableConnection* rConnection)
{
	unsigned char* packet = new unsigned char[size + 8];
	int bytes_read = rConnection->ReceivePacket(packet, size + 8);

	
	// check complete header
	if (bytes_read < 8)
	{
		delete packet;
		return 0;
	}

	// check CRC
	unsigned long received_crc = 0;
	Convert_UnsignedLong_UnsignedChar(2, &received_crc, &packet[bytes_read - 4]);
	if (received_crc != CalculateBufferCRC(bytes_read - 4, CRC_FACTOR, packet))
	{
		MessageBox(hWnd, "Detects the wrong CRC value!", "Critical File transmission Error", MB_ICONERROR);
		++crcFails;	// Keep track of CRC compare fails
		delete packet;
		return 0;
	}

	// get order number
	unsigned long orderNum = 0;
	Convert_UnsignedLong_UnsignedChar(2, &orderNum, &packet[0]);

	if (orderNum == 0)		// terminate transmission(server -> client)
	{
		MessageBox(hWnd, "Completed a file transmission,", "Information", MB_ICONINFORMATION);
		return 0;
	}
	else if (orderNum == 1)	// first send(client -> server)
	{
		// make buffer to get whole file
		if (k_buffer != NULL) delete k_buffer;
		Convert_UnsignedLong_UnsignedChar(2, &k_buffer_size, &packet[4]);
		k_left_buffer_size = k_buffer_size;
		k_buffer = new unsigned char[k_buffer_size + 1];

		// get fileName
		strcpy(k_fileName, (char*)&packet[8]);

		// Start timer
		QueryPerformanceCounter(&st);	// Start timer
		startTime = (st.QuadPart * 1000.0) / tickPerSec.QuadPart;	// start time in milliseconds

		delete packet;
		return 0;
	}

	memcpy(&k_buffer[(orderNum - 2) * (unsigned long)PACKET_SIZE], &packet[4], bytes_read - 8);	// copy content

	if (orderNum >= (k_buffer_size / PACKET_SIZE + 2))
		k_isGetAllFile = true;

	delete packet;
	return bytes_read - 8;	
}

// type : 1 - Unsigned Long to Unsigned Char
// type : 2 - Unsigned Char to Unsigned Long
void Convert_UnsignedLong_UnsignedChar(int type, unsigned long* n, unsigned char* bytes)
{
	// Unsigned Long to Unsigned Char
	if (type == 1)
	{
		// high -> low
		bytes[0] = (*n >> 24) & 0xFF; 
		bytes[1] = (*n >> 16) & 0xFF;
		bytes[2] = (*n >> 8) & 0xFF;
		bytes[3] = *n & 0xFF;
	}
	// Unsigned Char to Unsigned Long
	else if (type == 2)
	{
		*n = bytes[3] | (bytes[2] << 8) | (bytes[1] << 16) | (bytes[0] << 24);
	}

}

/// \details <b>Details</b> \n
/// AppendText: This function retrieves text from the hInbox, appends any
/// new text and re-displays it into the Server hInbox.
/// \param hwnd - <b>HWND</b> - Window handler
/// \param newText - <b>TCHAR</b> - New message text to append
/// \return - <b>N/A</b> - N/A
///
void AppendText(const HWND &hwnd, const char io, TCHAR *newText)
{
	HWND hwndOutput;

	if (io == 'i')
	{
		// Get edit control from dialog
		hwndOutput = GetDlgItem(hwnd, IDC_EDIT_IN);
	}
	else if (io == 'o')
	{
		hwndOutput = GetDlgItem(hwnd, IDC_EDIT_OUT);
	}

	// Get the current selection
	DWORD StartPos, EndPos;
	SendMessage(hwndOutput, EM_GETSEL, reinterpret_cast<WPARAM>(&StartPos), reinterpret_cast<WPARAM>(&EndPos));

	// Move the caret to the end of the text
	int outLength = GetWindowTextLength(hwndOutput);
	SendMessage(hwndOutput, EM_SETSEL, outLength, outLength);

	// Insert the text at the new caret position
	SendMessage(hwndOutput, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(newText));

	// Restore the previous selection
	SendMessage(hwndOutput, EM_SETSEL, StartPos, EndPos);
}

/*
* void BuildCRCTable(void);
* Reference: http://marknelson.us/1992/05/01/file-verification-using-crc-2/
*
* Instead of performing a straightforward calculation of the 32 bit
* CRC using a series of logical operations, this program uses the
* faster table lookup method.This routine is called once when the
* program starts up to build the table which will be used later
* when calculating the CRC values.
*/
void BuildCRCTable(void)
{
	int i;
	int j;
	unsigned long crc;

	for (i = 0; i <= 255; i++) {
		crc = i;
		for (j = 8; j > 0; j--) {
			if (crc & 0x01)
				crc = (crc >> 0x01) ^ CRC32_POLYNOMIAL;
			else
				crc >>= 1;
		}
		CRCTable[i] = crc;
	}	/* end for */
}	/* end BuildCRCTable */

/*
* unsigned long CalculateBufferCRC (unsigned int count, unsigned long crc, void *buffer);
* Reference: http://marknelson.us/1992/05/01/file-verification-using-crc-2/
*
* This routine calculates the CRC for a block of data using the
* table lookup method. It accepts an original value for the crc,
* and returns the updated value. A block of any type of data can
* be passed in via the void* parameter. The parameter count defines
* how much data at this address we're allowed to process.
*/

unsigned long CalculateBufferCRC(unsigned int count, unsigned long crc, void *buffer)
{
	unsigned char *p;
	unsigned long temp1;
	unsigned long temp2;

	p = (unsigned char *)buffer;
	while (count-- != 0) {
		temp1 = (crc >> 8) & 0x00FFFFFFL;
		temp2 = CRCTable[((int)crc ^ *p++) & 0xff];
		crc = temp1 ^ temp2;
	}	/* end while */
	return crc;
}	/* end CalculateBufferCRC */