// vim:fileencoding=utf-8
/*
NOTE: 关键字过滤基于unicode工作。输入的内容必须是已经转换为unicode的字符串！
*/
#ifndef _H_F_KEYWORD_FINDER_I892487592R92_
#define _H_F_KEYWORD_FINDER_I892487592R92_

#ifdef WIN32
    #include <windows.h>
#else
    #include <iconv.h>
#endif

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <stdint.h>

using namespace std;

struct KWPosition
{
    wstring  keyword;
    int        position;
};

struct __ac_trie_node
{
    wchar_t                                  ch;
    uint64_t                                 isKey;
    unordered_map<wchar_t, __ac_trie_node* > child_li;
    __ac_trie_node*                          back;
    __ac_trie_node*                          parent;

    __ac_trie_node():ch(0),isKey(false),back(0),parent(0) 
    {
    }

    ~__ac_trie_node()
    {
        for (auto node_it = child_li.begin(); node_it != child_li.end(); ++node_it)
        {
            auto p_node = node_it->second;
            delete p_node;
        }
    }
};

struct __serialized_field
{
    void*                                    mem_addr;
    wchar_t                                  ch;
    uint64_t                                 isKey;
    __ac_trie_node*                          back;
    __ac_trie_node*                          parent;
};

class KeywordFinder
{
public:
	// 参数：关键字文件的路径，要求文件是utf-8编码。每个关键词占一行
    KeywordFinder(const char* filepath, int filetype = 0);
    ~KeywordFinder();

	// 找出内容中包含的关键字。参数说明：
	// source 待过滤的内容
	// words  找到的关键字
	// bAll   true:找出所有的关键字;
	//        false:忽略嵌套的关键字（例如："fuck" "ck"都是关键字。 输入是"fuck you"时只找出"fuck",忽略"uck"）
    int FindoutKeyword(const wstring& source_orig, vector<KWPosition>& words, bool bAll = true);
    bool __utf8tounicode(string in, wstring& out);

protected:
    void _get_key_pos(__ac_trie_node* node, int index, const wstring& source, vector<KWPosition>& words);
	bool _isEngAlpha(wchar_t wch);
    bool _load_keyword(const char* filepath);
    void _gen_trie();
    void _aho_corasick();

    int __insert_into_trie_tree(wstring& keyword);
    void __add_back_edge();
    __ac_trie_node*  __findout_keyword(wstring& word);
    int __get_keyword_from_node(__ac_trie_node* node, wstring& keyword);

    void __serialize(ofstream& dst_file);
    void __unserialize(ifstream& src_file);

    void  _split(string& src, vector<string>& tokens, char delim);

private:
    vector<wstring>  m_keywords;       
    __ac_trie_node*    m_ac_tree;

#ifndef WIN32
    iconv_t m_conv_ctx;
#endif
};

#endif // _H_F_KEYWORD_FINDER_I892487592R92_
