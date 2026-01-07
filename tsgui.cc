//	GUI for vsgts
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
#include <iostream>
#include <vsgImGui/imgui.h>
#include <string>
#include <filesystem>

using namespace std;
using namespace filesystem;

#include "tsgui.h"
#include "mstsroute.h"
#include "mstsfile.h"

void TSGui::record(vsg::CommandBuffer& cb) const
{
	TSGuiData& data= TSGuiData::instance();
	if (data.showGui && mstsRoute) {
		ImGui::Begin("vsgts",&data.showGui);
//		ImGui::Text("Select an activiy:.");
		if (ImGui::BeginCombo("",data.selected.c_str())) {
			for (auto s: data.listItems) {
				if (ImGui::Selectable(s.c_str())) {
					data.selected= s;
				}
			}
			ImGui::EndCombo();
		}
		if (data.selected!="Select an activity" && ImGui::Button("Load")) {
			mstsRoute->activityName= data.selected;
			data.showGui= false;
		}
		ImGui::End();
        }
}

void TSGuiData::loadActivityList()
{
	listItems.clear();
	path p { fixFilenameCase(mstsRoute->routeDir+mstsRoute->dirSep+"ACTIVITIES") };
	for (const directory_entry& d: directory_iterator(p)) {
		const path& f= d;
		if (f.extension() == ".act")
			listItems.push_back(f.stem());
		sort(listItems.begin(),listItems.end());
		selected= "Select an activity";
	}
	showGui= true;
}
