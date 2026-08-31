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

#include <vsg/all.h>

#include "vsgjson.h"

JsonObject JsonArray::getObject(int index)
{
	if (!objects || index>=size())
		return JsonObject({});
	return JsonObject(objects->children[index]);
}

JsonArray JsonArray::getArray(int index)
{
	if (!objects || index>=size())
		return JsonArray({});
	return JsonArray(objects->children[index]);
}

JsonObject JsonObject::getObject(std::string name)
{
	if (!object)
		return JsonObject({});
	return JsonObject(object->getObject(name));
}

JsonArray JsonObject::getArray(std::string name)
{
	if (!object)
		return JsonArray({});
	return JsonArray(object->getObject(name));
}
