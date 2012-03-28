/***
 * -*- C++ -*-
 * -*- UTF-8 -*-
 *
 * Copyright (C),
 * @file
 * @brief
 * @author:
 * 		<name>		<email>
 * 		kins ren	kins.ren@me.com	
 * @version
 * @date
 * @warning
 * @History:
 * 		<author>	<time>	<desc>
*/

#pragma once

#include <string>
#include <vector>

#include "network.h"

using std::string;
using std::vector;

namespace StringUtil
{

static string trim(const string &str)
{	
	string dst;
	int len = str.size();
	
	int spos = 0;
	int epos = len - 1;

	for( int i=spos; i<len; i++ )
	{
		if( !(str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n') )
		{
			spos = i;
			break;
		}
	}
	
	if( spos < epos )
	{
		for( int i=epos; i>=0; i-- )
		{
			if( !(str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n') )
			{
				epos = i;
				break;
			}
		}
		dst = str.substr(spos, epos-spos+1);	
	}

	return dst;
}


static string lower(const string &str)
{
	string dst = str;

	for( int i=0; i< dst.size(); i++ )
	{
		if( dst[i] >= 0x41 && dst[i] <= 0x5A )
		{
			dst[i] = dst[i] + 0x20;
		}
	}

	return dst;
}

static string upper(const string &str)
{
	string dst = str;

	for( int i=0; i< dst.size() ; i++ )
	{
		if( dst[i] >= 0x61 && dst[i] <= 0x7a )
		{
			dst[i] = dst[i] - 0x20;
		}
	}

	return dst;
}

static bool startWith(const string &str, const string &prefix)
{
	if( prefix.empty() )
	{
		return false;
	}

	return (strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0);
}

static bool endWith(const string &str, const string &suffix)
{
	if( suffix.empty() )
	{
		return false;
	}

	return (strncmp(str.c_str()+(str.size()-suffix.size()), suffix.c_str(), suffix.size()) == 0);
}

static int getHexInt(const string &str, int pos, int len)
{
	const char *s = str.c_str();
	int slen = str.size();

	if( !s || slen <= 0 || pos >= slen || (pos+len)> slen || len <= 0 )
	{
		return 0;
	}

	int hexInt = 0;
	int unit = 1;
	for( int i=pos+len-1; i>=pos; i-- )
	{
		char hex = s[i];
		if( 0x30 <= hex && hex <= 0x39 )
		{
			hexInt += unit * (hex -0x30);
		}
		else if( 0x61 <= hex && hex <= 0x66 )
		{
			hexInt += unit * (hex - 0x61 + 10);
		}
		else
		{
			return 0;
		}

		unit *= 16;
	}

	return hexInt;
}

static int loadContentsOfURL(const string &url, string &content)
{
	Downloader downloader( &url );
	downloader.start();

	if( downloader.status() == 200 )
	{
		content = (char *)downloader.receivedData();
	}

	return downloader.status();
}

static void split(const string& line, const string& segmentor, vector<string>& segs)
{
	bool bStart = false;
	int iStart = 0;
	size_t i=0;
	for( i=0; i<line.length(); i++ )
	{
		bool bSegment = (segmentor.find(line[i]) != string::npos);
		if( !bStart && !bSegment )
		{
			// 开始
			bStart = true;
			iStart = i;
		}
		else if( bStart && bSegment )
		{
			// 结束
			bStart = false;
			segs.push_back(line.substr(iStart, i-iStart));
		}
	}

	// 最后一个
	if( bStart )
	{
		segs.push_back(line.substr(iStart, i-iStart));
	}
}

static bool convertHTMLColor(const string &style, unsigned int &color)
{
	static const char *normalColors[] = {
		"#000000", "black",	"#c0c0c0", "silver",
		"#ff0000", "red",	"#0000ff", "blue",
		"#ffffff", "white",	"#ffff00", "yellow",
		"#00ff00", "green",	"#00ffff", "aqua",
	};

	int normalCorlorCount = sizeof(normalColors)/sizeof(char *);
	
	string colorStyle = lower(trim(style));
	if( colorStyle.empty() )
	{
		return false;
	}
	
	if( !startWith(colorStyle, "#") && !startWith(colorStyle, "rgb") )
	{
		// 非#,rgb开头
		
		bool bNormalColor = false;
		for( int i=0; i<normalCorlorCount/2; i++ )
		{
			if( colorStyle == normalColors[i*2+1] )
			{
				colorStyle = normalColors[i];
				bNormalColor = true;
				break;
			}
		}

		if( !bNormalColor )
		{
			return false;
		}
	}
	
	int r,g,b;

	if( startWith(colorStyle, "#") )
	{
		if( colorStyle.size() == 7 )
		{
			// #ffff00
			r = getHexInt(colorStyle, 1, 2);
			g = getHexInt(colorStyle, 3, 2);
			b = getHexInt(colorStyle, 5, 2);
		}
		else if( colorStyle.size() == 4 )
		{
			// #ff0
			r = getHexInt(colorStyle, 1, 1);
			r = r*16+r;
			g = getHexInt(colorStyle, 2, 1);
			g = g*16 + g;
			b = getHexInt(colorStyle, 3, 1);
			b = b*16 + b;
		}
		else
		{
			return false;
		}
	}
	else if( startWith(colorStyle, "rgb") )
	{
		if( sscanf(colorStyle.c_str(), "rgb(%d,%d,%d)", &r, &g, &b) != 3 )
		{
			return false;
		}
	}

	if( r > 255 || g > 255 | b > 255 )
	{
		return false;
	}

	// ARGB
	color = 0xff000000 | r<<16 | g << 8 | b;
	return true;
}

};
