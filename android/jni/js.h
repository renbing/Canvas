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
		v8::Persistent<v8::Context> m_ctx;
		string m_path;
		static CV8Context *m_instance;

	private:
		CV8Context();

	public:
		~CV8Context();
		static CV8Context * getInstance();

		bool run(const string &path);
		bool callJSFunction(v8::Handle<v8::Function> func, int argc, v8::Handle<v8::Value> argv[]);

		const string &path();
		const v8::Persistent<v8::Context> context();

		void clean();
		void logException(const v8::TryCatch &tryCatch);
};
