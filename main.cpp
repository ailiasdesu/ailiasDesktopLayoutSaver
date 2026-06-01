// DesktopLayoutSaver - Saves Bags\1\Desktop + Streams\Desktop
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <cstdio>
#include <cstring>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

static const wchar_t* g_keys[]={L"Software\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop",L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Streams\\Desktop"};
static const int NK=2,SLOTS=20,NMAX=32;
static const DWORD MAGIC=0x52454743;
static wchar_t g_dir[MAX_PATH],g_nm[SLOTS][NMAX];
static int g_pri=-1;static bool g_ok[SLOTS];
static HWND g_lv,g_st,g_bt,g_hWnd;static HBRUSH g_br=NULL,g_bgBr=NULL;static bool g_dk=false;
static NOTIFYICONDATAW g_ni={};
#define WM_TRAY (WM_APP+100)
#define DK_BG RGB(32,32,32)
#define DK_FG RGB(240,240,240)
#define DK_HD RGB(43,43,43)
#define ID_LV 1
#define ID_HD 2
#define IDI_APPICON 1
enum{IDM_R=200,IDM_S=201,IDM_U=202,IDM_X=203};
enum{BTN_P=300,BTN_S=301,BTN_R=302,BTN_N=303,BTN_T=304};
enum{CTX_DEL=400,CTX_REN=401,CTX_PRI=402,CTX_RES=403};

void Rf();void UpPb();
void SS(const wchar_t* m){SetWindowTextW(g_st,m);}
bool IsDk(){DWORD v=1,s=4;HKEY hk;if(RegOpenKeyExW(HKEY_CURRENT_USER,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",0,KEY_READ,&hk)==0){RegQueryValueExW(hk,L"AppsUseLightTheme",NULL,NULL,(BYTE*)&v,&s);RegCloseKey(hk);}return v==0;}

void ApplyDarkToWindow(HWND h){
    BOOL b=TRUE;
    DwmSetWindowAttribute(h,20,&b,4);
    SetWindowTheme(h,L"DarkMode_Explorer",NULL);
    if(!g_br)g_br=CreateSolidBrush(DK_BG);
    if(!g_bgBr)g_bgBr=CreateSolidBrush(DK_BG);
    SetClassLongPtrW(h,GCLP_HBRBACKGROUND,(LONG_PTR)g_br);
    SetWindowTheme(g_lv,L"DarkMode_Explorer",NULL);
    ListView_SetBkColor(g_lv,DK_BG);
    ListView_SetTextBkColor(g_lv,DK_BG);
    ListView_SetTextColor(g_lv,DK_FG);
    EnumChildWindows(h,[](HWND c,LPARAM){
        wchar_t cn[32];GetClassNameW(c,cn,32);
        if(lstrcmpW(cn,L"Button")==0||lstrcmpW(cn,L"Static")==0)
            SetWindowTheme(c,L"DarkMode_Explorer",NULL);
        return TRUE;
    },0);
    InvalidateRect(h,NULL,TRUE);
}

void LdCfg(){wchar_t p[MAX_PATH];swprintf_s(p,MAX_PATH,L"%s\\config.bin",g_dir);HANDLE h=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(h==INVALID_HANDLE_VALUE)return;DWORD r;ReadFile(h,&g_pri,sizeof(g_pri),&r,NULL);ReadFile(h,g_nm,sizeof(g_nm),&r,NULL);CloseHandle(h);for(int i=0;i<SLOTS;i++){wchar_t sp[MAX_PATH];swprintf_s(sp,MAX_PATH,L"%s\\slot_%02d.dat",g_dir,i);g_ok[i]=GetFileAttributesW(sp)!=INVALID_FILE_ATTRIBUTES;}}
void SvCfg(){wchar_t p[MAX_PATH];swprintf_s(p,MAX_PATH,L"%s\\config.bin",g_dir);HANDLE h=CreateFileW(p,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);if(h==INVALID_HANDLE_VALUE)return;DWORD w;WriteFile(h,&g_pri,sizeof(g_pri),&w,NULL);WriteFile(h,g_nm,sizeof(g_nm),&w,NULL);CloseHandle(h);}

DWORD FindExp(){HANDLE sn=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W pe={sizeof(pe)};DWORD pid=0;if(Process32FirstW(sn,&pe)){do{if(_wcsicmp(pe.szExeFile,L"explorer.exe")==0){pid=pe.th32ProcessID;break;}}while(Process32NextW(sn,&pe));}CloseHandle(sn);return pid;}

bool DoSave(int idx){
    DWORD totalSz=20;
    struct KData{int idx,cnt,total;}kd[NK]={};
    for(int ki=0;ki<NK;ki++){
        HKEY hk;if(RegOpenKeyExW(HKEY_CURRENT_USER,g_keys[ki],0,KEY_READ,&hk)!=0){kd[ki].cnt=0;continue;}
        kd[ki].idx=ki;
        for(;;){wchar_t nm[256];DWORD nl=256,tp=0,ds=0;if(RegEnumValueW(hk,kd[ki].cnt,nm,&nl,NULL,&tp,NULL,&ds)!=0)break;if(nl>=256)nl=255;kd[ki].total+=4+nl*2+4+4+ds;kd[ki].cnt++;}
        RegCloseKey(hk);
        if(kd[ki].cnt>0){totalSz+=8+kd[ki].total;}
    }
    BYTE* buf=(BYTE*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,totalSz);if(!buf)return false;
    DWORD* hdr=(DWORD*)buf;hdr[0]=MAGIC;hdr[1]=8;hdr[2]=GetSystemMetrics(SM_CXSCREEN);hdr[3]=GetSystemMetrics(SM_CYSCREEN);hdr[4]=NK;
    BYTE* cur=buf+20;
    for(int ki=0;ki<NK;ki++){
        if(kd[ki].cnt==0){*(DWORD*)cur=(DWORD)ki;cur+=4;*(DWORD*)cur=0;cur+=4;continue;}
        *(DWORD*)cur=(DWORD)ki;cur+=4;*(DWORD*)cur=(DWORD)kd[ki].cnt;cur+=4;
        HKEY hk;RegOpenKeyExW(HKEY_CURRENT_USER,g_keys[ki],0,KEY_READ,&hk);
        for(DWORD i=0;i<(DWORD)kd[ki].cnt;i++){wchar_t nm[256];DWORD nl=256,tp=0,ds=0;RegEnumValueW(hk,i,nm,&nl,NULL,&tp,NULL,&ds);if(nl>=256)nl=255;nm[nl]=0;*(DWORD*)cur=nl;cur+=4;memcpy(cur,nm,nl*2);cur+=nl*2;*(DWORD*)cur=tp;cur+=4;*(DWORD*)cur=ds;cur+=4;if(ds>0){RegQueryValueExW(hk,nm,NULL,NULL,cur,&ds);cur+=ds;}}
        RegCloseKey(hk);
    }
    wchar_t fp[MAX_PATH];swprintf_s(fp,MAX_PATH,L"%s\\slot_%02d.dat",g_dir,idx);
    HANDLE fh=CreateFileW(fp,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);if(fh==INVALID_HANDLE_VALUE){HeapFree(GetProcessHeap(),0,buf);return false;}
    DWORD wr;BOOL ok=WriteFile(fh,buf,(DWORD)(cur-buf),&wr,NULL);CloseHandle(fh);HeapFree(GetProcessHeap(),0,buf);
    if(!ok)return false;
    g_ok[idx]=true;if(!g_nm[idx][0])swprintf_s(g_nm[idx],NMAX,L"存档 %d",idx+1);SvCfg();return true;
}

bool DoRestore(int idx){
    if(!g_ok[idx])return false;
    wchar_t fp[MAX_PATH];swprintf_s(fp,MAX_PATH,L"%s\\slot_%02d.dat",g_dir,idx);
    HANDLE fh=CreateFileW(fp,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(fh==INVALID_HANDLE_VALUE)return false;
    DWORD fsz=GetFileSize(fh,NULL);if(fsz<20){CloseHandle(fh);return false;}
    BYTE* buf=(BYTE*)HeapAlloc(GetProcessHeap(),0,fsz);if(!buf){CloseHandle(fh);return false;}
    DWORD r;ReadFile(fh,buf,fsz,&r,NULL);CloseHandle(fh);
    DWORD* hdr=(DWORD*)buf;if(hdr[0]!=MAGIC){HeapFree(GetProcessHeap(),0,buf);return false;}
    DWORD nk=hdr[4];
    for(int tries=0;tries<10;tries++){
        DWORD pid=FindExp();if(!pid)break;
        HANDLE hp=OpenProcess(PROCESS_TERMINATE|SYNCHRONIZE,FALSE,pid);
        if(hp){TerminateProcess(hp,0);WaitForSingleObject(hp,2000);CloseHandle(hp);}
        Sleep(100);
    }
    BYTE* cur=buf+20;
    for(DWORD ki=0;ki<nk;ki++){
        if((BYTE*)cur-buf+8>fsz)break;
        DWORD kidx=*(DWORD*)cur;cur+=4;DWORD vcnt=*(DWORD*)cur;cur+=4;
        if(kidx>=NK)continue;
        HKEY hk;DWORD disp;
        if(RegCreateKeyExW(HKEY_CURRENT_USER,g_keys[kidx],0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_SET_VALUE,NULL,&hk,&disp)!=0)continue;
        for(;;){wchar_t dn[256];DWORD dnl=256;if(RegEnumValueW(hk,0,dn,&dnl,NULL,NULL,NULL,NULL)!=0)break;RegDeleteValueW(hk,dn);}
        for(DWORD i=0;i<vcnt;i++){
            if((BYTE*)cur-buf+4>fsz)break;DWORD nl=*(DWORD*)cur;cur+=4;if(nl>=256)nl=255;
            if((BYTE*)cur-buf+nl*2>fsz)break;wchar_t nm[256];memcpy(nm,cur,nl*2);nm[nl]=0;cur+=nl*2;
            if((BYTE*)cur-buf+8>fsz)break;DWORD tp=*(DWORD*)cur;cur+=4;DWORD ds=*(DWORD*)cur;cur+=4;
            if(ds>0){if((BYTE*)cur-buf+ds>fsz)break;RegSetValueExW(hk,nm,0,tp,cur,ds);cur+=ds;}
        }
        RegCloseKey(hk);
    }
    HeapFree(GetProcessHeap(),0,buf);
    RegFlushKey(HKEY_CURRENT_USER);
    STARTUPINFOW si={sizeof(si)};PROCESS_INFORMATION pi={};
    CreateProcessW(L"C:\\Windows\\explorer.exe",NULL,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
    CloseHandle(pi.hProcess);CloseHandle(pi.hThread);
    return true;
}

bool DoDelete(int idx){
    if(!g_ok[idx])return false;
    wchar_t fp[MAX_PATH];swprintf_s(fp,MAX_PATH,L"%s\\slot_%02d.dat",g_dir,idx);
    if(!DeleteFileW(fp))return false;
    g_ok[idx]=false;g_nm[idx][0]=0;
    if(g_pri==idx)g_pri=-1;
    SvCfg();return true;
}

void Rf(){ListView_DeleteAllItems(g_lv);for(int i=0;i<SLOTS;i++){wchar_t b[64];swprintf_s(b,64,L"%d",i+1);LVITEMW li={LVIF_TEXT,i};li.pszText=b;int p=ListView_InsertItem(g_lv,&li);ListView_SetItemText(g_lv,p,1,g_nm[i][0]?g_nm[i]:(LPWSTR)L"(空)");ListView_SetItemText(g_lv,p,2,(i==g_pri&&g_ok[i])?(LPWSTR)L"★ 主用":g_ok[i]?(LPWSTR)L"已保存":(LPWSTR)L"空");}}
void UpPb(){if(g_pri>=0&&g_ok[g_pri]){wchar_t b[128];swprintf_s(b,128,L"★ 一键恢复 (%s)",g_nm[g_pri]);SetWindowTextW(g_bt,b);}else SetWindowTextW(g_bt,L"★ 一键恢复主用布局 (未设置)");}

void ShowCtxMenu(HWND h,int idx,int x,int y){
    HMENU mu=CreatePopupMenu();
    if(g_ok[idx]){AppendMenuW(mu,MF_STRING,CTX_RES,L"恢复此布局");AppendMenuW(mu,MF_SEPARATOR,0,NULL);}
    AppendMenuW(mu,MF_STRING,CTX_REN,L"重命名");
    if(idx!=g_pri&&g_ok[idx])AppendMenuW(mu,MF_STRING,CTX_PRI,L"设为主用");
    if(g_ok[idx]){AppendMenuW(mu,MF_SEPARATOR,0,NULL);AppendMenuW(mu,MF_STRING,CTX_DEL,L"删除");}
    TrackPopupMenu(mu,TPM_RIGHTBUTTON,x,y,0,h,NULL);
    DestroyMenu(mu);
}

// Subclassed header proc: draws dark header items manually 
// (SetWindowTheme on ListView does NOT darken the header in all Win10/11 builds)
LRESULT CALLBACK HdrSubclassProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam,UINT_PTR uid,DWORD_PTR){
    switch(msg){
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{
        PAINTSTRUCT ps;HDC dc=BeginPaint(hWnd,&ps);
        RECT rc;GetClientRect(hWnd,&rc);
        HBRUSH br=CreateSolidBrush(DK_HD);FillRect(dc,&rc,br);DeleteObject(br);
        int cnt=Header_GetItemCount(hWnd);
        for(int i=0;i<cnt;i++){
            RECT ir;if(!Header_GetItemRect(hWnd,i,&ir))continue;
            wchar_t buf[64]={};HDITEMW hi={};hi.mask=HDI_TEXT;hi.pszText=buf;hi.cchTextMax=64;
            Header_GetItem(hWnd,i,&hi);
            SetBkMode(dc,TRANSPARENT);SetTextColor(dc,DK_FG);
            SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));
            ir.left+=6;ir.right-=4;
            DrawTextW(dc,buf,-1,&ir,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
            HPEN pn=CreatePen(PS_SOLID,1,RGB(70,70,70));
            HPEN op=(HPEN)SelectObject(dc,pn);
            MoveToEx(dc,ir.right+3,ir.top+4,NULL);LineTo(dc,ir.right+3,ir.bottom-4);
            SelectObject(dc,op);DeleteObject(pn);
        }
        HPEN pn=CreatePen(PS_SOLID,1,RGB(70,70,70));
        HPEN op=(HPEN)SelectObject(dc,pn);
        MoveToEx(dc,rc.left,rc.bottom-1,NULL);LineTo(dc,rc.right,rc.bottom-1);
        SelectObject(dc,op);DeleteObject(pn);
        EndPaint(hWnd,&ps);return 0;
    }
    case WM_NCDESTROY:RemoveWindowSubclass(hWnd,HdrSubclassProc,uid);break;
    }
    return DefSubclassProc(hWnd,msg,wParam,lParam);
}

LRESULT CALLBACK WP(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
    case WM_CREATE:{
        g_lv=CreateWindowExW(0,WC_LISTVIEWW,NULL,WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SINGLESEL|LVS_EDITLABELS,10,50,580,390,h,(HMENU)ID_LV,GetModuleHandleW(NULL),NULL);
        ListView_SetExtendedListViewStyle(g_lv,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_HEADERDRAGDROP);
        LVCOLUMNW c={LVCF_WIDTH|LVCF_TEXT};c.cx=50;c.pszText=(LPWSTR)L"槽位";ListView_InsertColumn(g_lv,0,&c);c.cx=330;c.pszText=(LPWSTR)L"名称";ListView_InsertColumn(g_lv,1,&c);c.cx=194;c.pszText=(LPWSTR)L"状态";ListView_InsertColumn(g_lv,2,&c);
        HWND hdr=ListView_GetHeader(g_lv);if(hdr)SetWindowSubclass(hdr,HdrSubclassProc,0,0);
        g_bt=CreateWindowExW(0,L"BUTTON",L"★ 一键恢复主用布局",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,10,10,580,32,h,(HMENU)BTN_P,GetModuleHandleW(NULL),NULL);
        CreateWindowExW(0,L"BUTTON",L"保存当前",WS_CHILD|WS_VISIBLE,10,450,100,28,h,(HMENU)BTN_S,GetModuleHandleW(NULL),NULL);
        CreateWindowExW(0,L"BUTTON",L"恢复选中",WS_CHILD|WS_VISIBLE,115,450,100,28,h,(HMENU)BTN_R,GetModuleHandleW(NULL),NULL);
        CreateWindowExW(0,L"BUTTON",L"重命名",WS_CHILD|WS_VISIBLE,220,450,80,28,h,(HMENU)BTN_N,GetModuleHandleW(NULL),NULL);
        CreateWindowExW(0,L"BUTTON",L"设为主用",WS_CHILD|WS_VISIBLE,305,450,80,28,h,(HMENU)BTN_T,GetModuleHandleW(NULL),NULL);
        g_st=CreateWindowExW(0,L"STATIC",L"就绪 | 右键槽位可操作 | 双击恢复 | 点击名称可重命名",WS_CHILD|WS_VISIBLE|SS_LEFTNOWORDWRAP,10,488,580,20,h,NULL,GetModuleHandleW(NULL),NULL);
        Rf();UpPb();
        ZeroMemory(&g_ni,sizeof(g_ni));g_ni.cbSize=sizeof(NOTIFYICONDATAW);g_ni.hWnd=h;g_ni.uID=1;g_ni.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;g_ni.uCallbackMessage=WM_TRAY;g_ni.hIcon=LoadIconW(NULL,IDI_APPLICATION);wcscpy_s(g_ni.szTip,_countof(g_ni.szTip),L"ailiasDesktopLayoutSaver");Shell_NotifyIconW(NIM_ADD,&g_ni);
        if(g_dk)ApplyDarkToWindow(h);
        return 0;
    }
    case WM_COMMAND:{int id=LOWORD(w),sel=ListView_GetNextItem(g_lv,-1,LVNI_SELECTED);switch(id){
        case BTN_P:if(g_pri>=0&&g_ok[g_pri]){if(DoRestore(g_pri))SS(L"★ 主用布局已恢复！");else SS(L"恢复失败");}else SS(L"未设置主用布局");break;
        case BTN_S:if(sel<0){SS(L"请先选择槽位");break;}if(DoSave(sel)){SS(L"保存成功！");Rf();}else SS(L"保存失败");break;
        case BTN_R:if(sel<0){SS(L"请先选择槽位");break;}if(!g_ok[sel]){SS(L"该槽位无数据");break;}if(DoRestore(sel))SS(L"恢复成功！");else SS(L"恢复失败");break;
        case BTN_N:if(sel<0||!g_ok[sel]){SS(L"请选中有数据的槽位");break;}SetFocus(g_lv);ListView_EditLabel(g_lv,sel);break;
        case BTN_T:if(sel<0||!g_ok[sel]){SS(L"请选中有数据的槽位");break;}g_pri=sel;SvCfg();Rf();UpPb();SS(L"已设为主用布局");break;
        case CTX_DEL:{int s=ListView_GetNextItem(g_lv,-1,LVNI_SELECTED);if(s>=0&&g_ok[s]){wchar_t mb[128];swprintf_s(mb,128,L"确定删除槽位 %d (%s) 吗？",s+1,g_nm[s]);if(MessageBoxW(h,mb,L"确认",MB_YESNO|MB_ICONQUESTION)==IDYES){if(DoDelete(s)){SS(L"已删除");Rf();UpPb();}else SS(L"删除失败");}}}break;
        case CTX_REN:{int s=ListView_GetNextItem(g_lv,-1,LVNI_SELECTED);if(s>=0&&g_ok[s]){SetFocus(g_lv);ListView_EditLabel(g_lv,s);}}break;
        case CTX_PRI:{int s=ListView_GetNextItem(g_lv,-1,LVNI_SELECTED);if(s>=0&&g_ok[s]){g_pri=s;SvCfg();Rf();UpPb();SS(L"已设为主用布局");}}break;
        case CTX_RES:{int s=ListView_GetNextItem(g_lv,-1,LVNI_SELECTED);if(s>=0&&g_ok[s]){if(DoRestore(s))SS(L"恢复成功！");else SS(L"恢复失败");}}break;
        case IDM_R:if(g_pri>=0&&g_ok[g_pri])DoRestore(g_pri);break;
        case IDM_S:ShowWindow(h,SW_RESTORE);SetForegroundWindow(h);break;
        case IDM_U:MessageBoxW(h,L"请通过控制面板卸载。",L"卸载",MB_ICONINFORMATION);break;
        case IDM_X:Shell_NotifyIconW(NIM_DELETE,&g_ni);DestroyWindow(h);break;
    }return 0;}
    case WM_NOTIFY:{
        NMHDR* nmh=(NMHDR*)l;
        if(nmh->idFrom==ID_LV){
            if(nmh->code==NM_CUSTOMDRAW){
                NMCUSTOMDRAW* cd=(NMCUSTOMDRAW*)l;
                if(cd->dwDrawStage==CDDS_PREPAINT){SetWindowLongPtrW(h,DWLP_MSGRESULT,CDRF_NOTIFYITEMDRAW);return TRUE;}
                if(cd->dwDrawStage==CDDS_ITEMPREPAINT&&g_dk){
                    RECT rc=cd->rc;HBRUSH br=CreateSolidBrush(DK_BG);FillRect(cd->hdc,&rc,br);DeleteObject(br);
                    SetWindowLongPtrW(h,DWLP_MSGRESULT,CDRF_NEWFONT);
                    return TRUE;
                }
            } else if(nmh->code==LVN_ENDLABELEDITW){
                NMLVDISPINFOW* di=(NMLVDISPINFOW*)l;
                if(di->item.pszText&&di->item.pszText[0]){int idx=di->item.iItem;if(idx>=0&&idx<SLOTS){int len=0;while(di->item.pszText[len]&&len<NMAX-1)len++;memcpy(g_nm[idx],di->item.pszText,len*2);g_nm[idx][len]=0;SvCfg();Rf();UpPb();SS(L"已重命名");}}
            } else if(nmh->code==NM_DBLCLK){int s=ListView_GetNextItem(g_lv,-1,LVNI_SELECTED);if(s>=0&&g_ok[s]){if(DoRestore(s))SS(L"恢复成功！");else SS(L"恢复失败");}}
            else if(nmh->code==NM_RCLICK){NMITEMACTIVATE* na=(NMITEMACTIVATE*)l;int s=na->iItem;if(s>=0&&s<SLOTS){ListView_SetItemState(g_lv,s,LVIS_SELECTED,LVIS_SELECTED);POINT pt=na->ptAction;ClientToScreen(g_lv,&pt);ShowCtxMenu(h,s,pt.x,pt.y);}}
        }
        return 0;
    }
    case WM_TRAY:if(LOWORD(l)==WM_RBUTTONUP){HMENU mu=CreatePopupMenu();AppendMenuW(mu,MF_STRING,IDM_R,L"恢复主用布局");AppendMenuW(mu,MF_SEPARATOR,0,NULL);AppendMenuW(mu,MF_STRING,IDM_S,L"显示主窗口");AppendMenuW(mu,MF_STRING,IDM_U,L"卸载");AppendMenuW(mu,MF_SEPARATOR,0,NULL);AppendMenuW(mu,MF_STRING,IDM_X,L"退出");POINT pt;GetCursorPos(&pt);SetForegroundWindow(h);TrackPopupMenu(mu,TPM_RIGHTBUTTON,pt.x,pt.y,0,h,NULL);PostMessageW(h,WM_NULL,0,0);DestroyMenu(mu);}else if(LOWORD(l)==WM_LBUTTONDBLCLK){ShowWindow(h,SW_RESTORE);SetForegroundWindow(h);}return 0;
    case WM_SIZE:if(w==SIZE_MINIMIZED){ShowWindow(h,SW_HIDE);return 0;}return 0;
    case WM_CLOSE:ShowWindow(h,SW_HIDE);return 0;
    case WM_DESTROY:Shell_NotifyIconW(NIM_DELETE,&g_ni);if(g_br)DeleteObject(g_br);if(g_bgBr)DeleteObject(g_bgBr);PostQuitMessage(0);return 0;
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:case WM_CTLCOLORLISTBOX:
        if(g_dk){SetBkColor((HDC)w,DK_BG);SetTextColor((HDC)w,DK_FG);if(!g_br)g_br=CreateSolidBrush(DK_BG);return(LRESULT)g_br;}break;
    case WM_CTLCOLOREDIT:
        if(g_dk){SetBkColor((HDC)w,DK_BG);SetTextColor((HDC)w,DK_FG);if(!g_bgBr)g_bgBr=CreateSolidBrush(DK_BG);return(LRESULT)g_bgBr;}break;
    case WM_ERASEBKGND:
        if(g_dk){RECT rc;GetClientRect(h,&rc);FillRect((HDC)w,&rc,g_br?g_br:GetSysColorBrush(COLOR_WINDOW));return 1;}break;
    case WM_SETTINGCHANGE:
        if(l&&wcscmp((LPCWSTR)l,L"ImmersiveColorSet")==0){bool nd=IsDk();if(nd!=g_dk){g_dk=nd;if(g_dk)ApplyDarkToWindow(h);else InvalidateRect(h,NULL,TRUE);}}return 0;
    }
    return DefWindowProcW(h,m,w,l);
}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,PWSTR,int nS){
    wchar_t ad[MAX_PATH];SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,0,ad);swprintf_s(g_dir,MAX_PATH,L"%s\\ailiasDesktopLayoutSaver",ad);CreateDirectoryW(g_dir,NULL);LdCfg();
    INITCOMMONCONTROLSEX icc={sizeof(icc),ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&icc);
    g_dk=IsDk();
    WNDCLASSEXW wc={sizeof(wc)};wc.lpfnWndProc=WP;wc.hInstance=hi;wc.hIcon=LoadIconW(hi,MAKEINTRESOURCEW(IDI_APPICON));wc.hCursor=LoadCursorW(NULL,IDC_ARROW);
    wc.hbrBackground=g_dk?CreateSolidBrush(DK_BG):(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"DLS";RegisterClassExW(&wc);
    g_hWnd=CreateWindowExW(0,L"DLS",L"ailiasDesktopLayoutSaver",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,610,550,NULL,NULL,hi,NULL);
    ShowWindow(g_hWnd,nS);
    MSG msg;while(GetMessageW(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return (int)msg.wParam;
}