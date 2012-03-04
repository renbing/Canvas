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

#include "network.h"

// 自定义String相关
class CString
{
	private:
		char *m_buf;
		int m_len;
	
	public:
		const char *c_str() const { return m_buf; }
		char *c_str(){ return m_buf; }
		size_t size() const { return m_len; }

		CString() { m_buf = new char[1]; m_buf[0] = 0; m_len = 0; }
		CString(char *s, int len);
		~CString() { if( m_buf ) {delete []m_buf;} }

		CString(const char *s)
		{
			setString(s);
		}

		CString(const CString &s)
		{
			setString(s.c_str());
		}


		CString& operator=(const CString &s);
		CString& operator=(const char *s);
		bool operator == (const CString &s);
		bool operator == (const char *s);

		void castToLower();
		void castToUpper();
		void trim();
		bool empty();
		bool hasPrefix(const char *prefix);
		void concat(CString &s);
		int readZipFile(zip *apkArchive, const char *name);
		int loadContentsOfURL(const char *url);

	private:
		void setString(const char *s);
};

CString::CString(char *s, int len)
{
	m_len = len > strlen(s) ? strlen(s) : len;

	m_buf = new char[m_len+1];
	m_buf[m_len] = 0;
	strncpy(m_buf, s, m_len);
}

CString& CString::operator=(const CString &s)
{
	if( this != &s )
	{
		delete []m_buf;
		setString(s.c_str());
	}

	return *this;
}

CString& CString::operator=(const char *s)
{
	delete []m_buf;
	setString(s);

	return *this;
}

bool CString::operator == (const CString &s)
{
	return (strcmp(m_buf, s.c_str()) == 0);
}

bool CString::operator == (const char *s)
{
	return (strcmp(m_buf, s) == 0 );
}

void CString::castToLower()
{
	for( int i=0; i< m_len; i++ )
	{
		if( m_buf[i] >= 0x41 && m_buf[i] <= 0x5A )
		{
			m_buf[i] = m_buf[i] + 0x20;
		}
	}
}

void CString::castToUpper()
{
	for( int i=0; i< m_len; i++ )
	{
		if( m_buf[i] >= 0x61 && m_buf[i] <= 0x7a )
		{
			m_buf[i] = m_buf[i] - 0x20;
		}
	}
}

void CString::trim()
{
	int spos = 0;
	int epos = m_len - 1;

	for( int i=spos; i<m_len; i++ )
	{
		if( !(m_buf[i] == ' ' || m_buf[i] == '\t' || m_buf[i] == '\r' || m_buf[i] == '\n') )
		{
			spos = i;
			break;
		}
	}
	
	if( spos < epos )
	{
		for( int i=epos; i>=0; i-- )
		{
			if( !(m_buf[i] == ' ' || m_buf[i] == '\t' || m_buf[i] == '\r' || m_buf[i] == '\n') )
			{
				epos = i;
				break;
			}
		}
		
		m_len = epos - spos + 1;
		char *newBuf = new char[m_len+1];
		newBuf[m_len] = 0;
		strncpy(newBuf, m_buf+spos, m_len);

		delete []m_buf;
		m_buf = newBuf;
	}
	else
	{
		m_buf[0] = 0;
		m_len = 0;
	}
}

bool CString::empty()
{
	return (!m_buf || m_len <= 0);
}

bool CString::hasPrefix(const char *prefix)
{
	if( !prefix || strlen(prefix) == 0 )
	{
		return false;
	}

	return (strncmp(m_buf, prefix, strlen(prefix)) == 0);
}

void CString::setString(const char *s)
{
	m_len = strlen(s);	
	m_buf = new char[m_len+1];
	m_buf[m_len] = 0;

	if( m_len > 0 )
	{
		strcpy(m_buf, s);
	}
}

void CString::concat(CString &s)
{
	if( !s.c_str() || s.size() <= 0 )
	{
		return;
	}
	
	m_len = m_len + s.size();

	char *newBuf = new char[m_len+1];
	strcpy(newBuf, m_buf);
	strcat(newBuf, s.c_str());

	delete []m_buf;
	m_buf = newBuf;
}

int CString::readZipFile(zip *apkArchive, const char *name)
{
	struct zip_stat zstat;
	zip_stat_init(&zstat);

	zip_file *file = zip_fopen(apkArchive, name, 0);
	if( !file )
	{
		LOG("Error opening %s from APK", name);
		return -1;
	}

	zip_stat(apkArchive, name, 0, &zstat);
	if( zstat.size <= 0 )
	{
		zip_fclose(file);
		return 0;
	}

	if( m_buf )
	{
		delete []m_buf;
	}

	m_len = zstat.size;
	m_buf = new char[m_len+1];
	m_buf[m_len] = 0;
	
	int bytesRead = zip_fread(file, m_buf, m_len);
	zip_fclose(file);

	return bytesRead;
}

int CString::loadContentsOfURL(const char *url)
{
	//LOG("string load by url:%s", url);

	string strUrl(url);
	Downloader downloader;
	downloader.downloadURL( &strUrl );

	if( downloader.status() == 200 )
	{
		if( m_buf )
		{
			delete []m_buf;
		}

		m_len = downloader.receivedDataLength();
		m_buf = new char[m_len+1];
		memcpy(m_buf, downloader.receivedData(), m_len);
		m_buf[m_len] = 0;
		
		return m_len;
	}

	return -1;
}

int getHexIntFromString(const char *s, int pos, int len)
{
	int slen = strlen(s);
	if( !s || slen <= 0 || pos >= slen || (pos+len)>= slen || len <= 0 )
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
			hexInt = unit * (hex -0x30);
		}
		else if( 0x61 <= hex && hex <= 0x66 )
		{
			hexInt = unit * (hex - 0x61);
		}
		else
		{
			return 0;
		}

		unit *= 16;
	}

	return hexInt;
}
