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
#include "train.h"
#include "ttosim.h"

void TSGui::record(vsg::CommandBuffer& cb) const
{
	TSGuiData& data= TSGuiData::instance();
	if (data.showStatus && myTrain) {
		ImGui::Begin("Train Status",&data.showStatus);
		int t= (int)simTime;
		ImGui::Text("Time: %d:%2.2d:%2.2d fps %.1lf",t/3600,t/60%60,t%60,data.fps);
		ImGui::Text("Speed: %.1f mph",myTrain->speed*2.23693);
		ImGui::Text("Accel: %6.3f g  %6.3f%%",myTrain->accel/9.8,-100*myTrain->location.grade());
		ImGui::Text("Reverser: %.0f%%",100*myTrain->dControl);
		ImGui::Text("Throttle: %.0f%%",100*myTrain->tControl);
		if (myTrain->engAirBrake)
			ImGui::Text("Brakes: %s %.0f %.0f %.0f %.0f %.0f %.1f",
			  myTrain->bControl<0?"R":myTrain->bControl>0?"S":"L",
			  myTrain->engAirBrake->getEqResPressure(),
			  myTrain->engAirBrake->getPipePressure(),
			  myTrain->engAirBrake->getAuxResPressure(),
			  myTrain->engAirBrake->getCylPressure(),
			  myTrain->engAirBrake->getMainResPressure(),
			  myTrain->engAirBrake->getAirFlowCFM());
		else
			ImGui::Text("Brakes: %.1f",myTrain->bControl);
		ImGui::Text("Eng Brakes: %.0f%%",100*myTrain->engBControl);
		if (selectedRailCar)
			ImGui::Text("Hand Brake: %.0f%%",100*selectedRailCar->handBControl);
		float bp= -1;
		for (auto c=myTrain->firstCar; c; c=c->next) {
			if (!c->engine)
				continue;
			auto e= dynamic_cast<SteamEngine*>(c->engine);
			if (e) {
				auto x= e->getBoilerPressure();
				if (bp < x)
					bp= x;
			}
		}
		if (bp > 0)
			ImGui::Text("Boiler Pressure: %.0f",bp);
		ImGui::End();
	}
	if (data.showSelect) {
		ImGui::Begin("Select",&data.showSelect);
		if (ImGui::BeginCombo("",data.selected.c_str())) {
			for (auto s: data.listItems) {
				if (ImGui::Selectable(s.c_str())) {
					data.selected= s;
				}
			}
			ImGui::EndCombo();
		}
		if (mstsRoute && data.selected!="Select an activity" && ImGui::Button("Load")) {
			mstsRoute->activityName= data.selected;
			data.showSelect= false;
		}
		if (!mstsRoute && data.selected!="Select a route" && ImGui::Button("Load")) {
			data.showSelect= false;
		}
		ImGui::End();
        }
	if (data.showMessage) {
		ImGui::Begin("Message",&data.showMessage);
		for (auto s: data.listItems)
			ImGui::TextWrapped("%s",s.c_str());
		ImGui::End();
	}
}

void TSGuiData::loadRouteList()
{
	listItems.clear();
	auto paths= vsg::getEnvPaths("MSTSDIRS");
	for (auto p: paths) {
		path mstsDir= p.string();
		for (const directory_entry& d: recursive_directory_iterator(mstsDir)) {
			const path& f= d;
			if (f.extension() == ".tdb") {
				path tdbFile= mstsDir;
				tdbFile/= f;
				listItems.push_back(tdbFile);
			}
		}
	}
	sort(listItems.begin(),listItems.end());
	selected= "Select a route";
	showSelect= true;
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
	showSelect= true;
}

void TSGuiData::displayMessage(std::string message)
{
	if (!showMessage)
		listItems.clear();
	listItems.push_back(message);
	showMessage= true;
}
