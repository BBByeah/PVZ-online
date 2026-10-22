#include <time.h>
#include "TodDebug.h"
#include "TodCommon.h"
#include "misc/Debug.h"
#include "misc/SEHCatcher.h"
#include "../SexyAppFramework/SexyAppBase.h"

using namespace Sexy;

static char gLogFileName[MAX_PATH];
static char gDebugDataFolder[MAX_PATH];

// @pvz-online: 最近日志环形缓冲（2026-10-13 崩溃取证批）——崩溃时把尾部拼进
// crash.txt，让现场能回溯崩溃前发生了什么。写日志的线程不止一条（主线程 +
// 联机收发线程），这里不做加锁：最坏情况是某一行被撕裂，对诊断无害；换来的是
// 每条日志零额外成本。缓冲先写这里、后落盘——即使日志文件打不开（只读盘/
// 杀软锁），崩溃现场依然拿得到日志尾。
enum { RECENT_LOG_LINES = TOD_RECENT_LOG_LINES, RECENT_LOG_LINE_LEN = TOD_RECENT_LOG_LINE_LEN };
static char gRecentLog[RECENT_LOG_LINES][RECENT_LOG_LINE_LEN];
static int gRecentLogWrite = 0;	// 下一行写哪格
static int gRecentLogTotal = 0;	// 累计写入行数（>= LINES 即已绕圈）

static void TodRecentLogAppend(const char* theLine)
{
	char* aSlot = gRecentLog[gRecentLogWrite];
	strncpy(aSlot, theLine, RECENT_LOG_LINE_LEN - 1);
	aSlot[RECENT_LOG_LINE_LEN - 1] = '\0';
	gRecentLogWrite = (gRecentLogWrite + 1) % RECENT_LOG_LINES;
	gRecentLogTotal++;
}

// 把环形缓冲里现存的日志行按时间顺序（最老→最新）拷进 theBuffer（含行尾换行，
// 可能截断到放不下的上一行）。theBuffer 至多写 theBufferSize 字节，恒以 '\0' 结尾。
void TodGetRecentLog(char* theBuffer, int theBufferSize)
{
	if (theBuffer == nullptr || theBufferSize <= 0)
		return;
	theBuffer[0] = '\0';

	int aLines = (gRecentLogTotal < RECENT_LOG_LINES) ? gRecentLogTotal : RECENT_LOG_LINES;
	int aStart = (gRecentLogTotal < RECENT_LOG_LINES) ? 0 : gRecentLogWrite;	// 绕圈后写指针处即最老一行

	int aPos = 0;
	for (int i = 0; i < aLines; i++)
	{
		const char* aLine = gRecentLog[(aStart + i) % RECENT_LOG_LINES];
		int aLen = (int)strlen(aLine);
		if (aPos + aLen >= theBufferSize - 1)
			break;
		memcpy(theBuffer + aPos, aLine, aLen);
		aPos += aLen;
		theBuffer[aPos] = '\0';
	}
}

//0x514EA0
void TodErrorMessageBox(const char* theMessage, const char* theTitle)
{
	HWND hWnd = (gSexyAppBase && gSexyAppBase->mHWnd) ? gSexyAppBase->mHWnd : GetActiveWindow();
	TodTraceAndLog("%s.%s", theMessage, theTitle);
	MessageBoxA(hWnd, theMessage, theTitle, MB_ICONEXCLAMATION);
}

void TodTraceMemory()
{
}

void* TodMalloc(int theSize)
{
	TOD_ASSERT(theSize > 0);
	return malloc(theSize);
}

void TodFree(void* theBlock)
{
	if (theBlock != nullptr)
	{
		free(theBlock);
	}
}

void TodAssertFailed(const char* theCondition, const char* theFile, int theLine, const char* theMsg, ...)
{
	char aFormattedMsg[1024];
	va_list argList;
	va_start(argList, theMsg);
	int aCount = TodVsnprintf(aFormattedMsg, sizeof(aFormattedMsg), theMsg, argList);
	va_end(argList);

	if (aCount != 0) {
		if (aFormattedMsg[aCount - 1] != '\n')
		{
			if (aCount + 1 < 1024)
			{
				aFormattedMsg[aCount] = '\n';
				aFormattedMsg[aCount + 1] = '\0';
			}
			else
			{
				aFormattedMsg[aCount - 1] = '\n';
			}
		}
	}

	char aBuffer[1024];
	if (*theCondition != '\0')
	{
		TodSnprintf(aBuffer, sizeof(aBuffer), "\n%s(%d)\nassertion failed: '%s'\n%s\n", theFile, theLine, theCondition, aFormattedMsg);
	}
	else
	{
		TodSnprintf(aBuffer, sizeof(aBuffer), "\n%s(%d)\nassertion failed: %s\n", theFile, theLine, aFormattedMsg);
	}
	TodTrace("%s", aBuffer);

	if (!IsDebuggerPresent())
	{
		if (gInAssert)
		{
			TodLog("Assert during exception processing");
			exit(0);
		}


		gInAssert = true;
		LPEXCEPTION_POINTERS exp;

		__try
		{
			RaiseException(EXCEPTION_NONCONTINUABLE_EXCEPTION, NULL, NULL, NULL);
		}
		__except (exp = GetExceptionInformation(), EXCEPTION_CONTINUE_EXECUTION)
		{
			TodReportError(exp, aFormattedMsg);
		}

		gInAssert = false;
		exit(0);
	}
}

void TodLog(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aCount <= 0)	// @pvz-online: 空串保护——TodVsnprintf 只会在空输出时返回 0，别读 aButter[-1]
		return;

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	TodLogString(aButter);
}

void TodLogString(const char* theMsg)
{
	TodRecentLogAppend(theMsg);

	FILE* f = fopen(gLogFileName, "a");
	if (f == nullptr)
	{
		// @pvz-online: 关键修复（2026-10-13）——以前这里不 return，fwrite(nullptr)
		// 会触发 CRT 无效参数检查直接 FastFail：日志文件打不开的机器（ProgramData
		// 只读/杀软锁文件/磁盘满）任何一条日志都静默秒退，无 crash.txt 无对话框。
		OutputDebugString(_S("Failed to open log file\n"));
		return;
	}

	if (fwrite(theMsg, strlen(theMsg), 1, f) != 1)
	{
		OutputDebugString(_S("Failed to write to log file\n"));
	}

	fclose(f);
}

void TodTrace(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aCount <= 0)	// @pvz-online: 空串保护——TodVsnprintf 只会在空输出时返回 0，别读 aButter[-1]
		return;

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	OutputDebugStringA(aButter);
	// @pvz-online: mirror traces to stderr only when PVZ_TRACE is set, so a redirected
	// run can capture them on demand without every normal run paying for the write.
	static int sMirrorToStderr = -1;
	if (sMirrorToStderr < 0)
		sMirrorToStderr = (GetEnvironmentVariableA("PVZ_TRACE", nullptr, 0) > 0) ? 1 : 0;
	if (sMirrorToStderr == 1)
	{
		fputs(aButter, stderr);
		fflush(stderr);
	}
}

void TodHesitationTrace(...)
{
}

void TodTraceAndLog(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aCount <= 0)	// @pvz-online: 空串保护——TodVsnprintf 只会在空输出时返回 0，别读 aButter[-1]
		return;

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	OutputDebugStringA(aButter);
	TodLogString(aButter);
}

void TodTraceWithoutSpamming(const char* theFormat, ...)
{
	static __time64_t gLastTraceTime = 0i64;
	__time64_t aTime = _time64(nullptr);
	if (aTime <= gLastTraceTime)
	{
		return;
	}

	gLastTraceTime = aTime;
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aCount <= 0)	// @pvz-online: 空串保护——TodVsnprintf 只会在空输出时返回 0，别读 aButter[-1]
		return;

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	OutputDebugStringA(aButter);
}

void TodReportError(LPEXCEPTION_POINTERS exceptioninfo, const char* theMessage)
{
	(void)theMessage;
	Sexy::SEHCatcher::UnhandledExceptionFilter(exceptioninfo);
}

long __stdcall TodUnhandledExceptionFilter(LPEXCEPTION_POINTERS exceptioninfo)
{
	if (gInAssert)
	{
		TodLog("Exception during exception processing");
	}
	else
	{
		gInAssert = true;
		TodLog("\nUnhandled Exception");
		TodReportError(exceptioninfo, "Unhandled Exception");
		gInAssert = false;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void TodAssertInitForApp()
{
	MkDir(GetAppDataFolder() + "userdata");
	std::string aRelativeUserPath = GetAppDataFolder() + "userdata\\";
	strcpy(gDebugDataFolder, GetFullPath(aRelativeUserPath).c_str());
	strcpy(gLogFileName, gDebugDataFolder);
	strcpy(gLogFileName + strlen(gLogFileName), "log.txt");
	TOD_ASSERT(strlen(gLogFileName) < MAX_PATH);

	__time64_t aclock = _time64(nullptr);
	TodLog("Started %s\n", asctime(_localtime64(&aclock)));

	SetUnhandledExceptionFilter(TodUnhandledExceptionFilter);
}
