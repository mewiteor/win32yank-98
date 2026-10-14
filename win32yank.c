#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdbool.h>
#include <string.h>
int __getmainargs(int *, char ***, char ***, int, int *);

#define WriteS(s)                                                                \
    do                                                                           \
    {                                                                            \
        DWORD dw;                                                                \
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, sizeof(s) - 1, &dw, NULL); \
    } while (0)

void print_usage(void)
{
    WriteS("win32yank\n"
           "\n"
           "Usage:\n"
           "    win32yank -o [--lf]\n"
           "    win32yank -i [--crlf]\n"
           "\n"
           "Options:\n"
           "    -o          Print clipboard contents to stdout\n"
           "    -i          Set clipboard from stdin\n"
           "    --lf        Replace CRLF with LF before printing to stdout\n"
           "    --crlf      Replace lone LF bytes with CRLF before setting the clipboard\n"
           "\n");
}

__forceinline bool is_high_surrogate(WCHAR w)
{
    return 0xD800 <= w && w < 0xDC00;
}

__forceinline bool is_low_surrogate(WCHAR w)
{
    return 0xDC00 <= w && w < 0xE000;
}

char encode_one_utf8(char *buf, size_t *bs, LPCWSTR *w)
{
    if (!**w)
        return 0;
    DWORD code;
    if (is_low_surrogate(**w))
        return 1;
    else if (!is_high_surrogate(**w))
        code = *(*w)++;
    else if (!is_low_surrogate((*w)[1]))
        return 1;
    else
    {
        code = ((((*w)[0] - 0xD800) << 10) | ((*w)[1] - 0xDC00)) + 0x10000;
        *w += 2;
    }
    if (code < 0x80)
        buf[(*bs)++] = (char)code;
    else if (code < 0x800)
    {
        buf[(*bs)++] = (char)(0xC0 | (code >> 6));
        buf[(*bs)++] = (char)(0x80 | (code & 0x3F));
    }
    else if (code < 0x10000)
    {
        buf[(*bs)++] = (char)(0xE0 | (code >> 12));
        buf[(*bs)++] = (char)(0x80 | (code >> 6 & 0x3F));
        buf[(*bs)++] = (char)(0x80 | (code & 0x3F));
    }
    else if (code < 0x110000)
    {
        buf[(*bs)++] = (char)(0xF0 | (code >> 18));
        buf[(*bs)++] = (char)(0x80 | (code >> 12 & 0x3F));
        buf[(*bs)++] = (char)(0x80 | (code >> 6 & 0x3F));
        buf[(*bs)++] = (char)(0x80 | (code & 0x3F));
    }
    else
        return 1;
    return 2;
}

bool write_utf16_to_utf8(LPCWSTR w, bool lf)
{
    char   buf[0x88];
    size_t bs = 0;
    DWORD  dw;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    for (;;)
    {
        // 此时bs最大为0x7F
        char status = encode_one_utf8(buf, &bs, &w);
        if (status == 0)
            break;
        if (status == 1)
            return false;
        // 此时bs最大为0x83
        if (buf[bs - 1] == '\r' && lf)
        {
            status = encode_one_utf8(buf, &bs, &w);
            if (status == 0)
                break;
            if (status == 1)
                return false;
            // 此时bs最大为0x87
            if (buf[bs - 1] == '\n')
            {
                buf[bs - 2] = '\n';
                bs--;
            }
        }
        if (bs >= 0x80)
        {
            WriteFile(hStdOut, buf, bs, &dw, NULL);
            bs = 0;
        }
    }
    if (bs > 0)
        WriteFile(hStdOut, buf, bs, &dw, NULL);
    return true;
}

HGLOBAL read_utf8_to_utf16(bool crlf)
{
    HGLOBAL hClipMem = NULL;
    LPWSTR  wstr    = NULL;
    size_t  wlen   = 0;
    char    buf[0x20];
    DWORD   bytes_read      = 0;
    DWORD   carry     = 0;
    HANDLE  hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE  hHeap  = GetProcessHeap();
    while (ReadFile(hStdIn, buf + carry, sizeof(buf) - carry, &bytes_read, NULL) && bytes_read > 0)
    {
        bytes_read += carry;
        carry = 0;
        WCHAR  wbuf[0x50];
        size_t wsize = 0;
        DWORD  i     = 0;
        while (i < bytes_read)
        {
            if ((buf[i] & 0xF8) == 0xF0)
            {
                if (i + 4 > bytes_read)
                {
                    memmove(buf, buf + i, bytes_read - i);
                    carry = bytes_read - i;
                    break;
                }
                for (DWORD j = 1; j < 4; j++)
                {
                    if ((buf[i + j] & 0xC0) != 0x80)
                        goto err;
                }
                DWORD code = (buf[i] & 7) << 18 | (buf[i + 1] & 0x3F) << 12 | (buf[i + 2] & 0x3F) << 6 | (buf[i + 3] & 0x3F);
                code -= 0x10000;
                if (code >= 0x100000)
                    goto err;
                wbuf[wsize++] = (code >> 10 & 0x3FF) + 0xD800;
                wbuf[wsize++] = (code & 0x3FF) + 0xDC00;
                i += 4;
            }
            else if ((buf[i] & 0xF0) == 0xE0)
            {
                if (i + 3 > bytes_read)
                {
                    memmove(buf, buf + i, bytes_read - i);
                    carry = bytes_read - i;
                    break;
                }
                for (DWORD j = 1; j < 3; j++)
                {
                    if ((buf[i + j] & 0xC0) != 0x80)
                        goto err;
                }
                DWORD code = (buf[i] & 0xF) << 12 | (buf[i + 1] & 0x3F) << 6 | (buf[i + 2] & 0x3F);
                if (0xD800 <= code && code < 0xE000)
                    goto err;
                wbuf[wsize++] = (WCHAR)code;
                i += 3;
            }
            else if ((buf[i] & 0xE0) == 0xC0)
            {
                if (i + 2 > bytes_read)
                {
                    memmove(buf, buf + i, bytes_read - i);
                    carry = bytes_read - i;
                    break;
                }
                if ((buf[i + 1] & 0xC0) != 0x80)
                    goto err;
                wbuf[wsize++] = (buf[i] & 0x1F) << 6 | (buf[i + 1] & 0x3F);
                i += 2;
            }
            else if (buf[i] & 0x80)
                goto err;
            else
            {
                wbuf[wsize++] = buf[i];
                i++;
            }
            if (wbuf[wsize - 1] == '\n' && crlf)
            {
                WCHAR pre = 0;
                if (wsize > 1)
                    pre = wbuf[wsize - 2];
                else if (wlen > 0) // wsize == 1
                    pre = wstr[wlen - 1];
                if (pre != '\r')
                {
                    wbuf[wsize]   = L'\r';
                    wbuf[wsize++] = L'\n';
                }
            }
        }
        if (wsize)
        {
            if (!wstr)
            {
                wstr = HeapAlloc(hHeap, 0, (wlen + wsize) * sizeof(WCHAR));
                if (!wstr)
                    goto err;
            }
            else
            {
                LPVOID s = HeapReAlloc(hHeap, 0, wstr, (wlen + wsize) * sizeof(WCHAR));
                if (!s)
                    goto err;
                wstr = (LPWSTR)s;
            }
            memcpy(wstr + wlen, wbuf, wsize * sizeof(WCHAR));
            wlen += wsize;
        }
    }
    hClipMem = GlobalAlloc(GMEM_MOVEABLE, (wlen + 1) * sizeof(WCHAR));
    if (!hClipMem)
        goto err;
    LPVOID ptr = GlobalLock(hClipMem);
    if (!ptr)
    {
        GlobalFree(hClipMem);
        hClipMem = NULL;
        goto err;
    }
    if (wstr && wlen)
        memcpy(ptr, wstr, wlen * sizeof(WCHAR));
    ((WCHAR *)ptr)[wlen] = 0;
    GlobalUnlock(hClipMem);
err:
    if (wstr)
        HeapFree(hHeap, 0, wstr);
    return hClipMem;
}

int clipboard_paste(bool lf)
{
    int r = 1;
    if (!OpenClipboard(NULL))
        r = GetLastError();
    else
    {
        HANDLE hClip = GetClipboardData(CF_UNICODETEXT);
        if (!hClip)
        {
            r = GetLastError();
            if (!r)
                r = -3;
        }
        else
        {
            LPCWSTR w = GlobalLock(hClip);
            if (!w)
                r = GetLastError();
            else
                r = write_utf16_to_utf8(w, lf) ? 0 : -2;
            if (!GlobalUnlock(hClip) && !r)
                r = GetLastError();
        }
        if (!CloseClipboard() && !r)
            r = GetLastError();
    }
    return r;
}

int clipboard_copy(bool crlf)
{
    int r = 0;
    if (!OpenClipboard(NULL))
        r = GetLastError();
    else
    {
        if (!EmptyClipboard())
            r = GetLastError();
        else
        {
            HGLOBAL h = read_utf8_to_utf16(crlf);
            if (!h)
                r = -2;
            else if (!SetClipboardData(CF_UNICODETEXT, h))
            {
                r = GetLastError();
                GlobalFree(h);
            }
        }
        if (!CloseClipboard() && !r)
            r = GetLastError();
    }
    return r;
}

int main(int argc, char **argv)
{
    bool flag_i    = false;
    bool flag_o    = false;
    bool lf   = false;
    bool crlf = false;
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-i"))
            flag_i = true;
        else if (!strcmp(argv[i], "-o"))
            flag_o = true;
        else if (!strcmp(argv[i], "--lf"))
            lf = true;
        else if (!strcmp(argv[i], "--crlf"))
            crlf = true;
        else
        {
            print_usage();
            return 1;
        }
    }
    if (!(flag_i ^ flag_o) || flag_i && lf || flag_o && crlf)
    {
        print_usage();
        return 1;
    }
    if (flag_o)
        return clipboard_paste(lf);
    else // i
        return clipboard_copy(crlf);
}

void mainCRTStartup(void)
{
    int    argc;
    char **argv;
    char **envp;
    if (__getmainargs(&argc, &argv, &envp, 0, NULL) < 0)
        ExitProcess((UINT)-1);
    else
        ExitProcess((UINT)main(argc, argv));
}
