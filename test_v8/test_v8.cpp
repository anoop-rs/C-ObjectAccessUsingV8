// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "libplatform/libplatform.h"
#include "v8.h"
#include "v8-debug.h"
#include "src\base\platform\platform.h"
#include <thread>

int main(int argc, char* argv[]) {
	// Initialize V8.
	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	auto platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();
	// Create a new Isolate and make it the current one.
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator =
		v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	v8::Isolate* isolate = v8::Isolate::New(create_params);
	{
		v8::Isolate::Scope isolate_scope(isolate);
		// Create a stack-allocated handle scope.
		v8::HandleScope handle_scope(isolate);
		/*auto global = ObjectTemplate::New(isolate);
		global->Set(String::NewFromUtf8(GetIsolate(), "log", NewStringType::kNormal).ToLocalChecked(), )*/
		// Create a new context.
		v8::Local<v8::Context> context = v8::Context::New(isolate);
		// Enter the context for compiling and running the hello world script.
		v8::Context::Scope context_scope(context);

		// Create a string containing the JavaScript source code.
		v8::Local<v8::String> source =
			v8::String::NewFromUtf8(isolate, "function main(args){var a = 10; var b = 20; var c = a + b; return c;}",
				v8::NewStringType::kNormal)
			.ToLocalChecked();
		// Compile the source code.
		v8::Local<v8::Script> script =
			v8::Script::Compile(context, source).ToLocalChecked();
		// Run the script to get the result.
		script->Run(context).ToLocalChecked();

		v8::Local<v8::String> jsStrMainFunction = v8::String::NewFromUtf8(isolate, u8"main", v8::NewStringType::kNormal).ToLocalChecked();

		v8::Local<v8::Array> jsArgs = v8::Array::New(isolate, 2);
		jsArgs->Set(context, 0, v8::Int32::New(isolate, 2));
		jsArgs->Set(context, 1, v8::Int32::New(isolate, 3));
		v8::Local<v8::Value> jsValArgs = jsArgs.As<v8::Value>();

		v8::Local<v8::Value> jsValMainFunc = context->Global()->Get(context, jsStrMainFunction).ToLocalChecked();
		v8::Local<v8::Function> jsMainFunc = v8::Local<v8::Function>::Cast(jsValMainFunc);

		v8::MaybeLocal<v8::Value> maybeResult = jsMainFunc->Call(context, v8::Null(isolate), 1, &jsValArgs);
		v8::Local<v8::Value> funcResult;
		if (maybeResult.ToLocal(&funcResult))
		{
			// Convert the result to an UTF8 string and print it.
			v8::String::Utf8Value utf8(funcResult);
			printf("%s\n", *utf8);
		}
	}
	// Dispose the isolate and tear down V8.
	isolate->Dispose();
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete create_params.array_buffer_allocator;
	return 0;
}

