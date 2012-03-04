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

class CV8Context
{
	private:
		Persistent<Context> m_ctx;
		string m_path;
		static CV8Context *m_instance;

	private:
		CV8Context();

	public:
		~CV8Context();
		static CV8Context * getInstance();

		bool run(const string &path);
		bool callJSFunction(Handle<Function> func, int argc, Handle<Value> argv[]);

		const string &path();
		const Persistent<Context> context();

		void clean();
};
