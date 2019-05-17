// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "libplatform/libplatform.h"
#include "v8.h"
//#include "v8-debug.h"
//#include "src\base\platform\platform.h"
#include <thread>

using namespace v8;
using namespace std;

// v8::Isolate* isolate;

struct Point
{
public:
	Point(int, int);
	~Point();
	int x_, y_;
};

Point::Point(int x, int y)
	: x_(x)
	, y_(y)
{
}

Point::~Point()
{
}

Point* UnwrapPoint(Local<Object> obj)
{
	Local<External> field = Local<External>::Cast(obj->GetInternalField(0));
	auto ptr = field->Value();
	auto pointPtr = static_cast<Point*>(ptr);
	return pointPtr;
}

void AccessorGetterCallbackFunction(Local<Name> property, const PropertyCallbackInfo<Value>& info)
{
	auto pointPtr = UnwrapPoint(info.Holder());
	bool bIsString = property->IsString();
	string accessed_property;
	auto value = Local<String>::Cast(property);
	String::Utf8Value utf8_value(info.GetIsolate(), Local<String>::Cast(property));
	accessed_property = *utf8_value;
	if (accessed_property == "x")
		info.GetReturnValue().Set(v8::Integer::New(info.GetIsolate(), pointPtr->x_));
	else if (accessed_property == "y")
		info.GetReturnValue().Set(v8::Integer::New(info.GetIsolate(), pointPtr->y_));
}

v8::Handle<v8::Object> WrapPoint(v8::Isolate* isolate, Point* ptoWrap)
{
	if (isolate != nullptr)
	{
		v8::EscapableHandleScope scope(isolate);
		auto point_function_template = v8::FunctionTemplate::New(isolate);
		auto instance_template = point_function_template->InstanceTemplate();
		instance_template->SetInternalFieldCount(1);
		instance_template->SetHandler(v8::NamedPropertyHandlerConfiguration(AccessorGetterCallbackFunction));
		auto result = instance_template->NewInstance();
		Local<External> ptr = External::New(isolate, ptoWrap);
		result->SetInternalField(0, ptr);
		return scope.Escape(result);
	}
	else
		return v8::Handle<v8::Object>();
}


void constructorCall(const v8::FunctionCallbackInfo<v8::Value>& info)
{
	v8::HandleScope handle_scope(info.GetIsolate());

	// get an x and y
	auto x = static_cast<int>(info[0]->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked());
	auto y = static_cast<int>(info[1]->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked());

	// generate a new point
	Point* point = new Point(x, y);

	auto pointObject = WrapPoint(info.GetIsolate(), point);
	info.GetReturnValue().Set(pointObject);

	return;
}

int main(int argc, char* argv[])
{
	// Initialize V8.
	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	auto platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();
	// Create a new Isolate and make it the current one.
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	auto isolate = v8::Isolate::New(create_params);
	{
		v8::Isolate::Scope isolate_scope(isolate);
		// Create a stack-allocated handle scope.
		v8::HandleScope handle_scope(isolate);

		// Create a new context.
		v8::Local<v8::Context> context = v8::Context::New(isolate/*, 0, global*/);
		// Enter the context for compiling and running the hello world script.
		v8::Context::Scope context_scope(context);
		//Create a function template
		auto function = v8::FunctionTemplate::New(isolate, constructorCall);
		//get the global object from context
		auto globalObj = context->Global();
		//Set the function template function to be called when user uses Point keyword
		globalObj->Set(context, v8::String::NewFromUtf8(isolate, u8"Point",
			v8::NewStringType::kNormal).ToLocalChecked(), function->GetFunction());
		// Create a string containing the JavaScript source code.
		v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, "function func(args){ \
				var p = new Point(1,2); \
				var x = p.x; \
				var y = p.y; \
				var a = x; \
				var b = y; \
				var c = a + b; \
				return c;\
				}",
			v8::NewStringType::kNormal)
			.ToLocalChecked();
		// Compile the source code.
		v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();
		// Run the script to get the result.
		script->Run(context).ToLocalChecked();

		v8::Local<v8::String> jsStrMainFunction
			= v8::String::NewFromUtf8(isolate, u8"func", v8::NewStringType::kNormal)
			.ToLocalChecked();

		v8::Local<v8::Array> jsArgs = v8::Array::New(isolate, 2);
		jsArgs->Set(context, 0, v8::Int32::New(isolate, 2));
		jsArgs->Set(context, 1, v8::Int32::New(isolate, 3));
		v8::Local<v8::Value> jsValArgs = jsArgs.As<v8::Value>();

		v8::Local<v8::Value> jsValMainFunc
			= context->Global()->Get(context, jsStrMainFunction).ToLocalChecked();
		v8::Local<v8::Function> jsMainFunc = v8::Local<v8::Function>::Cast(jsValMainFunc);

		v8::MaybeLocal<v8::Value> maybeResult
			= jsMainFunc->Call(context, v8::Null(isolate), 1, &jsValArgs);
		v8::Local<v8::Value> funcResult;
		if (maybeResult.ToLocal(&funcResult))
		{
			// Convert the result to an UTF8 string and print it.
			v8::String::Utf8Value utf8(isolate, funcResult);
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
