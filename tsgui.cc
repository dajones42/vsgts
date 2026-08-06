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
#include "camerac.h"
#include "interlocking.h"
#include "track.h"

void TSGui::record(vsg::CommandBuffer& cb) const
{
	TSGuiData& data= TSGuiData::instance();
	if (data.showMenu)
		showMainMenu();
	if (data.showStatus)
		showStatusWindow();
	if (data.showSelect)
		showSelectWindow();
	if (data.showMessage)
		showMessageWindow();
	if (interlocking)
		showLeversWindow();
	if (data.showBlocks)
		showBlockSheet();
	if (data.showSave)
		showSaveWindow();
}

void TSGui::showMainMenu()
{
	TSGuiData& data= TSGuiData::instance();
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (mstsRoute && ImGui::MenuItem("Save",""))
				data.showSave= true;
			if (mstsRoute && simTime>0 && ImGui::MenuItem("Load Consist","")) {
				mstsRoute->activityName= " Explore";
				data.loadConsistList();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Display")) {
			if (timeTable && ImGui::MenuItem("Train Status",""))
				data.showStatus= true;
			if (timeTable && ImGui::MenuItem("Block Sheet",""))
				data.showBlocks= true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Print")) {
			if (timeTable && ImGui::MenuItem("Time Sheet",""))
				timeTable->printTimeSheet2(stderr);
			if (timeTable && ImGui::MenuItem("Horizontal Time Sheet",""))
				timeTable->printTimeSheetHorz2(stderr);
			if (timeTable && ImGui::MenuItem("TimeTable",""))
				timeTable->print(stderr);
			if (timeTable && ImGui::MenuItem("Horizontal TimeTable",""))
				timeTable->printHorz(stderr);
			if (myTrain && ImGui::MenuItem("Train Forces","")) {
				fprintf(stderr,"name speed force drag coupler mass\n");
				for (auto car=myTrain->firstCar; car; car=car->next)
					fprintf(stderr,"%s %f %f %f %f %f\n",
					  car->def->name.c_str(),car->speed,
					  car->force,car->drag,car->cU,car->mass);
			}
			if (ImGui::MenuItem("Track Locations",""))
				printTrackLocations();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Dispatcher")) {
			if (ttoSim.dispatcher.ignoreOtherTrains && ImGui::MenuItem("Check Other Trains",""))
				ttoSim.dispatcher.ignoreOtherTrains= false;
			if (!ttoSim.dispatcher.ignoreOtherTrains && ImGui::MenuItem("Ignore Other Trains",""))
				ttoSim.dispatcher.ignoreOtherTrains= true;
			ImGui::EndMenu();
		}
		if (timeTable && ImGui::BeginMenu("Block")) {
			for (auto i=ttoSim.blockMenu.begin(); i!=ttoSim.blockMenu.end(); i++) {
				if (ImGui::MenuItem(i->first.c_str(),"")) {
					if (i->first.find("cleared") != std::string::npos) {
						auto r= i->second->getNextRow(0);
						if (timeTable->getRow(r)->getCallSign() == userOSCallSign)
							i->second->setBlockCleared(r,simTime);
						ttoSim.blockMenu.erase(i);
					} else if (i->first.find("entered") != std::string::npos) {
						auto r= i->second->getNextRow(0);
						if (timeTable->getRow(r)->getCallSign() != userOSCallSign)
							r= i->second->getRow();
						if (timeTable->getRow(r)->getCallSign() == userOSCallSign)
							i->second->setBlockEntered(r,simTime);
						ttoSim.blockMenu.erase(i);
					} else if (timeTable->getBlockFor(i->second,simTime)) {
						i->second->message= "";
						ttoSim.blockMenu.erase(i);
					}
					break;
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void TSGui::showStatusWindow()
{
	TSGuiData& data= TSGuiData::instance();
	ImGui::Begin("Train Status",&data.showStatus);
	int t= (int)simTime;
	ImGui::Text("Time: %d:%2.2d:%2.2d Time Mult: %d fps %.1lf",t/3600,t/60%60,t%60,timeMult,data.fps);
	if (myTrain) {
		ImGui::Text("Speed: %.1f mph  Accel: %6.3f g",
		  myTrain->speed*2.23693,myTrain->accel/9.8);
		ImGui::Text("Grade: %.3f %%",-100*myTrain->location.grade());
		ImGui::Text("Throttle: %.0f %%  Reverser: %.0f %%",
		  100*myTrain->tControl,100*myTrain->dControl);
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
		ImGui::Text("Eng Brakes: %.0f %%",100*myTrain->engBControl);
		if (selectedRailCar)
			ImGui::Text("Hand Brake: %.0f %%",100*selectedRailCar->handBControl);
		if (myRailCar && myRailCar->engine) {
			auto e= dynamic_cast<SteamEngine*>(myRailCar->engine);
			if (e)
				ImGui::Text("Boiler Pressure: %.0f",e->getBoilerPressure());
		}
		std::string slack;
		for (auto car=myTrain->firstCar; car!=myTrain->lastCar; car=car->next) {
			if (car->cU < 0)
				slack+= "<";
			else if (car->cU > 0)
				slack+= ">";
			else
				slack+= "-";
		}
		if (slack.size() > 0)
			ImGui::Text("Couplers: %s",slack.c_str());
	}
	if (timeTable) {
		for (int i=timeTable->getNumTrains()-1; i>=0; i--) {
			AITrain* train= (AITrain*) timeTable->getTrain(i);
			int r= train->getRow();
			int a= train->getActualAr(r);
			int d= train->getActualLv(r);
			if (!train->consist || r<0 || a<0)
				continue;
			if (d < 0)
				d= a;
			d/= 60;
			WLocation wl;
			train->consist->location.getWLocation(&wl);
			ImGui::Text("train %-5.5s %s %2d:%2.2d %s %4.1f m %2.0f mph %2.0f %3.0f",
			  train->getName().c_str(),
			  timeTable->getRow(r)->getCallSign().c_str(),
			  d/60,d%60,train->route.c_str(),
			  vsg::length(myLookAt->center-wl.coord)*3.281/5280,
			  train->consist->speed*2.24,
			  8*train->consist->tControl,100*train->consist->bControl);
			if (train->message.size() > 0)
				ImGui::Text(" %s",train->message.c_str());
		}
#if 0
		for (auto i=ttoSim.needsBlock.begin(); i!=ttoSim.needsBlock.end(); i++) {
			if (ImGui::Button(i->second.c_str()) &&
			  timeTable->getBlockFor(i->first,simTime)) {
				ttoSim.needsBlock.erase(i);
				break;
			}
		}
#endif
	}
	ImGui::End();
}

void TSGui::showSelectWindow()
{
	TSGuiData& data= TSGuiData::instance();
	ImGui::Begin("Select",&data.showSelect);
	if (ImGui::BeginCombo("",data.selected.c_str())) {
		for (auto s: data.listItems) {
			if (ImGui::Selectable(s.c_str())) {
				data.selected= s;
			}
		}
		ImGui::EndCombo();
	}
	if (mstsRoute && mstsRoute->activityName==" Explore" && data.selected!="Select a consist") {
		ImGui::Text("%s","Center start location the select Load.");
		if (ImGui::Button("Load")) {
			mstsRoute->consistName= data.selected;
			data.showSelect= false;
		}
	}
	if (mstsRoute && mstsRoute->activityName.size()==0 && data.selected!="Select an activity" &&
	  ImGui::Button("Load")) {
		mstsRoute->activityName= data.selected;
		data.showSelect= false;
	}
	if (!mstsRoute && data.selected!="Select a route" && ImGui::Button("Load")) {
		data.showSelect= false;
	}
	ImGui::End();
}

void TSGui::showMessageWindow()
{
	TSGuiData& data= TSGuiData::instance();
	ImGui::Begin("Message",&data.showMessage);
	for (auto s: data.listItems)
		ImGui::TextWrapped("%s",s.c_str());
	ImGui::End();
}

void TSGui::showSaveWindow()
{
	TSGuiData& data= TSGuiData::instance();
	ImGui::Begin("Save",&data.showSave);
	static char saveFilename[128]= "savefile";
	ImGui::InputText("",saveFilename,IM_ARRAYSIZE(saveFilename));
	if (ImGui::Button("Save")) {
		mstsRoute->saveState(saveFilename);
		data.showSave= false;
	}
	ImGui::End();
}

void TSGui::showLeversWindow()
{
	TSGuiData& data= TSGuiData::instance();
	ImGui::SetNextWindowContentSize(ImVec2(26*interlocking->getNumLevers(),50));
	ImGui::Begin("Levers",nullptr,ImGuiWindowFlags_HorizontalScrollbar);
	char buf[20];
	for (int i=0; i<interlocking->getNumLevers(); i++) {
		sprintf(buf,"%d",i+1);
		if (i > 0)
			ImGui::SameLine();
		auto state= interlocking->getState(i);
		auto color= interlocking->getColor(i);
		auto c1= .8*color + vsg::vec3(.1,.1,.1);
		auto c2= .9*color + vsg::vec3(.2,.2,.2);
		float y= state==Interlocking::NORMAL ? .25 : state==Interlocking::REVERSE ? .75 :
		  interlocking->getLockDurationS(i,simTime)>0 ? .5 : .375;
		int ht= 50;
		auto occ= interlocking->getSwitchOccupied(i);
		if (occ==Interlocking::OCCUPIED || occ==Interlocking::ROUTELOCK)
			ht= 55;
		auto sig= interlocking->getSignal(i);
		if (sig && sig->trainDistance>0)
			ht= 55;
		ImGui::PushID(i);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.r,color.g,color.b,1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c1.r,c1.g,c1.b,1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(c2.r,c2.g,c2.b,1));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(.5,y));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
		if (ImGui::Button(buf,ImVec2(18,ht)))
			interlocking->toggleState(i,simTime);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		ImGui::PopID();
	}
	ImGui::End();
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
	listItems.push_back(" Explore");
	path p { fixFilenameCase(mstsRoute->routeDir+mstsRoute->dirSep+"ACTIVITIES") };
	for (const directory_entry& d: directory_iterator(p)) {
		const path& f= d;
		if (f.extension()==".act" || f.extension()==".json")
			listItems.push_back(f.filename());
	}
	sort(listItems.begin(),listItems.end());
	selected= "Select an activity";
	if (listItems.size() == 1) {
		mstsRoute->activityName= " Explore";
		loadConsistList();
	}
	showSelect= true;
}

void TSGuiData::loadConsistList()
{
	listItems.clear();
	path p { fixFilenameCase(mstsRoute->mstsDir+mstsRoute->dirSep+"TRAINS"+mstsRoute->dirSep+"CONSISTS") };
	for (const directory_entry& d: directory_iterator(p)) {
		const path& f= d;
		if (f.extension() == ".con") {
			path file= p;
			file/= f;
			MSTSFile conFile;
			conFile.readFile(file.c_str());
			MSTSFileNode* cfg= conFile.getFirstNode()->get("TrainCfg");
			string engFile;
			for (auto node=cfg->get(0); node; node=node->next) {
				if (node->value && *(node->value)=="Engine") {
					auto data= node->get("EngineData");
					engFile= *(data->get(0)->value);
					break;
				}
			}
			if (simTime>0 || engFile.size()>0)
				listItems.push_back(engFile+" |"+f.stem().string());
		}
	}
	sort(listItems.begin(),listItems.end());
	selected= "Select a consist";
	showSelect= true;
}

void TSGuiData::displayMessage(std::string message)
{
	if (!showMessage)
		listItems.clear();
	listItems.push_back(message);
	showMessage= true;
}

void TSGui::showBlockSheet()
{
	TSGuiData& data= TSGuiData::instance();
	ImGui::Begin("Block Sheet",&data.showBlocks);
	int t= (int)simTime;
	ImGui::Text("Time: %d:%2.2d:%2.2d",t/3600,t/60%60,t%60);
	for (int i=0; i<timeTable->getNumBlocks(); i++) {
		tt::Block* b= timeTable->getBlock(i);
		ImGui::Text("%s-%s",b->station1->getCallSign().c_str(),
		  b->station2->getCallSign().c_str());
		int j= b->trainTimes.size()-1;
		if (j >= 0) {
			auto tt= &b->trainTimes[j];
			ImGui::SameLine();
			ImGui::Text("%s %2.2d:%2.2d",
			  b->trainTimes[j].train->getName().c_str(),
			  b->trainTimes[j].timeGiven/3600,
			  b->trainTimes[j].timeGiven/60%60);
			if (b->trainTimes[j].timeEntered >= 0) {
				ImGui::SameLine();
				ImGui::Text("%2.2d:%2.2d",
				  b->trainTimes[j].timeEntered/3600,
				  b->trainTimes[j].timeEntered/60%60);
			}
			if (b->trainTimes[j].timeCleared >= 0) {
				ImGui::SameLine();
				ImGui::Text("%2.2d:%2.2d",
				  b->trainTimes[j].timeCleared/3600,
				  b->trainTimes[j].timeCleared/60%60);
			}
		}
	}
	ImGui::End();
}
