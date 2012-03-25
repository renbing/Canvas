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

#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace URLUtil{

/** be carefull
 	there is a deal: the URLMetaStruct::path must begin with "/",if the it doesn't contain
	url,set it to "/" as default
*/

/**
 *	用来描述URL结构,scheme://host:port/path?query
 *	schema:协议 http ftp https etc
 *	host:域名或者ip地址
 *	port:端口号
 *	path:文档路经
 *	query:查询参数
*/

struct URLMetaStruct
{
	string schema;
	string host;
	string port;
	string path;
	string query;
};

/** 将url地址解析成URLMetaStruct结构 */
void urlParse(const string& url,URLMetaStruct& meta);

/** 将URLMetaStruct结构还原成url地址 */
string urlUnparse(const URLMetaStruct& meta);

/** 通过base地址,将相对路径转换为绝对路径 */
string url2Absolute(const string& base,const string& href);

}
