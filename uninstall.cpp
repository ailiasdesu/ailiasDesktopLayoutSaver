#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cstdio>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
    wchar_t dir[MAX_PATH];DWORD sz=sizeof(dir);
    // Read install dir from registry
    if(RegGetValueW(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ailiasDesktopLayoutSaver",
        L"InstallLocation",RRF_RT_REG_SZ,NULL,dir,&sz)!=ERROR_SUCCESS){
        GetModuleFileNameW(NULL,dir,MAX_PATH);
        wchar_t* bs=wcsrchr(dir,L'\\');if(bs)*bs=0;
    }
    if(MessageBoxW(NULL,L"确定要卸载 ailiasDesktopLayoutSaver？\n\n存档数据将保留在 AppData 中。",
        L"ailiasDesktopLayoutSaver 卸载",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;

    // Kill running instance
    HWND hw=FindWindowW(L"DLS",NULL);if(hw)PostMessageW(hw,WM_CLOSE,0,0);
    Sleep(300);

    // Delete main exe
    wchar_t fp[MAX_PATH];swprintf_s(fp,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver.exe",dir);
    DeleteFileW(fp);

    // Remove Start Menu shortcut
    wchar_t sm[MAX_PATH];SHGetFolderPathW(NULL,CSIDL_PROGRAMS,NULL,0,sm);
    swprintf_s(fp,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver\\ailiasDesktopLayoutSaver.lnk",sm);
    DeleteFileW(fp);
    swprintf_s(fp,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver",sm);
    RemoveDirectoryW(fp);

    // Remove Desktop shortcut
    wchar_t dt[MAX_PATH];SHGetFolderPathW(NULL,CSIDL_DESKTOP,NULL,0,dt);
    swprintf_s(fp,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver.lnk",dt);
    DeleteFileW(fp);

    // Remove registry
    RegDeleteKeyW(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ailiasDesktopLayoutSaver");
    

    // Self-delete on reboot & remove dir
    wchar_t self[MAX_PATH];GetModuleFileNameW(NULL,self,MAX_PATH);
    MoveFileExW(self,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);
    RemoveDirectoryW(dir);

    MessageBoxW(NULL,L"卸载完成。\n\n存档数据保留在:\n%APPDATA%\\ailiasDesktopLayoutSaver",
        L"ailiasDesktopLayoutSaver",MB_OK|MB_ICONINFORMATION);
    return 0;
}
