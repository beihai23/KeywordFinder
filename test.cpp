//vim:fileencoding=utf-8
#include <iostream>

#include "keywordfinder.h"
#ifndef WIN32
#include <sys/time.h>
#endif
#include <algorithm>

#ifdef WIN32
void ascii2unicode(string in, wstring& out)
{
    int wcsLen = MultiByteToWideChar(CP_ACP, NULL, in.c_str(), in.length(), NULL, 0);
    wchar_t* wsz_buf = new wchar_t[wcsLen + 1];
    wcsLen = MultiByteToWideChar(CP_ACP, NULL, in.c_str(), in.length(), wsz_buf, wcsLen);
    wsz_buf[wcsLen] = 0;
    out.assign(wsz_buf, wcslen(wsz_buf));
    delete[] wsz_buf;
}
#endif

uint64_t GetTickCountMS(void)
{
    uint64_t currentTime;
#ifdef WIN32
    currentTime = GetTickCount();
#else
    struct timeval current;
    gettimeofday(&current, NULL);
    currentTime = current.tv_sec * 1000 + current.tv_usec/1000;
#endif
    return currentTime;
}

int main(int argc, char** argv)
{
    try
    {
        uint64_t begin = GetTickCountMS();
        KeywordFinder* kfinder = new KeywordFinder("RestrictList.dat", 0);
        cout << "construct KeywordFinder exhaust:" << GetTickCountMS() - begin << " ms." << endl;

        while (true)
        {
            vector<KWPosition> kws;
            string input;
            cout << "please input content:";
            std::getline(cin, input);
            std::transform(input.begin(),input.end(),input.begin(),::tolower);

            wstring content;
#ifdef WIN32
            ascii2unicode(input, content);
#else
            kfinder->__utf8tounicode(input, content);
#endif

            cout << "length of input=" << input.size() << " and input content:" << input <<endl;

            kfinder->FindoutKeyword(content, kws, false);
            cout << "found:" << dec << kws.size() << endl;
            for (auto it = kws.begin(); it != kws.end(); ++it)
            {
                KWPosition& kw = *it;
                cout << "keyword pos:" << kw.position << " keyword length:" << kw.keyword.length() << endl;
            }
        }


        delete kfinder;
    }
    catch(const char* e)
    {
        cerr << e << endl;
        exit(0);
    }


}
