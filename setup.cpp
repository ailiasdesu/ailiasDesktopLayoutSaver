#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cstdio>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")

#define IDR_MAINEXE  101
#define IDR_UNINST   102
#define IDI_APPICON  1

static wchar_t g_selfDir[MAX_PATH],g_dest[MAX_PATH];

bool ExtractResource(int resId,const wchar_t* outPath,HWND hProg){
    HRSRC hrs=FindResourceW(NULL,MAKEINTRESOURCEW(resId),RT_RCDATA);
    if(!hrs)return false;
    HGLOBAL hg=LoadResource(NULL,hrs);
    DWORD sz=SizeofResource(NULL,hrs);
    void* data=LockResource(hg);
    if(!data)return false;
    SendMessageW(hProg,PBM_SETRANGE32,0,sz);
    HANDLE hf=CreateFileW(outPath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if(hf==INVALID_HANDLE_VALUE)return false;
    DWORD w;WriteFile(hf,data,sz,&w,NULL);
    CloseHandle(hf);
    SendMessageW(hProg,PBM_SETPOS,sz,0);
    return w==sz;
}

void CreateShortcut(const wchar_t* lnkPath,const wchar_t* target,const wchar_t* dir){
    IShellLinkW* sl;CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLinkW,(void**)&sl);
    sl->SetPath(target);sl->SetWorkingDirectory(dir);
    IPersistFile* pf;sl->QueryInterface(IID_IPersistFile,(void**)&pf);
    pf->Save(lnkPath,TRUE);pf->Release();sl->Release();
}

LRESULT CALLBACK WP(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
    case WM_CREATE:{
        CreateWindowExW(0,L"STATIC",L"ailiasDesktopLayoutSaver 安装程序",WS_CHILD|WS_VISIBLE,20,10,360,25,h,NULL,NULL,NULL);
        CreateWindowExW(0,L"STATIC",L"安装目录:",WS_CHILD|WS_VISIBLE,20,45,60,23,h,NULL,NULL,NULL);
        CreateWindowExW(0,L"EDIT",g_dest,WS_CHILD|WS_VISIBLE|WS_BORDER,85,43,200,23,h,(HMENU)1001,NULL,NULL);
        CreateWindowExW(0,L"BUTTON",L"浏览...",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,290,42,70,23,h,(HMENU)1005,NULL,NULL);
        CreateWindowExW(0,PROGRESS_CLASSW,L"",WS_CHILD|WS_VISIBLE,20,75,360,20,h,(HMENU)1002,NULL,NULL);
        CreateWindowExW(0,L"STATIC",L"就绪 - 点击安装开始",WS_CHILD|WS_VISIBLE,20,100,360,20,h,(HMENU)1003,NULL,NULL);
        CreateWindowExW(0,L"BUTTON",L"安装",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,150,130,100,32,h,(HMENU)1004,NULL,NULL);
        return 0;
    }
    case WM_COMMAND:
        if(LOWORD(w)==1005){
            BROWSEINFOW bi={};bi.hwndOwner=h;bi.lpszTitle=L"选择安装目录";
            bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl=SHBrowseForFolderW(&bi);
            if(pidl){wchar_t d[MAX_PATH];SHGetPathFromIDListW(pidl,d);
            SetWindowTextW(GetDlgItem(h,1001),d);CoTaskMemFree(pidl);}
            return 0;
        }
        if(LOWORD(w)==1004){
            HWND hD=GetDlgItem(h,1001),hP=GetDlgItem(h,1002),hS=GetDlgItem(h,1003);
            GetWindowTextW(hD,g_dest,MAX_PATH);CreateDirectoryW(g_dest,NULL);
            EnableWindow(hD,FALSE);EnableWindow(GetDlgItem(h,1004),FALSE);
            EnableWindow(GetDlgItem(h,1005),FALSE);
            wchar_t t[MAX_PATH];

            SetWindowTextW(hS,L"正在安装主程序...");
            swprintf_s(t,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver.exe",g_dest);
            if(!ExtractResource(IDR_MAINEXE,t,hP)){SetWindowTextW(hS,L"安装失败！");return 0;}
            SendMessageW(hP,PBM_SETPOS,0,0);

            SetWindowTextW(hS,L"正在安装卸载器...");
            swprintf_s(t,MAX_PATH,L"%s\\uninstall.exe",g_dest);
            ExtractResource(IDR_UNINST,t,hP);
            SendMessageW(hP,PBM_SETPOS,0,0);

            SetWindowTextW(hS,L"正在创建快捷方式...");
            swprintf_s(t,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver.exe",g_dest);
            wchar_t lk[MAX_PATH];
            SHGetFolderPathW(NULL,CSIDL_PROGRAMS,NULL,0,lk);
            swprintf_s(g_selfDir,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver",lk);
            CreateDirectoryW(g_selfDir,NULL);
            swprintf_s(g_selfDir,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver\\ailiasDesktopLayoutSaver.lnk",lk);
            CreateShortcut(g_selfDir,t,g_dest);
            SHGetFolderPathW(NULL,CSIDL_DESKTOP,NULL,0,lk);
            swprintf_s(g_selfDir,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver.lnk",lk);
            CreateShortcut(g_selfDir,t,g_dest);
            SendMessageW(hP,PBM_SETPOS,50,0);

            SetWindowTextW(hS,L"正在注册卸载信息...");
            HKEY hk;
            RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ailiasDesktopLayoutSaver",
                0,NULL,0,KEY_WRITE,NULL,&hk,NULL);
            swprintf_s(t,MAX_PATH,L"%s\\uninstall.exe",g_dest);
            RegSetValueExW(hk,L"DisplayName",0,REG_SZ,(BYTE*)L"ailiasDesktopLayoutSaver",48);
            RegSetValueExW(hk,L"UninstallString",0,REG_SZ,(BYTE*)t,(DWORD)(wcslen(t)*2+2));
            RegSetValueExW(hk,L"InstallLocation",0,REG_SZ,(BYTE*)g_dest,(DWORD)(wcslen(g_dest)*2+2));
            RegSetValueExW(hk,L"Publisher",0,REG_SZ,(BYTE*)L"ailias",14);
            DWORD one=1;RegSetValueExW(hk,L"NoModify",0,REG_DWORD,(BYTE*)&one,4);
            RegSetValueExW(hk,L"NoRepair",0,REG_DWORD,(BYTE*)&one,4);
            RegCloseKey(hk);
            SendMessageW(hP,PBM_SETPOS,70,0);

            SendMessageW(hP,PBM_SETPOS,100,0);
            SetWindowTextW(hS,L"安装完成！");
            MessageBoxW(h,L"ailiasDesktopLayoutSaver 安装完成！",L"安装完成",MB_OK|MB_ICONINFORMATION);
            DestroyWindow(h);return 0;
        }
        return 0;
    case WM_CLOSE:DestroyWindow(h);return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(h,m,w,l);
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int){
    GetModuleFileNameW(NULL,g_selfDir,MAX_PATH);
    wchar_t* bs=wcsrchr(g_selfDir,L'\\');if(bs)*bs=0;
    INITCOMMONCONTROLSEX icc={sizeof(icc),ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&icc);
    CoInitialize(NULL);
    SHGetFolderPathW(NULL,CSIDL_PROGRAM_FILES,NULL,0,g_dest);
    wcscat_s(g_dest,MAX_PATH,L"\\ailiasDesktopLayoutSaver");
    WNDCLASSEXW wc={sizeof(wc)};
    wc.lpfnWndProc=WP;wc.hInstance=hi;wc.hIcon=LoadIconW(hi,MAKEINTRESOURCEW(IDI_APPICON));
    wc.hCursor=LoadCursorW(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"DLSSetup";
    RegisterClassExW(&wc);
    CreateWindowExW(0,L"DLSSetup",L"ailiasDesktopLayoutSaver 安装",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_VISIBLE,
        CW_USEDEFAULT,CW_USEDEFAULT,420,210,NULL,NULL,hi,NULL);
    MSG msg;while(GetMessageW(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    CoUninitialize();
    return 0;
}
