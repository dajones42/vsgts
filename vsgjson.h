//	wrapper classes for processing json data read by vsg::read
//
/*
Copyright © 2026 Doug Jones

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef VSGJSON_H
#define VSGJSON_H

#include <vsg/all.h>

class JsonObject;

class JsonArray
{
public:
	vsg::Objects* objects;
	explicit JsonArray(vsg::ref_ptr<vsg::Object> object) {
		objects= dynamic_cast<vsg::Objects*>(object.get());
	}
	explicit JsonArray(vsg::Object* object) {
		objects= dynamic_cast<vsg::Objects*>(object);
	}
	int size() {
		if (!objects)
			return 0;
		return (int)objects->children.size();
	}
	std::string getString(int index, std::string dflt="") {
		if (!objects || index>=size())
			return dflt;
		auto p= (static_cast<const vsg::stringValue*>(objects->children[index].get()));
		return p ? p->value() : dflt;
	}
	int getInt(int index, int dflt=0) {
		if (!objects || index>=size())
			return dflt;
		auto p= (static_cast<const vsg::doubleValue*>(objects->children[index].get()));
		return p ? (int)p->value() : dflt;
	}
	int getDouble(int index, double dflt=0) {
		if (!objects || index>=size())
			return dflt;
		auto p= (static_cast<const vsg::doubleValue*>(objects->children[index].get()));
		return p ? p->value() : dflt;
	}
	int getBool(int index, bool dflt=false) {
		if (!objects || index>=size())
			return dflt;
		auto p= (static_cast<const vsg::boolValue*>(objects->children[index].get()));
		return p ? p->value() : dflt;
	}
	JsonObject getObject(int index);
	JsonArray getArray(int index);
};

class JsonObject
{
public:
	vsg::ref_ptr<vsg::Object> object;
	explicit JsonObject(vsg::ref_ptr<vsg::Object> o) {
		object= o;
	}
	explicit JsonObject(vsg::Object* o) {
		object= vsg::ref_ptr<vsg::Object>{o};
	}
	JsonObject(std::string path, vsg::ref_ptr<vsg::Options> options) {
		object= vsg::read_cast<vsg::Object>(path,options);
	}
	std::string getString(std::string name, std::string dflt="") {
		if (!object)
			return dflt;
		return vsg::value<std::string>(dflt,name,object);
	}
	int getInt(std::string name, int dflt=0) {
		if (!object)
			return dflt;
		return (int)vsg::value<double>(dflt,name,object);
	}
	int getDouble(std::string name, double dflt=0) {
		if (!object)
			return dflt;
		return vsg::value<double>(dflt,name,object);
	}
	int getBool(std::string name, bool dflt=false) {
		if (!object)
			return dflt;
		return vsg::value<bool>(dflt,name,object);
	}
	JsonObject getObject(std::string name);
	JsonArray getArray(std::string name);
};

#endif
