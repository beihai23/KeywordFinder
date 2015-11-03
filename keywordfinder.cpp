//vim:fileencoding=utf-8
#include "keywordfinder.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <sstream>

KeywordFinder::KeywordFinder(const char* filepath, int filetype)
:m_ac_tree(0)
{
#ifndef WIN32
    char toCoding[24] = {0};
    sprintf(toCoding, "UCS%d", sizeof(wchar_t));
    m_conv_ctx = iconv_open(toCoding,"UTF-8");
    if ((iconv_t)-1 == m_conv_ctx)
    {
        throw "iconv_open failed";
        return;
    }
#endif

    if (filetype == 0)
    {
        if (!_load_keyword(filepath))
        {
            throw "load word failed.";
            return;
        }

        m_ac_tree = new __ac_trie_node();
        _gen_trie();
        _aho_corasick();

        // prepare dump file stream.
        string dump_filepath = filepath;
        dump_filepath.append(".dump");
        ofstream df(dump_filepath, std::ios::binary | std::ios::out);
        if (!df.is_open())
        {
            cerr << "open dump file for serialize failed. " << dump_filepath << endl;
            return;
        }

        // serialize tries tree to dump file.
        __serialize(df);

    }
    else
    {
        // load trie tree from dump file.
        ifstream df(filepath, std::ios::binary | std::ios::in);
        if (!df.is_open())
        {
            cerr << "open dump file for unserialize failed. " << filepath << endl;
            return;
        }

        __unserialize(df);
    }

}

KeywordFinder::~KeywordFinder()
{
    delete m_ac_tree;
#ifndef WIN32
    if ((iconv_t)-1 != m_conv_ctx)
        iconv_close(m_conv_ctx);
#endif
}

int KeywordFinder::FindoutKeyword(const wstring& source_orig, vector<KWPosition>& words, bool bAll)
{
	// add by yanwei: convert input to lower
	wstring source = source_orig;
    
	std::transform(source.begin(),source.end(),source.begin(),::tolower);

    //======================================
    auto node = m_ac_tree;
    auto child_node = m_ac_tree;
    for (unsigned int index = 0; index < source.length(); ++index)
    {
        try
        {
            child_node = node->child_li.at(source[index]);
            if (child_node->isKey)
            {
                // found keyword.
                auto get_key_pos = [&](__ac_trie_node* __node)
                {
                    KWPosition kwp;
                    __get_keyword_from_node(__node, kwp.keyword);
                    kwp.position = index - kwp.keyword.length() + 1;

                    bool isEng = true;
                    for (unsigned int _ki = 0; _ki < kwp.keyword.length(); ++_ki)
                    {
                        if (!_isEngAlpha(kwp.keyword[_ki]))
                        {
                            isEng = false;
                            break;
                        }
                    }

                    if (isEng)
                    {
                        // 关键字是英文单词时，只匹配完整的单词(就是前后要有空格，或者是在行首或行尾)
                        int pos_head = kwp.position - 1;
                        int pos_tail = kwp.position + kwp.keyword.length();
                        if ((pos_head == -1 || !_isEngAlpha(source[pos_head]))
                                && (pos_tail == source.length() || !_isEngAlpha(source[pos_tail])))
                        {
                            words.push_back(kwp);
                        }
                    }
                    else
                    {
                        words.push_back(kwp);
                    }
                };
                get_key_pos(child_node);

                if (bAll)
                {
                    // 按返回边查找
                    auto back_node = child_node->back;
                    while (back_node && back_node != m_ac_tree)
                    {
                        get_key_pos(back_node);
                        back_node = back_node->back;
                    }
                }
            }

            node = child_node;
        }
        catch (std::out_of_range)
        {
            // next char in source.
            if (node != m_ac_tree)
            {
                // 当前查找的字符是从之前已经匹配上的字符的节点开始的
                --index;
            }

            if (node->back)
            {
                node = node->back;
            }
            else
            {
                node = m_ac_tree;
            }
            continue;
        }

    }

    return 0; 
}

bool KeywordFinder::_isEngAlpha(wchar_t wch)
{
	if (wch >= L'A' && wch <= L'z')
		return true;
	else
		return false;
}

bool KeywordFinder::_load_keyword(const char* filepath)
{
    ifstream kf(filepath, std::ios::in);
    if (!kf.is_open())
    {
        cerr << "open keyword file failed. " << filepath << endl;
        return false;
    }

    string in;
    while(getline(kf, in))
    {
        string::size_type n;
        n = in.rfind("\r\n");
        if (in.length() > 2 && in.length() - 2 == n)
        {
            in.pop_back();
            in.pop_back();
        }
        n = in.rfind("\n");
        if (in.length() > 1 && in.length() - 1 == n)
        {
            in.pop_back();
        }

        // trans keyword to unicode
        wstring keyword;
		// add by yanwei: convert string to lower
		std::transform(in.begin(), in.end(), in.begin(), ::tolower);
        if (!__utf8tounicode(in, keyword))
        {
            continue;
        }

        // append keyword to array.
        m_keywords.push_back(keyword);
    }

    return true;
}

void KeywordFinder::_gen_trie()
{
    for (auto it = m_keywords.begin(); it != m_keywords.end(); ++it)
    {
        // search key in trie tree , then add keyword into trie tree.
        wstring& keyword = *it;
        __insert_into_trie_tree(keyword);
    }
}

void KeywordFinder::_aho_corasick()
{
    __add_back_edge();
}

bool KeywordFinder::__utf8tounicode(string in, wstring& out)
{
#ifdef WIN32
    int wcsLen = MultiByteToWideChar(CP_UTF8, NULL, in.c_str(), in.length(), NULL, 0);
    wchar_t* wsz_buf = new wchar_t[wcsLen + 1];
    wcsLen = MultiByteToWideChar(CP_UTF8, NULL, in.c_str(), in.length(), wsz_buf, wcsLen);
    wsz_buf[wcsLen] = 0;
    out.assign(wsz_buf, wcslen(wsz_buf));
    delete[] wsz_buf;
#else
    char* szIn = (char*)in.c_str();
    size_t in_len = in.length();

    size_t out_len = in_len * sizeof(wchar_t) * 4;
    char* outBuf = new char[out_len + 1];
    char* szOut = outBuf;
    memset(szOut, 0, out_len + 1);

    size_t ret = iconv(m_conv_ctx, &szIn, &in_len, &szOut, &out_len);
    if (-1 == ret)
    {
        perror("iconv failed, errorno=");
        if ((iconv_t)-1 != m_conv_ctx)
            iconv_close(m_conv_ctx);

        delete[] outBuf;
        return false;
    }

    out = (wchar_t*)outBuf;
    delete[] outBuf;
#endif


    return true;
}

int KeywordFinder::__insert_into_trie_tree(wstring& keyword)
{
    auto node = m_ac_tree;
    for (unsigned int index = 0; index < keyword.length(); ++index)
    {
        try
        {
            node = node->child_li.at(keyword[index]);
        }
        catch (std::out_of_range)
        {
            // insert ch into here
            auto new_node = new __ac_trie_node();
            new_node->ch = keyword[index];
            new_node->isKey = false;
            new_node->parent = node;
            node->child_li[keyword[index]] = new_node;
            node = new_node;
        }
    }

    // mark this node as keyword
    node->isKey = true;
    return 0;
}

__ac_trie_node*  KeywordFinder::__findout_keyword(wstring& word)
{
    auto node = m_ac_tree;
    for (unsigned int index = 0; index < word.length(); ++index)
    {
        try
        {
            node = node->child_li.at(word[index]);
        }
        catch (std::out_of_range)
        {
            // not found key
            return m_ac_tree;
        }
    }

    return node->isKey ? node : m_ac_tree; 
}

void KeywordFinder::__add_back_edge()
{
    std::list<__ac_trie_node* > traverse_list;
    traverse_list.push_back(m_ac_tree);

    while(!traverse_list.empty())
    {
        auto node_in_list = traverse_list.front();
        traverse_list.pop_front();
        if (node_in_list->isKey)
        {
            // add back edge for this node
            wstring found_keyword;
            auto ___node = node_in_list;
            __get_keyword_from_node(___node, found_keyword);
            while (found_keyword.length())
            {
                found_keyword.erase(found_keyword.begin());
                ___node->back =  __findout_keyword(found_keyword);
				___node = ___node->back;
            }
        }

        auto node_it = node_in_list->child_li.begin();
        for (; node_it != node_in_list->child_li.end(); ++node_it)
        {
            traverse_list.push_back(node_it->second);
        }
    }
}

int KeywordFinder::__get_keyword_from_node(__ac_trie_node* node, wstring& keyword)
{
    do
    {
        keyword += node->ch;
		node = node->parent;
    }while (node != m_ac_tree);

    // reverse string
    for (unsigned int i = 0; i <  keyword.length()/2; i++)
    {
        auto ch = keyword[i];
        keyword[i] = keyword[keyword.length()-i-1];
        keyword[keyword.length()-i-1] = ch;
    }

    return 0; // done
}

void KeywordFinder::__serialize(ofstream& dst_file)
{
    std::list<__ac_trie_node* > traverse_list;
    traverse_list.push_back(m_ac_tree);
    __serialized_field sf;

    while(!traverse_list.empty())
    {
        auto node_in_list = traverse_list.front();
        traverse_list.pop_front();

        for (auto node_it = node_in_list->child_li.begin();
			 node_it != node_in_list->child_li.end();
			 ++node_it)
        {
            traverse_list.push_back(node_it->second);
        }

        // 序列化该节点
        sf.mem_addr = node_in_list;
        sf.ch = node_in_list->ch;
        sf.isKey = node_in_list->isKey;
        sf.back = node_in_list->back;
        sf.parent = node_in_list->parent;
        dst_file.write(reinterpret_cast<char*>(&sf), sizeof sf);
    }
}

void KeywordFinder::__unserialize(ifstream& src_file)
{
    unordered_map<uint64_t, __ac_trie_node* > _node_table;

    __serialized_field sf;
    while (src_file.read(reinterpret_cast<char*>(&sf), sizeof sf))
    {
        auto new_node = new __ac_trie_node();

        uint64_t _addr   = reinterpret_cast<uint64_t>(sf.mem_addr);
        _node_table[_addr] = new_node;
        new_node->ch     = sf.ch;
        new_node->isKey  = sf.isKey;
        if (sf.back)
        {
            new_node->back   = _node_table[reinterpret_cast<uint64_t>(sf.back)];
        }
        if (sf.parent)
        {
            new_node->parent = _node_table[reinterpret_cast<uint64_t>(sf.parent)];
            new_node->parent->child_li[new_node->ch] = new_node;
        }

        if (!m_ac_tree)
        {
            m_ac_tree = new_node;
        }
    }
}

void  KeywordFinder::_split(string& src, vector<string>& tokens, char delim)
{
    string tok;
    stringstream ss(src);
    while(std::getline(ss, tok, delim))
    {
        tokens.push_back(tok);
    }
}
