/***
 * -*- C++ -*-
 * -*- UTF-8 -*-
 *
 * Copyright (C),
 * @file
 * @brief
 * @author:
 * 		<name>		<email>
 * 		kins ren	renxiaoxian@gmail.com	
 * @version
 * @date
 * @warning
 * @History:
 * 		<author>	<time>	<desc>
*/

#include "urllib.h"

namespace URLUtil{


/**	需要处理#fragment */
void urlParse(const string& url,URLMetaStruct &meta)
{
	string strURL = url;
	size_t pos = strURL.rfind("#");
	if (pos != string::npos)
	{
		// 暂时去掉fragment
		strURL = strURL.substr(0,pos);
	}

	size_t p_path = strURL.find('?');	

	if(string::npos != p_path)
	{
		meta.query = strURL.substr(p_path+1);
	}else
	{
		p_path = strURL.length()+1;
	}

	size_t p_schema = strURL.find("://");

	if(string::npos != p_schema && p_schema < p_path)
	{
		meta.schema = strURL.substr(0,p_schema);
		p_schema = p_schema + 3;
		size_t p_host = strURL.find('/',p_schema);
		if(string::npos != p_schema && p_host < p_path)
		{
			meta.host = strURL.substr(p_schema,p_host-p_schema);
			meta.path = strURL.substr(p_host,p_path-p_host);
		}else
		{
			meta.host = strURL.substr(p_schema,p_path-p_schema);
			meta.path = "/";
		}
	}else
	{
		meta.path = strURL.substr(0,p_path);
	}

	size_t p_port = meta.host.find(":");
	if( string::npos != p_port )
	{
		meta.port = meta.host.substr(p_port+1, meta.host.size()-p_port-1);
		meta.host = meta.host.substr(0, p_port);
	}
}

string urlUnparse(const URLMetaStruct &meta)
{
	string url;	

	if(meta.schema != "")
	{
		url = meta.schema + "://" + meta.host;
	}

	if(meta.port != "")
	{
		url += ":" + meta.port;
	}

	if(meta.path != "")
	{
		if(meta.path.at(0) != '/')
		{
			url += "/" + meta.path;
		}else
		{
			url += meta.path;
		}
	}
	if(meta.query != "")
	{
		url+= "?" + meta.query;
	}
	
	return url;
}


string url2Absolute(const string& base,const string& href)
{
	URLMetaStruct meta_base,meta_href,meta_absolute;
	urlParse(base,meta_base);
	urlParse(href,meta_href);
	
	// href已经是绝对路径
	if(meta_href.schema != "")
	{
		return href;
	}else
	{
		meta_absolute.schema = meta_base.schema;
		meta_absolute.host = meta_base.host;
		meta_absolute.port = meta_base.port;

		if ((meta_href.path.size() > 0) && (meta_href.path[0] != '/'))
		{
			size_t parent= meta_base.path.rfind('/');
			size_t pos = 0;
			while((pos+3 <= meta_href.path.size()) && (meta_href.path.substr(pos,3) == "../"))
			{
				parent = meta_base.path.rfind('/',parent-1);
				if (string::npos == parent)
				{
					meta_href.path = "/";
					break;
				}
				pos += 3;
			}
			meta_absolute.path = meta_base.path.substr(0,parent+1) + meta_href.path.substr(pos,meta_href.path.length()-pos);
		}else
		{
			meta_absolute.path = meta_base.path;
		}

		meta_absolute.query = meta_href.query;

		return urlUnparse(meta_absolute);
	}
}

}
