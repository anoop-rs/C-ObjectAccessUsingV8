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
using namespace v8;

v8::Isolate* isolate;
v8::Handle<v8::FunctionTemplate> point_template;

struct Point
{
public:
	Point(int, int);
	~Point();
	int x_, y_;
};

Point::Point(int x, int y) :x_(x), y_(y)
{
}

Point::~Point()
{
}

v8::Handle<v8::Object> WrapPoint(Point* ptoWrap)
{
	EscapableHandleScope handle_scope(isolate);
	auto point_instance = point_template->New(isolate);
	//point_instance->Set SetInternalField(0, External::New(isolate, ptoWrap));
	return handle_scope.Escape(point_instance);
}


void constructorCall(const FunctionCallbackInfo<Value>& info)
{
	// throw if called without `new'
	if (!info.IsConstructCall())
		//return ThrowException(String::New("Cannot call constructor as function"));
		std::cout << "not from constr" << std::endl;

	//start a handle scope
	v8::HandleScope handle_scope(isolate);

	//get an x and y
	double x = info[0]->NumberValue();
	double y = info[1]->NumberValue();

	//generate a new point
	Point *point = new Point(x, y);

	//return the wrapped point
	//return WrapPoint(point);
	auto wrapped = WrapPoint(point);
	info.GetReturnValue().Set(wrapped);
	return;
}


void GetPointX(Local<String> property,
	const PropertyCallbackInfo<Value>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
	int value = static_cast<Point*>(ptr)->x_;
	info.GetReturnValue().Set(value);
}

void SetPointX(Local<String> property,
	Local<Value> value,
	const PropertyCallbackInfo<void>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
	static_cast<Point*>(ptr)->x_ = value->Int32Value();
}

void GetPointY(Local<String> property,
	const PropertyCallbackInfo<Value>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
	int value = static_cast<Point*>(ptr)->y_;
	info.GetReturnValue().Set(value);
}

void SetPointY(Local<String> property,
	Local<Value> value,
	const PropertyCallbackInfo<void>& info) {
	Local<Object> self = info.Holder();
	Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
	void* ptr = wrap->Value();
	static_cast<Point*>(ptr)->y_ = value->Int32Value();
}

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
	isolate = v8::Isolate::New(create_params);
	{
		v8::Isolate::Scope isolate_scope(isolate);
		// Create a stack-allocated handle scope.
		v8::HandleScope handle_scope(isolate);
		// Create a new context.
		v8::Local<v8::Context> context = v8::Context::New(isolate);
		// Enter the context for compiling and running the hello world script.
		v8::Context::Scope context_scope(context);

		//
		point_template = v8::FunctionTemplate::New(isolate);
		v8::Handle<v8::ObjectTemplate> point_instance_template = point_template->InstanceTemplate();
		point_instance_template->SetInternalFieldCount(1);
		point_instance_template->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), GetPointX, SetPointX);
		point_instance_template->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), GetPointY, SetPointY);

		auto global = context->Global();
		auto function = v8::FunctionTemplate::New(isolate,constructorCall);
		global->Set(v8::String::NewFromUtf8(isolate, "Point"), function->GetFunction());
		//

		// Create a string containing the JavaScript source code.
		v8::Local<v8::String> source =
			v8::String::NewFromUtf8(isolate, "function main(args){var a = 10; var b = 20; var c = a + b; return c;}",
				v8::NewStringType::kNormal)
			.ToLocalChecked();
		// Compile the source code.
		v8::Local<v8::Script> script =
			v8::Script::Compile(context, source).ToLocalChecked();
		// Run the script to get the result.
		v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
		// Convert the result to an UTF8 string and print it.
		v8::String::Utf8Value utf8(result);
		printf("%s\n", *utf8);

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

